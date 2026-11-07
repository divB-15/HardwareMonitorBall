// ============================================================================
//  app/app.h — 应用级常量、指标定义与全局状态
//
//  全局变量只在 app/app_core.cpp 里定义一次，其余翻译单元 include 本头文件
//  拿到 extern 声明。这样"谁拥有状态"是明确的：状态属于 app 层。
// ============================================================================
#pragma once

#include <windows.h>

#include <string>

// --- 身份 -------------------------------------------------------------------
inline constexpr wchar_t kAppTitle[]   = L"硬件监视球";
inline constexpr wchar_t kAppVersion[] = L"v1.1.0.0";   // 与 src/app/app.rc 的 FILEVERSION 一致
inline constexpr wchar_t kWndClass[]   = L"HWMonBallClass";
inline constexpr wchar_t kMutexName[]  = L"Local\\HWMonBallSingleton";
inline constexpr wchar_t kRegKey[]     = L"Software\\HWMonBall";

// --- 布局（逻辑像素，最终都要过 theme::PX 换算）------------------------------
inline constexpr int kBallD   = 72;    // 悬浮球直径
inline constexpr int kPanelW  = 258;   // 展开面板
inline constexpr int kPanelH  = 291;
inline constexpr int kTitleH  = 57;    // 面板标题栏高度
inline constexpr int kMargin  = 15;    // 面板外边距
inline constexpr int kCardGap = 12;    // 卡片间距
inline constexpr int kCardH   = 96;    // 卡片高度

// --- 时序 -------------------------------------------------------------------
inline constexpr UINT_PTR kTimerSample = 1;
inline constexpr int      kSampleMs    = 1000;   // 采样周期
inline constexpr int      kSsdEvery    = 5;      // 磁盘温度每 5 个周期探一次（IOCTL 慢）
inline constexpr int      kDragSlop    = 4;      // 位移小于该值视为点击而非拖拽

inline constexpr UINT kWmTray = WM_APP + 1;

// --- 指标 -------------------------------------------------------------------
// 菜单 ID 用 IDM_METRIC_BASE + 编号，省一张映射表。
enum {
    M_CPU_TEMP = 0,
    M_SSD_TEMP = 1,
    M_RAM      = 2,
    M_CPU_USE  = 3,
    M_COUNT    = 4,
};

struct MetricDef {
    const wchar_t* name;   // 面板卡片 / 菜单 / 提示泡里的全名
    const wchar_t* tag;    // 球内小字（空间有限）
    const wchar_t* unit;   // 单位
};

inline constexpr MetricDef kMetricDef[M_COUNT] = {
    { L"CPU 温度", L"CPU", L"°C" },
    { L"SSD 温度", L"SSD", L"°C" },
    { L"内存占用", L"RAM", L"%"  },
    { L"CPU 占用", L"CPU", L"%"  },
};

enum {
    IDM_METRIC_BASE = 1000,   // + 指标编号
    IDM_EXPAND = 1020,        // 展开 / 收起
    IDM_TOPMOST,              // 置顶开关
    IDM_HIDE,                 // 隐藏到托盘
    IDM_SHOW,                 // 从托盘唤回
    IDM_EXIT,
};

// --- 采样结果 ---------------------------------------------------------------
struct Metrics {
    double cpuTempC   = -1.0;   // <0 = 没采到
    double ssdTempC   = -1.0;
    double cpuUsage   = -1.0;
    double ramUsedGB  = 0.0;
    double ramTotalGB = 0.0;
    double ramPct     = -1.0;
};

// --- 全局状态（定义在 app_core.cpp）-----------------------------------------
extern Metrics g_m;

extern bool g_panel;                 // false = 悬浮球，true = 展开面板
extern int  g_metric;                // 球当前显示的指标
extern bool g_topmost;
extern bool g_hidden;
extern bool g_hover;                 // 鼠标悬停在球上
extern bool g_pressed;               // 球被按下
extern bool g_btnHover[2];           // 面板按钮悬停：0 = 收起，1 = 关闭
extern bool g_tipShown;              // 首次运行的气泡提示是否已弹过
extern bool g_panelPosValid;         // g_panelPos 是否被真正定位过（防注册表默认值冒充）

extern HWND g_hwnd;
extern HWND g_tip;                   // 悬浮球提示泡（tooltip）
extern std::wstring g_tipText;

extern RECT g_ballPos;               // 悬浮球窗口矩形（屏幕坐标）
extern RECT g_panelPos;              // 展开面板窗口矩形（屏幕坐标）
extern RECT g_rcBtnCollapse;         // 面板按钮（客户区坐标）
extern RECT g_rcBtnExit;

extern UINT g_nproc;                 // 逻辑核心数
extern std::wstring g_ssdModel;
extern int  g_ssdDrive;              // 缓存有温度上报的盘号，-1 = 还没找到

// 拖拽状态（WM_LBUTTONDOWN 起，WM_LBUTTONUP 止）
extern bool  g_captured;
extern bool  g_moved;
extern POINT g_dragStart;            // 按下时的客户区坐标
extern POINT g_dragStartScreen;      // 按下时的屏幕坐标（拖拽必须用它）
extern RECT  g_dragWin;              // 按下时的窗口矩形

// --- 生命周期（app_core.cpp）------------------------------------------------
// 建字体、初始化数据源、装托盘与提示泡、进入悬浮球形态。
void AppInit(HWND hwnd);

// 逆序收尾：定时器由调用方先停。
void AppShutdown();

// --- 行为（window.cpp）------------------------------------------------------
// 切换悬浮球 / 展开面板（含窗口尺寸、异形区域、按钮矩形）
void ApplyMode(bool panel);

void HideApp();
void ShowApp();
void SetMetric(int m);      // 同时刷新提示泡文本并持久化
void CycleMetric();         // 循环到下一个指标
void OnCommand(UINT id);

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
