// ============================================================================
//  app/window.cpp — 窗口过程与交互
//
//  交互模型（全部走客户区消息，异形与"球外点击穿透"交给窗口区域处理）：
//    左键单击  切换指标        左键双击  展开 / 收起面板
//    滚轮      切换指标        右键      上下文菜单
//    拖拽      移动窗口（位移超过 kDragSlop 才算拖拽）
//    托盘      双击显隐，右键菜单
// ============================================================================
#include "app.h"

#include "config.h"
#include "layout.h"
#include "menu.h"
#include "metrics.h"
#include "paint.h"
#include "text.h"
#include "tooltip.h"
#include "tray.h"
#include "theme.h"

void ApplyMode(bool panel) {
    g_panel = panel;
    if (panel) InitPanelPos();

    const int w = panel ? PanelW() : BallD();
    const int h = panel ? PanelH() : BallD();
    RECT& pos = panel ? g_panelPos : g_ballPos;

    // 面板从未定位过时给一个兜底位置（球位置在入口里已经算好）
    if (pos.right - pos.left <= 0) {
        RECT wa{};
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
        const int x = wa.right - w - 64;
        const int y = wa.top + 180;
        ClampToWorkArea(pos = { x, y, x + w, y + h });
    }

    SetWindowPos(g_hwnd, nullptr, pos.left, pos.top, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
    ApplyRegion();

    if (panel) {
        PanelButtonRects(g_rcBtnCollapse, g_rcBtnExit);
    } else {
        g_rcBtnCollapse = RECT{};
        g_rcBtnExit     = RECT{};
    }

    UpdateToolRect();
    InvalidateRect(g_hwnd, nullptr, FALSE);
}

void HideApp() {
    g_hidden = true;
    ShowWindow(g_hwnd, SW_HIDE);
    SaveCfg();
}

void ShowApp() {
    g_hidden = false;
    ShowWindow(g_hwnd, SW_SHOW);
    SaveCfg();
}

void SetMetric(int metric) {
    g_metric = metric;
    UpdateToolText();
    SaveCfg();
    InvalidateRect(g_hwnd, nullptr, FALSE);
}

void CycleMetric() {
    SetMetric((g_metric + 1) % M_COUNT);
}

void OnCommand(UINT id) {
    if (id >= (UINT)IDM_METRIC_BASE && id < (UINT)(IDM_METRIC_BASE + M_COUNT)) {
        SetMetric((int)(id - IDM_METRIC_BASE));
        return;
    }

    switch (id) {
    case IDM_EXPAND:
        if (g_hidden) ShowApp();
        ApplyMode(!g_panel);
        SaveCfg();
        break;
    case IDM_TOPMOST:
        g_topmost = !g_topmost;
        SetWindowPos(g_hwnd, g_topmost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        SaveCfg();
        break;
    case IDM_HIDE:
        HideApp();
        break;
    case IDM_SHOW:
        ShowApp();
        break;
    case IDM_EXIT:
        DestroyWindow(g_hwnd);
        break;
    }
}

namespace {

// 客户区坐标 -> POINT（lParam 里是 16 位有符号）
POINT ClientPoint(LPARAM lp) {
    return POINT{ (int)(short)LOWORD(lp), (int)(short)HIWORD(lp) };
}

// 按下后是否已经超过拖拽阈值
bool ExceededSlop(int dx, int dy) {
    return (dx < 0 ? -dx : dx) > kDragSlop || (dy < 0 ? -dy : dy) > kDragSlop;
}

}  // namespace

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        AppInit(hwnd);
        return 0;

    case WM_TIMER:
        if (wp == kTimerSample) {
            static int tick = 0;
            if (++tick % kSsdEvery == 0) SsdSample();
            SampleAll();
            UpdateToolText();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_PAINT:
        PaintAll(hwnd);
        return 0;

    case WM_ERASEBKGND:
        return 1;                    // 全部自己画，别让系统刷底色（会闪）

    case WM_SIZE:
        ReleaseBackBuffer();         // 客户区尺寸变了，双缓冲缓存作废
        return 0;

    case WM_MOVE:
        // 拖拽 / 程序化移动都经此更新当前模式的窗口矩形
        GetWindowRect(hwnd, g_panel ? &g_panelPos : &g_ballPos);
        return 0;

    case WM_EXITSIZEMOVE: {
        RECT& pos = g_panel ? g_panelPos : g_ballPos;
        const RECT before = pos;
        GetWindowRect(hwnd, &pos);
        ClampToWorkArea(pos);
        if (pos.left != before.left || pos.top != before.top) {
            SetWindowPos(hwnd, nullptr, pos.left, pos.top, 0, 0,
                         SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        SaveCfg();
        return 0;
    }

    case WM_DPICHANGED: {
        g_scale = (float)HIWORD(wp) / 96.0f;
        DestroyFonts();
        CreateFonts();
        const RECT* rc = (const RECT*)lp;
        SetWindowPos(hwnd, nullptr, rc->left, rc->top, rc->right - rc->left,
                     rc->bottom - rc->top, SWP_NOZORDER | SWP_NOACTIVATE);
        GetWindowRect(hwnd, g_panel ? &g_panelPos : &g_ballPos);
        ApplyRegion();
        if (g_panel) PanelButtonRects(g_rcBtnCollapse, g_rcBtnExit);
        UpdateToolRect();
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    // ---- 鼠标 ----
    case WM_LBUTTONDOWN: {
        // g_captured 必须最先置位：WM_MOUSEMOVE / WM_LBUTTONUP 都靠它判定拖拽会话
        g_captured = true;
        SetCapture(hwnd);
        g_moved     = false;
        g_dragStart = ClientPoint(lp);
        GetCursorPos(&g_dragStartScreen);
        GetWindowRect(hwnd, &g_dragWin);
        if (!g_panel) {
            g_pressed = true;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        const POINT pt = ClientPoint(lp);
        if (g_captured) {
            if (!g_moved && ExceededSlop(pt.x - g_dragStart.x, pt.y - g_dragStart.y)) {
                g_moved = true;
            }
            if (g_moved) {
                // 用屏幕坐标差推算窗口新位置：窗口跟着光标移动会改变客户区坐标，
                // 若用客户区坐标差会形成正反馈，窗口越拖越快飞走
                POINT sp;
                GetCursorPos(&sp);
                const int nx = g_dragWin.left + (sp.x - g_dragStartScreen.x);
                const int ny = g_dragWin.top + (sp.y - g_dragStartScreen.y);
                RECT r{ nx, ny, nx + (g_dragWin.right - g_dragWin.left),
                        ny + (g_dragWin.bottom - g_dragWin.top) };
                ClampToWorkArea(r);
                SetWindowPos(hwnd, nullptr, r.left, r.top, 0, 0,
                             SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
        } else {
            bool need_paint = false;
            if (g_panel) {
                for (int i = 0; i < 2; ++i) {
                    const bool hover = PtIn((i == 0) ? g_rcBtnCollapse : g_rcBtnExit, pt);
                    if (hover != g_btnHover[i]) {
                        g_btnHover[i] = hover;
                        need_paint    = true;
                    }
                }
            } else if (!g_hover) {
                g_hover    = true;
                need_paint = true;
            }
            if (need_paint) InvalidateRect(hwnd, nullptr, FALSE);

            TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, hwnd, 0 };
            TrackMouseEvent(&tme);
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        if (!g_captured) return 0;
        ReleaseCapture();
        g_captured = false;
        const bool was_moved = g_moved;
        g_moved = false;

        if (g_pressed) {
            g_pressed = false;
            InvalidateRect(hwnd, nullptr, FALSE);
        }

        if (was_moved) {
            SaveCfg();               // 拖拽结束，记下新位置
            return 0;
        }

        const POINT pt = ClientPoint(lp);
        if (g_panel) {
            if (PtIn(g_rcBtnCollapse, pt)) {
                ApplyMode(false);
            } else if (PtIn(g_rcBtnExit, pt)) {
                HideApp();
            } else {
                for (int i = 0; i < M_COUNT; ++i) {
                    RECT r;
                    PanelCardRect(i, r);
                    if (PtIn(r, pt)) {   // 单击卡片：设为球的默认显示
                        SetMetric(i);
                        break;
                    }
                }
                InvalidateRect(g_hwnd, nullptr, FALSE);
            }
        } else {
            CycleMetric();               // 单击球：循环切换指标
        }
        return 0;
    }

    case WM_LBUTTONDBLCLK:
        if (g_panel) {
            const POINT pt = ClientPoint(lp);
            if (PtIn({ 0, 0, PanelW(), PX((float)kTitleH) }, pt)) ApplyMode(false);
        } else {
            // 双击展开。第一击已经切过一次指标，这里退回去，保持所见即所得
            SetMetric((g_metric + M_COUNT - 1) % M_COUNT);
            ApplyMode(true);
        }
        return 0;

    case WM_RBUTTONUP:
        if (!g_captured) ShowContextMenu();
        return 0;

    case WM_MOUSEWHEEL:
        if (!g_panel) {
            const int delta = (int)(short)HIWORD(wp);   // GET_WHEEL_DELTA_WPARAM
            SetMetric((g_metric + (delta > 0 ? 1 : M_COUNT - 1) + M_COUNT) % M_COUNT);
        }
        return 0;

    case WM_MOUSELEAVE:
        g_hover       = false;
        g_btnHover[0] = g_btnHover[1] = false;
        if (!g_panel) InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_SETCURSOR:
        if (LOWORD(lp) == HTCLIENT && HIWORD(lp) == WM_MOUSEMOVE) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hwnd, &pt);
            if (g_panel && (PtIn(g_rcBtnCollapse, pt) || PtIn(g_rcBtnExit, pt))) {
                SetCursor(LoadCursorW(nullptr, IDC_HAND));
                return TRUE;
            }
        }
        break;

    case WM_KEYDOWN:
        if (wp == VK_ESCAPE) {
            if (g_panel) ApplyMode(false);
            else         HideApp();
        }
        return 0;

    case WM_COMMAND:
        OnCommand(LOWORD(wp));
        return 0;

    case kWmTray:
        if (LOWORD(lp) == WM_RBUTTONUP) {
            ShowTrayMenu();
        } else if (LOWORD(lp) == WM_LBUTTONDBLCLK) {
            OnCommand(g_hidden ? IDM_SHOW : IDM_HIDE);
        }
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, kTimerSample);
        AppShutdown();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
