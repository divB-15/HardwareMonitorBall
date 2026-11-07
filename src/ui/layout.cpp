#include "layout.h"

#include "app.h"
#include "theme.h"

int BallD()  { return PX((float)kBallD); }
int PanelW() { return PX((float)kPanelW); }
int PanelH() { return PX((float)kPanelH); }

void PanelCardRect(int index, RECT& out) {
    const int col = index % 2;
    const int row = index / 2;
    const int cw = (PanelW() - PX((float)kMargin) * 2 - PX((float)kCardGap)) / 2;
    const int x0 = PX((float)kMargin) + col * (cw + PX((float)kCardGap));
    const int y0 = PX((float)kTitleH) + PX((float)kMargin) +
                   row * (PX((float)kCardH) + PX((float)kCardGap));
    out = { x0, y0, x0 + cw, y0 + PX((float)kCardH) };
}

void PanelButtonRects(RECT& collapse, RECT& exit) {
    const int bw = PX(39);
    const int bh = PX(33);
    const int y  = (PX((float)kTitleH) - bh) / 2;
    const int xr = PanelW() - PX((float)kMargin);
    exit     = { xr - bw, y, xr, y + bh };
    collapse = { exit.left - PX(6) - bw, y, exit.left - PX(6), y + bh };
}

void ClampToWorkArea(RECT& r) {
    const int w = r.right - r.left;
    const int h = r.bottom - r.top;
    if (w <= 0 || h <= 0) return;

    HMONITOR monitor = MonitorFromRect(&r, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(monitor, &mi)) {
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &mi.rcWork, 0);
    }
    const RECT& wa = mi.rcWork;
    if (r.left < wa.left) {
        r.left = wa.left;
    } else if (r.right > wa.right) {
        r.left = wa.right - w;
    }
    if (r.top < wa.top) {
        r.top = wa.top;
    } else if (r.bottom > wa.bottom) {
        r.top = wa.bottom - h;
    }
    r.right  = r.left + w;
    r.bottom = r.top + h;
}

void InitPanelPos() {
    if (g_panelPosValid) {
        ClampToWorkArea(g_panelPos);
        return;
    }

    const int pw = PanelW();
    const int ph = PanelH();
    const int bd = BallD();

    RECT wa{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);

    int x;
    int y = g_ballPos.top;
    if (g_ballPos.left + bd + 8 + pw <= wa.right) {
        x = g_ballPos.left + bd + 8;            // 优先右侧
    } else if (g_ballPos.left - 8 - pw >= wa.left) {
        x = g_ballPos.left - 8 - pw;            // 右边放不下就翻到左侧
    } else {
        x = wa.left + (wa.right - wa.left - pw) / 2;   // 都放不下就居中
    }
    if (y + ph > wa.bottom) y = wa.bottom - ph;
    if (y < wa.top) y = wa.top;

    g_panelPos      = { x, y, x + pw, y + ph };
    g_panelPosValid = true;
}

void ApplyRegion() {
    const int w = g_panel ? PanelW() : BallD();
    const int h = g_panel ? PanelH() : BallD();
    HRGN rgn = g_panel ? CreateRoundRectRgn(0, 0, w, h, PX(16), PX(16))
                       : CreateEllipticRgn(0, 0, w, h);
    // SetWindowRgn 成功后系统接管该区域，切勿再 DeleteObject
    SetWindowRgn(g_hwnd, rgn, TRUE);
}
