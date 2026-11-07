#include "config.h"

#include <windows.h>

#include "app.h"
#include "layout.h"

namespace {

bool ReadDword(const wchar_t* name, DWORD& value) {
    DWORD size = sizeof(value), type = 0;
    return RegGetValueW(HKEY_CURRENT_USER, kRegKey, name, RRF_RT_REG_DWORD, &type, &value,
                        &size) == ERROR_SUCCESS;
}

}  // namespace

void SaveCfg() {
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRegKey, 0, nullptr, 0, KEY_WRITE, nullptr, &key,
                        nullptr) != ERROR_SUCCESS) {
        return;
    }

    const DWORD values[] = {
        (DWORD)g_ballPos.left,
        (DWORD)g_ballPos.top,
        (DWORD)g_panelPos.left,
        (DWORD)g_panelPos.top,
        (DWORD)g_metric,
        (DWORD)(g_topmost ? 1 : 0),
        (DWORD)(g_hidden ? 1 : 0),
        (DWORD)(g_tipShown ? 1 : 0),
        (DWORD)(g_panelPosValid ? 1 : 0),
    };
    const wchar_t* names[] = {
        L"x", L"y", L"px", L"py", L"metric", L"topmost", L"hidden", L"tipShown", L"pvalid",
    };
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
        RegSetValueExW(key, names[i], 0, REG_DWORD, (const BYTE*)&values[i], sizeof(DWORD));
    }
    RegCloseKey(key);
}

void LoadCfg() {
    const int d  = BallD();
    const int pw = PanelW();
    const int ph = PanelH();

    DWORD x = 0, y = 0, px = 0, py = 0, v = 0;
    if (ReadDword(L"x", x) && ReadDword(L"y", y)) {
        g_ballPos = { (LONG)x, (LONG)y, (LONG)x + d, (LONG)y + d };
    }
    if (ReadDword(L"pvalid", v) && v != 0 && ReadDword(L"px", px) && ReadDword(L"py", py)) {
        g_panelPos      = { (LONG)px, (LONG)py, (LONG)px + pw, (LONG)py + ph };
        g_panelPosValid = true;
    }
    if (ReadDword(L"metric", v) && v < M_COUNT) g_metric = (int)v;
    if (ReadDword(L"topmost", v)) g_topmost = (v != 0);
    if (ReadDword(L"hidden", v)) g_hidden = (v != 0);
    if (ReadDword(L"tipShown", v)) g_tipShown = (v != 0);

    // 屏幕配置可能变过（拔显示器 / 改分辨率），位置必须收敛回工作区
    if (g_ballPos.right > g_ballPos.left) ClampToWorkArea(g_ballPos);
    if (g_panelPos.right > g_panelPos.left) ClampToWorkArea(g_panelPos);
}
