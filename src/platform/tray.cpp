#include "tray.h"

#include <windows.h>
#include <shellapi.h>

#include "app.h"
#include "resource.h"
#include "util.h"

namespace {

NOTIFYICONDATAW g_nid{};

}  // namespace

void TrayAdd() {
    ZeroMemory(&g_nid, sizeof(g_nid));
    g_nid.cbSize           = sizeof(g_nid);
    g_nid.hWnd             = g_hwnd;
    g_nid.uID              = 1;
    g_nid.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_nid.uCallbackMessage = kWmTray;
    g_nid.hIcon = (HICON)LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_APP_ICON),
                                    IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
    if (!g_nid.hIcon) g_nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    CopyTip(g_nid.szTip, 128, Fmt(L"%ls %ls", kAppTitle, kAppVersion).c_str());
    Shell_NotifyIconW(NIM_ADD, &g_nid);

#ifndef NOTIFYICON_VERSION_4
#define NOTIFYICON_VERSION_4 4
#endif
    g_nid.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &g_nid);
}

void TrayDelete() {
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

void TrayBalloon(const wchar_t* title, const wchar_t* text) {
    g_nid.uFlags |= NIF_INFO;
    CopyTip(g_nid.szInfoTitle, 64, title);
    CopyTip(g_nid.szInfo, 256, text);
    g_nid.dwInfoFlags = NIIF_INFO;
    g_nid.uTimeout    = 10000;
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
    g_nid.uFlags &= ~NIF_INFO;
}

void ShowTrayMenu() {
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, IDM_SHOW, g_hidden ? L"显示悬浮球" : L"隐藏悬浮球");
    AppendMenuW(menu, MF_STRING, IDM_EXPAND, g_panel ? L"收起为悬浮球" : L"展开面板");
    AppendMenuW(menu, MF_STRING | (g_topmost ? MF_CHECKED : MF_UNCHECKED), IDM_TOPMOST,
                L"置顶显示");
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
