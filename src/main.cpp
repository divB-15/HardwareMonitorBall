// ============================================================================
//  main.cpp — 入口：单实例、主题、DPI、注册窗口类、消息循环
//
//  HardwareMonitorBall · 极简硬件监视器
//  纯 Win32 + GDI，零第三方依赖，不需要管理员权限。
//
//  数据源（全部普通用户权限可读）：
//    CPU 温度  PDH \Thermal Zone Information(*)\Temperature（ACPI 热区，开尔文）
//    SSD 温度  IOCTL_STORAGE_QUERY_PROPERTY / StorageDeviceTemperatureProperty
//    CPU 占用  GetSystemTimes 差分（kernel 已含 idle）
//    内存      GlobalMemoryStatusEx
//
//  模块划分见 docs/ARCHITECTURE.md，构建见 build.bat / CMakeLists.txt。
// ============================================================================
#include <windows.h>

#include "app.h"
#include "config.h"
#include "layout.h"
#include "resource.h"
#include "theme.h"

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int) {
    // 单实例：重复启动时把已在跑的球唤回，而不是再开一个
    HANDLE mutex = CreateMutexW(nullptr, TRUE, kMutexName);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (HWND exist = FindWindowW(kWndClass, nullptr)) {
            PostMessageW(exist, WM_COMMAND, IDM_SHOW, 0);
        }
        if (mutex) CloseHandle(mutex);
        return 0;
    }

    LoadSystemPalette();

    HDC screen = GetDC(nullptr);
    int dpi = GetDeviceCaps(screen, LOGPIXELSY);
    ReleaseDC(nullptr, screen);
    if (dpi <= 0) dpi = 96;
    g_scale = (float)dpi / 96.0f;

    // 位置 / 指标 / 置顶 / 隐藏状态（换算物理尺寸，所以必须在 g_scale 之后）
    LoadCfg();

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = kWndClass;
    wc.hIcon   = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, 0, 0,
                                   LR_DEFAULTSIZE | LR_SHARED);
    wc.hIconSm = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, 16, 16,
                                   LR_SHARED);
    if (!wc.hIcon)   wc.hIcon   = LoadIconW(nullptr, IDI_APPLICATION);
    if (!wc.hIconSm) wc.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);
    if (!RegisterClassExW(&wc)) {
        if (mutex) CloseHandle(mutex);
        return 1;
    }

    // 默认停靠在屏幕右侧（注册表里没有位置记录时）
    if (g_ballPos.right - g_ballPos.left <= 0) {
        RECT wa{};
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
        const int d = BallD();
        const int x = wa.right - d - 64;
        const int y = wa.top + 180;
        ClampToWorkArea(g_ballPos = { x, y, x + d, y + d });
    }

    HWND hwnd = CreateWindowExW(
        WS_EX_TOOLWINDOW,          // 不进任务栏、不进 Alt-Tab
        kWndClass, kAppTitle, WS_POPUP,
        g_ballPos.left, g_ballPos.top, BallD(), BallD(),
        nullptr, nullptr, hInst, nullptr);
    if (!hwnd) {
        if (mutex) CloseHandle(mutex);
        return 1;
    }

    if (g_topmost) {
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }

    ShowWindow(hwnd, g_hidden ? SW_HIDE : SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    if (mutex) CloseHandle(mutex);
    return (int)msg.wParam;
}
