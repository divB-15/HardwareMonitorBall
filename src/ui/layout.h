// ============================================================================
//  ui/layout.h — 布局几何
//
//  app.h 里的尺寸常量是**逻辑像素**（96 dpi 下的设计值），这里统一乘 PX()
//  换算成当前 DPI 的物理像素。除本文件和 theme::PX 之外，别处不要自己乘 dpi。
// ============================================================================
#pragma once

#include <windows.h>

int BallD();    // 悬浮球直径（物理像素）
int PanelW();
int PanelH();

// 面板 2×2 卡片矩形（客户区坐标），index 就是指标编号
void PanelCardRect(int index, RECT& out);

// 面板标题栏右侧两个按钮：收起 / 关闭（客户区坐标）
void PanelButtonRects(RECT& collapse, RECT& exit);

inline bool PtIn(const RECT& r, POINT p) {
    return PtInRect(&r, p) != FALSE;
}

// 把窗口矩形收敛回"最近的显示器"的工作区（拖拽不丢窗口）
void ClampToWorkArea(RECT& r);

// 面板默认开在球旁边：优先右侧，空间不够翻左侧，最后居中兜底。
// g_panelPosValid 为真时只做工作区收敛。
void InitPanelPos();

// 按当前模式给窗口套异形区域（球=椭圆，面板=圆角矩形）
void ApplyRegion();
