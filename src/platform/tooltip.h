// ============================================================================
//  platform/tooltip.h — 悬浮球悬停提示泡
//
//  提示泡由 comctl32 的 TOOLTIPS_CLASS 提供，注册到球窗口的客户区矩形上，
//  文本随指标变化刷新。面板形态 / 隐藏状态下把工具矩形置空，不弹提示。
// ============================================================================
#pragma once

#include <windows.h>

// WM_CREATE 里调用，创建提示泡并挂到 owner 的客户区
void TooltipCreate(HWND owner);

// 指标值变了以后刷新文本（内部做了去重，值没变不重发）
void UpdateToolText();

// 模式切换 / 隐藏后调整工具矩形
void UpdateToolRect();
