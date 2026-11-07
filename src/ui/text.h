// ============================================================================
//  ui/text.h — 字体与文本输出
//
//  球面字号小、面板字号大，共 9 个字重/尺寸组合。字体按 DPI 重建：
//  WM_DPICHANGED 时 DestroyFonts + CreateFonts。
// ============================================================================
#pragma once

#include <windows.h>

#include <string>

struct Fonts {
    HFONT title     = nullptr;   // 面板标题
    HFONT ball_big  = nullptr;   // 球内大数值
    HFONT ball_unit = nullptr;   // 球内单位
    HFONT tag       = nullptr;   // 球内指标小字
    HFONT label     = nullptr;   // 卡片标题
    HFONT big       = nullptr;   // 卡片大数值
    HFONT unit      = nullptr;   // 卡片单位
    HFONT sub       = nullptr;   // 卡片副文本
    HFONT btn       = nullptr;   // 面板按钮
};

extern Fonts g_fonts;

void CreateFonts();
void DestroyFonts();

// 左上角定位
void Txt(HDC dc, int x, int y, const std::wstring& s, COLORREF color, HFONT font);

// 以 cx 水平居中
void TxtC(HDC dc, int cx, int y, const std::wstring& s, COLORREF color, HFONT font);

// 矩形内水平 + 垂直居中（按钮字形）
void TxtBox(HDC dc, const RECT& r, const std::wstring& s, COLORREF color, HFONT font);

// 数值 + 单位整体居中，单位与数值底线对齐（数值大字，单位小字）
void DrawValueUnit(HDC dc, int cx, int cy, const std::wstring& value, const std::wstring& unit,
                   HFONT font_big, HFONT font_unit, COLORREF value_color, COLORREF unit_color);
