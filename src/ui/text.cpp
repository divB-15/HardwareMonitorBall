#include "text.h"

#include "theme.h"

Fonts g_fonts;

namespace {

HFONT MakeFont(int logical_px, int weight, const wchar_t* face) {
    return CreateFontW(-PX((float)logical_px), 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH,
                       face);
}

}  // namespace

void CreateFonts() {
    const wchar_t* yahei = L"Microsoft YaHei UI";   // 数字/标题：雅黑
    const wchar_t* hei   = L"SimHei";               // 小号文字：黑体（更实、更黑）

    g_fonts.title     = MakeFont(20, FW_SEMIBOLD, yahei);   // 面板标题
    g_fonts.ball_big  = MakeFont(21, FW_SEMIBOLD, yahei);   // 球内大数值
    g_fonts.ball_unit = MakeFont(11, FW_NORMAL, yahei);     // 球内单位
    g_fonts.tag       = MakeFont(10, FW_BOLD, hei);         // 球内指标小字
    g_fonts.label     = MakeFont(15, FW_MEDIUM, hei);       // 卡片标题
    g_fonts.big       = MakeFont(33, FW_SEMIBOLD, yahei);   // 卡片大数值
    g_fonts.unit      = MakeFont(16, FW_NORMAL, yahei);     // 卡片单位
    g_fonts.sub       = MakeFont(13, FW_MEDIUM, hei);       // 卡片副文本
    g_fonts.btn       = MakeFont(18, FW_MEDIUM, yahei);     // 面板按钮
}

void DestroyFonts() {
    HFONT all[] = { g_fonts.title, g_fonts.ball_big, g_fonts.ball_unit, g_fonts.tag,
                    g_fonts.label, g_fonts.big,      g_fonts.unit,      g_fonts.sub,
                    g_fonts.btn };
    for (HFONT f : all) {
        if (f) DeleteObject(f);
    }
    g_fonts = Fonts{};
}

void Txt(HDC dc, int x, int y, const std::wstring& s, COLORREF color, HFONT font) {
    if (!font || s.empty()) return;
    HGDIOBJ old = SelectObject(dc, font);
    SetTextColor(dc, color);
    SetBkMode(dc, TRANSPARENT);
    SetTextAlign(dc, TA_LEFT | TA_TOP);
    TextOutW(dc, x, y, s.c_str(), (int)s.size());
    SelectObject(dc, old);
}

void TxtC(HDC dc, int cx, int y, const std::wstring& s, COLORREF color, HFONT font) {
    if (!font || s.empty()) return;
    HGDIOBJ old = SelectObject(dc, font);
    SIZE sz{};
    GetTextExtentPoint32W(dc, s.c_str(), (int)s.size(), &sz);
    SetTextColor(dc, color);
    SetBkMode(dc, TRANSPARENT);
    SetTextAlign(dc, TA_LEFT | TA_TOP);
    TextOutW(dc, cx - sz.cx / 2, y, s.c_str(), (int)s.size());
    SelectObject(dc, old);
}

void TxtBox(HDC dc, const RECT& r, const std::wstring& s, COLORREF color, HFONT font) {
    if (!font || s.empty()) return;
    HGDIOBJ old = SelectObject(dc, font);
    SetTextColor(dc, color);
    SetBkMode(dc, TRANSPARENT);
    RECT box = r;   // DrawTextW 的 lpRect 不是 const
    DrawTextW(dc, s.c_str(), (int)s.size(), &box, DT_CENTER | DT_VCENTER | DT_SINGLELINE |
                                                    DT_NOPREFIX);
    SelectObject(dc, old);
}

void DrawValueUnit(HDC dc, int cx, int cy, const std::wstring& value, const std::wstring& unit,
                   HFONT font_big, HFONT font_unit, COLORREF value_color, COLORREF unit_color) {
    if (value.empty()) return;

    SIZE vsz{}, usz{};
    {
        HGDIOBJ old = SelectObject(dc, font_big);
        GetTextExtentPoint32W(dc, value.c_str(), (int)value.size(), &vsz);
        SelectObject(dc, old);
    }
    if (!unit.empty()) {
        HGDIOBJ old = SelectObject(dc, font_unit);
        GetTextExtentPoint32W(dc, unit.c_str(), (int)unit.size(), &usz);
        SelectObject(dc, old);
    }

    const int gap   = unit.empty() ? 0 : PX(2);
    const int total = vsz.cx + gap + usz.cx;
    const int x     = cx - total / 2;
    const int y     = cy - vsz.cy / 2;

    SetTextColor(dc, value_color);
    SetBkMode(dc, TRANSPARENT);
    SetTextAlign(dc, TA_LEFT | TA_TOP);
    HGDIOBJ old = SelectObject(dc, font_big);
    TextOutW(dc, x, y, value.c_str(), (int)value.size());
    if (!unit.empty()) {
        SelectObject(dc, font_unit);
        SetTextColor(dc, unit_color);
        TextOutW(dc, x + vsz.cx + gap, y + vsz.cy - usz.cy, unit.c_str(), (int)unit.size());
    }
    SelectObject(dc, old);
}
