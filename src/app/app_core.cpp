// ============================================================================
//  app/app_core.cpp — 全局状态定义与生命周期
//
//  这里是全工程唯一定义 g_* 状态的地方（声明在 app.h）。其他模块只读/读改，
//  但"变量属于谁"始终清楚：状态属于 app 层。
// ============================================================================
#include "app.h"

#include "config.h"
#include "cpu_temp.h"
#include "layout.h"
#include "metrics.h"
#include "paint.h"
#include "system_load.h"
#include "text.h"
#include "theme.h"
#include "tooltip.h"
#include "tray.h"

// --- 状态定义 ---------------------------------------------------------------
Metrics g_m;

bool g_panel          = false;      // false = 悬浮球，true = 展开面板
int  g_metric         = M_CPU_TEMP; // 默认显示 CPU 温度
bool g_topmost        = true;
bool g_hidden         = false;
bool g_hover          = false;
bool g_pressed        = false;
bool g_btnHover[2]    = { false, false };
bool g_tipShown       = false;
bool g_panelPosValid  = false;

HWND g_hwnd = nullptr;
HWND g_tip  = nullptr;
std::wstring g_tipText;

RECT g_ballPos{};
RECT g_panelPos{};
RECT g_rcBtnCollapse{};
RECT g_rcBtnExit{};

UINT g_nproc = 0;
std::wstring g_ssdModel;
int  g_ssdDrive = -1;

bool  g_captured = false;
bool  g_moved    = false;
POINT g_dragStart{};
POINT g_dragStartScreen{};
RECT  g_dragWin{};

// --- 生命周期 ---------------------------------------------------------------

void AppInit(HWND hwnd) {
    g_hwnd = hwnd;

    // DPI：先定缩放系数，再建字体（字体尺寸是逻辑像素乘 PX）
    HDC dc = GetDC(hwnd);
    int dpi = GetDeviceCaps(dc, LOGPIXELSY);
    ReleaseDC(hwnd, dc);
    if (dpi <= 0) dpi = 96;
    g_scale = (float)dpi / 96.0f;
    CreateFonts();

    // 数据：先开数据源，再采一轮（CpuUsageRead 第一次只是建差分基线）
    CpuTempInit();
    CpuUsageRead();
    SsdSample();
    SampleAll();

    SYSTEM_INFO si{};
    GetSystemInfo(&si);
    g_nproc = si.dwNumberOfProcessors;

    TooltipCreate(hwnd);
    ApplyMode(false);       // 首帧进入悬浮球形态
    TrayAdd();
    UpdateToolText();

    if (!g_tipShown) {
        TrayBalloon(L"硬件监视球已启动",
                    L"单击切换指标 · 双击展开面板 · 右键菜单 · 可拖拽移动");
        g_tipShown = true;
    }

    SetTimer(hwnd, kTimerSample, (UINT)kSampleMs, nullptr);
    SaveCfg();
}

void AppShutdown() {
    CpuTempClose();
    TrayDelete();
    ReleaseBackBuffer();
    DestroyFonts();
    SaveCfg();
}
