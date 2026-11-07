#include "tooltip.h"

#include <windows.h>
#include <commctrl.h>

#include "app.h"
#include "metrics.h"
#include "util.h"

void TooltipCreate(HWND owner) {
    INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_BAR_CLASSES };
    InitCommonControlsEx(&icc);

    g_tip = CreateWindowExW(0, TOOLTIPS_CLASS, nullptr, WS_POPUP | TTS_ALWAYSTIP, 0, 0, 0, 0,
                            owner, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!g_tip) return;

    TOOLINFOW ti{};
    ti.cbSize  = sizeof(ti);
    ti.hwnd    = owner;
    ti.uId     = (UINT_PTR)owner;
    ti.uFlags  = TTF_SUBCLASS;
    GetClientRect(owner, &ti.rect);
    ti.lpszText = (LPWSTR)g_tipText.c_str();
    SendMessageW(g_tip, TTM_ADDTOOLW, 0, (LPARAM)&ti);
}

void UpdateToolText() {
    std::wstring unit;
    const std::wstring value = MetricText(g_metric, unit);
    std::wstring text = Fmt(L"%ls：%ls %ls", kMetricDef[g_metric].name, value.c_str(), unit.c_str());
    if (text == g_tipText) return;      // 值没变就不打扰 tooltip
    g_tipText = text;
    if (!g_tip) return;

    TOOLINFOW ti{};
    ti.cbSize   = sizeof(ti);
    ti.hwnd     = g_hwnd;
    ti.uId      = (UINT_PTR)g_hwnd;
    ti.lpszText = (LPWSTR)g_tipText.c_str();
    SendMessageW(g_tip, TTM_UPDATETIPTEXTW, 0, (LPARAM)&ti);
}

void UpdateToolRect() {
    if (!g_tip) return;
    TOOLINFOW ti{};
    ti.cbSize = sizeof(ti);
    ti.hwnd   = g_hwnd;
    ti.uId    = (UINT_PTR)g_hwnd;
    ti.rect   = RECT{};
    // 面板与隐藏状态下矩形留空 → 鼠标不在"球"上，不弹提示
    if (!g_panel && !g_hidden) GetClientRect(g_hwnd, &ti.rect);
    SendMessageW(g_tip, TTM_NEWTOOLRECT, 0, (LPARAM)&ti);
}
