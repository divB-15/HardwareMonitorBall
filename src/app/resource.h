#pragma once

// 资源 ID。app.rc 里的图标和版本信息都用这里的定义。
//
// 只定义**一个**图标 ID：同一个 .ico 里带了 16~256 多个尺寸，
// LoadIcon / LoadImage 会按系统要的尺寸自己挑，所以大图标和小图标共用一份资源。
// 分成两组会让 exe 里出现两个 RT_GROUP_ICON 组，而资源管理器只认 ID 最小的那组。
#define IDI_APP_ICON 1
