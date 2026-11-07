#include "system_load.h"

#include <windows.h>

namespace {

ULONGLONG g_prev_idle   = 0;
ULONGLONG g_prev_kernel = 0;
ULONGLONG g_prev_user   = 0;
bool      g_first       = true;

ULONGLONG ToU64(const FILETIME& ft) {
    return ((ULONGLONG)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
}

}  // namespace

double CpuUsageRead() {
    FILETIME idle, kernel, user;
    if (!GetSystemTimes(&idle, &kernel, &user)) return -1.0;

    ULONGLONG i = ToU64(idle);
    ULONGLONG k = ToU64(kernel);
    ULONGLONG u = ToU64(user);

    ULONGLONG di = i - g_prev_idle;
    ULONGLONG dk = k - g_prev_kernel;
    ULONGLONG du = u - g_prev_user;
    g_prev_idle = i;
    g_prev_kernel = k;
    g_prev_user = u;

    if (g_first) {          // 首次只建基线，没有可用的增量
        g_first = false;
        return -1.0;
    }

    ULONGLONG total = dk + du;      // kernel 已含 idle
    if (total == 0) return -1.0;

    double pct = (double)(total - di) * 100.0 / (double)total;
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    return pct;
}

void RamRead(double& used_gb, double& total_gb, double& percent) {
    MEMORYSTATUSEX ms{};
    ms.dwLength = sizeof(ms);
    if (!GlobalMemoryStatusEx(&ms)) return;
    constexpr double kGiB = 1024.0 * 1024.0 * 1024.0;
    total_gb = (double)ms.ullTotalPhys / kGiB;
    used_gb  = (double)(ms.ullTotalPhys - ms.ullAvailPhys) / kGiB;
    percent  = (double)ms.dwMemoryLoad;
}
