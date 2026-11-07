#include "paint.h"

#include "app.h"
#include "gfx.h"
#include "layout.h"
#include "metrics.h"
#include "text.h"
#include "theme.h"
#include "util.h"

namespace {

// --- 双缓冲 -----------------------------------------------------------------
HDC     g_mem_dc  = nullptr;
HBITMAP g_mem_bmp = nullptr;
HGDIOBJ g_mem_old = nullptr;
int     g_mem_w   = 0;
int     g_mem_h   = 0;

HDC GetBackBuffer(HDC ref, int w, int h) {
    if (g_mem_dc && g_mem_w == w && g_mem_h == h) return g_mem_dc;
    ReleaseBackBuffer();
    g_mem_dc  = CreateCompatibleDC(ref);
    g_mem_bmp = CreateCompatibleBitmap(ref, w, h);
    g_mem_old = SelectObject(g_mem_dc, g_mem_bmp);
    g_mem_w = w;
    g_mem_h = h;
    return g_mem_dc;
}

// --- 悬浮球 -----------------------------------------------------------------
void PaintBall(HDC dc, RECT cr) {
    const int w = cr.right;
    const int h = cr.bottom;
    const int cx = w / 2;
    const int cy = h / 2;
    const int radius = (w < h ? w : h) / 2;
    const int press = g_pressed ? PX(1) : 0;   // 按下时整体下沉 1px

    // 球体
    FillCircle(dc, cx, cy + press, radius, g_pal.card);
    StrokeCircle(dc, cx, cy + press, radius - 1,
                 g_hover ? Mix(g_pal.border, g_pal.text2, 0.5) : g_pal.border, 1);

    // 仪表环：底轨 + 当前比例弧
    const int ring = radius - PX(6);
    StrokeCircle(dc, cx, cy + press, ring, g_pal.track, PX(6));
    const double ratio = MetricRatio(g_metric);
    if (ratio > 0) DrawArc(dc, cx, cy + press, ring, ratio, MetricAccent(g_metric), PX(6));

    // 中央数值 + 单位（与下方小字保持 4px 间距，省垂直空间）
    std::wstring unit;
    const std::wstring value = MetricText(g_metric, unit);
    DrawValueUnit(dc, cx, cy - PX(8) + press, value, unit, g_fonts.ball_big,
                  g_fonts.ball_unit, MetricAccent(g_metric), g_pal.text2);

    // 指标小字（黑体 + 主文字色，避免发淡）
    TxtC(dc, cx, cy + PX(10) + press, kMetricDef[g_metric].tag, g_pal.text, g_fonts.tag);
}

// --- 面板卡片 ---------------------------------------------------------------
void PaintCard(HDC dc, const RECT& r, int metric) {
    const COLORREF accent = MetricAccent(metric);
    const bool current = (metric == g_metric);
    const int pad_x = PX(12);

    FillRound(dc, r, PX(12), g_pal.card);
    StrokeRound(dc, r, PX(12), current ? accent : g_pal.border);

    // 指标名；当前默认显示的卡片右上角加点
    Txt(dc, r.left + pad_x, r.top + PX(4), kMetricDef[metric].name, g_pal.text, g_fonts.label);
    if (current) FillCircle(dc, r.right - PX(18), r.top + PX(14), PX(3), accent);

    // 大数值 + 单位
    std::wstring unit;
    const std::wstring value = MetricText(metric, unit);
    DrawValueUnit(dc, (r.left + r.right) / 2, r.top + PX(48), value, unit, g_fonts.big,
                  g_fonts.unit, accent, g_pal.text2);

    // 副文本（同指标的补充口径）
    std::wstring sub;
    if (metric == M_RAM) {
        sub = Fmt(L"%.1f / %.1f GB", g_m.ramUsedGB, g_m.ramTotalGB);
    } else if (metric == M_CPU_USE) {
        sub = Fmt(L"%u 逻辑核心", g_nproc);
    }
    if (!sub.empty()) {
        TxtC(dc, (r.left + r.right) / 2, r.top + PX(72), sub, g_pal.text, g_fonts.sub);
    }

    // 进度条
    const double ratio = MetricRatio(metric);
    const int by = r.bottom - PX(6);
    RECT track{ r.left + pad_x, by, r.right - pad_x, by + PX(4) };
    FillRound(dc, track, PX(2), g_pal.track);
    if (ratio > 0) {
        const int fill_w = (int)((track.right - track.left) * ratio);
        if (fill_w > 2) {
            RECT fill{ track.left, track.top, track.left + fill_w, track.bottom };
            FillRound(dc, fill, PX(2), accent);
        }
    }
}

// --- 展开面板 ---------------------------------------------------------------
void PaintPanel(HDC dc, RECT cr) {
    HBRUSH bg = CreateSolidBrush(g_pal.bg);
    FillRect(dc, &cr, bg);
    DeleteObject(bg);
    StrokeRound(dc, cr, PX(24), g_pal.border);

    Txt(dc, PX(20), PX(16), kAppTitle, g_pal.text, g_fonts.title);

    // 标题栏两个按钮（悬停高亮）
    for (int i = 0; i < 2; ++i) {
        const RECT& rc = (i == 0) ? g_rcBtnCollapse : g_rcBtnExit;
        if (g_btnHover[i]) FillRound(dc, rc, PX(6), Mix(g_pal.card, g_pal.text, 0.10));
        TxtBox(dc, rc, (i == 0) ? L"—" : L"×", g_pal.text2, g_fonts.btn);
    }

    for (int i = 0; i < M_COUNT; ++i) {
        RECT r;
        PanelCardRect(i, r);
        PaintCard(dc, r, i);
    }
}

}  // namespace

void ReleaseBackBuffer() {
    if (!g_mem_dc) return;
    SelectObject(g_mem_dc, g_mem_old);
    DeleteObject(g_mem_bmp);
    DeleteDC(g_mem_dc);
    g_mem_dc = nullptr;
    g_mem_bmp = nullptr;
    g_mem_old = nullptr;
    g_mem_w = 0;
    g_mem_h = 0;
}

void PaintAll(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hwnd, &ps);
    RECT cr;
    GetClientRect(hwnd, &cr);

    HDC mem = GetBackBuffer(dc, cr.right, cr.bottom);
    if (g_panel) {
        PaintPanel(mem, cr);
    } else {
        PaintBall(mem, cr);
    }
    BitBlt(dc, 0, 0, cr.right, cr.bottom, mem, 0, 0, SRCCOPY);
    EndPaint(hwnd, &ps);
}
