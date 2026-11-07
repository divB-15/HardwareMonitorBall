#include "gfx.h"

#include <cmath>
#include <vector>

void FillRound(HDC dc, RECT r, int radius, COLORREF fill) {
    HBRUSH  brush = CreateSolidBrush(fill);
    HPEN    pen   = CreatePen(PS_SOLID, 1, fill);
    HGDIOBJ old_brush = SelectObject(dc, brush);
    HGDIOBJ old_pen   = SelectObject(dc, pen);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, radius, radius);
    SelectObject(dc, old_brush);
    SelectObject(dc, old_pen);
    DeleteObject(brush);
    DeleteObject(pen);
}

void StrokeRound(HDC dc, RECT r, int radius, COLORREF stroke) {
    HBRUSH  brush = (HBRUSH)GetStockObject(NULL_BRUSH);
    HPEN    pen   = CreatePen(PS_SOLID, 1, stroke);
    HGDIOBJ old_brush = SelectObject(dc, brush);
    HGDIOBJ old_pen   = SelectObject(dc, pen);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, radius, radius);
    SelectObject(dc, old_brush);
    SelectObject(dc, old_pen);
    DeleteObject(pen);
}

void FillCircle(HDC dc, int cx, int cy, int radius, COLORREF color) {
    HBRUSH  brush = CreateSolidBrush(color);
    HGDIOBJ old_brush = SelectObject(dc, brush);
    HGDIOBJ old_pen   = SelectObject(dc, GetStockObject(NULL_PEN));
    Ellipse(dc, cx - radius, cy - radius, cx + radius, cy + radius);
    SelectObject(dc, old_brush);
    SelectObject(dc, old_pen);
    DeleteObject(brush);
}

void StrokeCircle(HDC dc, int cx, int cy, int radius, COLORREF color, int width) {
    HPEN    pen = CreatePen(PS_SOLID, width, color);
    HGDIOBJ old_brush = SelectObject(dc, GetStockObject(NULL_BRUSH));
    HGDIOBJ old_pen   = SelectObject(dc, pen);
    Ellipse(dc, cx - radius, cy - radius, cx + radius, cy + radius);
    SelectObject(dc, old_brush);
    SelectObject(dc, old_pen);
    DeleteObject(pen);
}

void DrawArc(HDC dc, int cx, int cy, int radius, double ratio, COLORREF color, int width) {
    if (ratio <= 0.0005 || radius <= 0) return;
    if (ratio > 1.0) ratio = 1.0;

    // 每 4° 取一个点，小圆也够平滑
    const double kDegToRad = 3.14159265358979 / 180.0;
    const double a0 = -90.0;
    const double a1 = a0 + ratio * 360.0;
    const int    steps = (int)(ratio * 360.0 / 4) + 1;

    std::vector<POINT> pts((size_t)steps + 1);
    for (int i = 0; i <= steps; ++i) {
        double a = (a0 + (a1 - a0) * i / steps) * kDegToRad;
        pts[(size_t)i] = { (int)(cx + radius * std::cos(a) + 0.5),
                           (int)(cy + radius * std::sin(a) + 0.5) };
    }

    HPEN    pen = CreatePen(PS_SOLID, width, color);
    HGDIOBJ old = SelectObject(dc, pen);
    Polyline(dc, pts.data(), (int)pts.size());
    SelectObject(dc, old);
    DeleteObject(pen);
}
