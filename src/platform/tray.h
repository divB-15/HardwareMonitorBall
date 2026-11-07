// ============================================================================
//  platform/tray.h — 任务栏通知区图标
//
//  用 Shell_NotifyIconW（NIM_ADD / NIM_DELETE / NIM_MODIFY）。
//  图标从 exe 资源取（IDI_APP_ICON），与窗口图标、任务管理器图标同一份。
// ============================================================================
#pragma once

void TrayAdd();
void TrayDelete();

// 气泡提示（首次启动的"怎么用"）
void TrayBalloon(const wchar_t* title, const wchar_t* text);

// 托盘右键菜单（显示/隐藏、展开/收起、置顶、退出）
void ShowTrayMenu();
