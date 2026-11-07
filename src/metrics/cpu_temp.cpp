#include "cpu_temp.h"

#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>

#include <vector>

namespace {

PDH_HQUERY   g_query   = nullptr;
PDH_HCOUNTER g_counter = nullptr;
bool         g_ok      = false;

// pdh.dll 的两个"英文计数器"入口要动态取：中文系统上本地化路径
// （"\热能区域(*)\温度"）用 PdhAddCounterW 找不到，PdhAddEnglishCounterW
// 中英文路径都能过。
typedef PDH_STATUS(WINAPI* PFN_PdhAddEnglishCounterW)(PDH_HQUERY, LPCWSTR, DWORD_PTR,
                                                      PDH_HCOUNTER*);
typedef PDH_STATUS(WINAPI* PFN_PdhGetFormattedCounterArrayW)(PDH_HCOUNTER, DWORD, LPDWORD,
                                                             LPDWORD,
                                                             PDH_FMT_COUNTERVALUE_ITEM_W*);

PFN_PdhAddEnglishCounterW        g_add_english = nullptr;
PFN_PdhGetFormattedCounterArrayW g_get_array   = nullptr;

bool LoadPdh() {
    if (g_add_english && g_get_array) return true;
    HMODULE lib = GetModuleHandleW(L"pdh.dll");
    if (!lib) lib = LoadLibraryW(L"pdh.dll");
    if (!lib) return false;
    // GetProcAddress 的返回值转函数指针必须绕一层 void*，直接强转会触发
    // -Wcast-function-type。
    if (!g_add_english) {
        g_add_english = (PFN_PdhAddEnglishCounterW)(void*)GetProcAddress(
            lib, "PdhAddEnglishCounterW");
    }
    if (!g_get_array) {
        g_get_array = (PFN_PdhGetFormattedCounterArrayW)(void*)GetProcAddress(
            lib, "PdhGetFormattedCounterArrayW");
    }
    return g_add_english != nullptr && g_get_array != nullptr;
}

// 先试英文路径，失败再回退本地化名。
PDH_STATUS AddCounterAny(PDH_HQUERY q, const wchar_t* path, PDH_HCOUNTER* h) {
    PDH_STATUS st = PDH_CSTATUS_NO_OBJECT;
    if (g_add_english) st = g_add_english(q, path, 0, h);
    if (st != ERROR_SUCCESS) st = PdhAddCounterW(q, path, 0, h);
    return st;
}

}  // namespace

bool CpuTempInit() {
    if (!LoadPdh()) return false;
    if (PdhOpenQueryW(nullptr, 0, &g_query) != ERROR_SUCCESS) return false;

    const wchar_t* path = L"\\Thermal Zone Information(*)\\Temperature";
    if (AddCounterAny(g_query, path, &g_counter) != ERROR_SUCCESS) {
        PdhCloseQuery(g_query);
        g_query = nullptr;
        return false;
    }
    // PDH 需要两次采集才有可用的格式化值
    PdhCollectQueryData(g_query);
    PdhCollectQueryData(g_query);
    g_ok = true;
    return true;
}

double CpuTempRead() {
    if (!g_ok || !g_get_array) return -1.0;
    if (PdhCollectQueryData(g_query) != ERROR_SUCCESS) return -1.0;

    DWORD size = 0, type = 0;
    // PDH_STATUS 是 signed long，PDH_MORE_DATA 是无符号常量，必须 (DWORD) 强转
    PDH_STATUS st = g_get_array(g_counter, PDH_FMT_DOUBLE, &size, &type, nullptr);
    if ((DWORD)st != (DWORD)PDH_MORE_DATA || size == 0) return -1.0;

    std::vector<BYTE> buf(size);
    PDH_FMT_COUNTERVALUE_ITEM_W* items = (PDH_FMT_COUNTERVALUE_ITEM_W*)buf.data();
    if (g_get_array(g_counter, PDH_FMT_DOUBLE, &size, &type, items) != ERROR_SUCCESS) return -1.0;

    DWORD count = size / sizeof(PDH_FMT_COUNTERVALUE_ITEM_W);
    for (DWORD i = 0; i < count; ++i) {
        // CStatus == 0 是 VALID_DATA，不是"没数据"
        if (items[i].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA ||
            items[i].FmtValue.CStatus == PDH_CSTATUS_NEW_DATA) {
            double t = items[i].FmtValue.doubleValue - 273.15;   // 计数器单位是开尔文
            if (t < -20.0 || t > 150.0) return -1.0;             // 防御荒谬值
            return t;
        }
    }
    return -1.0;
}

void CpuTempClose() {
    if (g_query) {
        PdhCloseQuery(g_query);
        g_query = nullptr;
    }
    g_counter = nullptr;
    g_ok = false;
}
