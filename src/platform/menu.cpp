#include "menu.h"

#include <windows.h>

#include "app.h"

void ShowContextMenu() {
    HMENU menu = CreatePopupMenu();
    for (int i = 0; i < M_COUNT; ++i) {
        AppendMenuW(menu, MF_STRING | (g_metric == i ? MF_CHECKED : MF_UNCHECKED),
                    IDM_METRIC_BASE + i, kMetricDef[i].name);
    }
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, IDM_EXPAND, g_panel ? L"收起为悬浮球" : L"展开面板");
    AppendMenuW(menu, MF_STRING | (g_topmost ? MF_CHECKED : MF_UNCHECKED), IDM_TOPMOST,
                L"置顶显示");
    AppendMenuW(menu, MF_STRING, IDM_HIDE, L"隐藏到托盘");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, IDM_EXIT, L"退出");

    POINT pt;
    GetCursorPos(&pt);
    // 先抢前台再弹菜单，否则点击菜单外部时菜单不会自动消失
    SetForegroundWindow(g_hwnd);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, g_hwnd, nullptr);
    PostMessageW(g_hwnd, WM_NULL, 0, 0);
    DestroyMenu(menu);
}
