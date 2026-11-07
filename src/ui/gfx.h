// ============================================================================
//  ui/gfx.h — GDI 图形基元
//
//  全部走"临时创建 → 选入 → 用完选回并删除"的写法，不缓存 GDI 对象，
//  避免句柄泄漏和跨绘制状态的隐式依赖。
// ============================================================================
#pragma once

#include <windows.h>

// 实心圆角矩形 / 圆角描边
void FillRound(HDC dc, RECT r, int radius, COLORREF fill);
void StrokeRound(HDC dc, RECT r, int radius, COLORREF stroke);

// 实心圆 / 圆环描边（描边笔宽 w 骑在半径上）
void FillCircle(HDC dc, int cx, int cy, int radius, COLORREF color);
void StrokeCircle(HDC dc, int cx, int cy, int radius, COLORREF color, int width);

// 从 12 点方向起顺时针扫 ratio*360° 的圆弧（GDI 角度从 +x 轴起算，故起点 -90°）
void DrawArc(HDC dc, int cx, int cy, int radius, double ratio, COLORREF color, int width);
