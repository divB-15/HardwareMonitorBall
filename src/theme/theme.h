// ============================================================================
//  theme/theme.h — 调色板、DPI 缩放与颜色运算
//
//  g_scale 是"物理像素 / 逻辑像素"的换算系数，PX() 是唯一的尺寸换算入口。
//  所有布局常量都写成逻辑像素（96 dpi 下的值），乘过 PX 才能用。
// ============================================================================
#pragma once

#include <windows.h>

struct Palette {
    COLORREF bg, card, border, text, text2, track;
    COLORREF red, blue, green, amber, teal, violet, cyan;
};

extern Palette g_pal;

// DPI 缩放（物理像素 / 逻辑像素）。
extern float g_scale;

// 整体缩小系数：布局按 96 dpi 设计，实际观感偏大，统一乘 0.9。
extern float g_uiScale;

// 逻辑像素 → 物理像素。浮点相乘，避免 175% 被取整成 2.0 倍。
int PX(float v);

// 跟随系统明暗主题（读 HKCU\...\Themes\Personalize\AppsUseLightTheme）
void LoadSystemPalette();

// 直接指定明暗（测试 / 强制）
void LoadPalette(bool dark);

// 线性插值：t=0 取 a，t=1 取 b（自动钳制 t）
COLORREF Mix(COLORREF a, COLORREF b, double t);
