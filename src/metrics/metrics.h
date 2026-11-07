// ============================================================================
//  metrics/metrics.h — 采样调度 + 指标呈现（数值 / 文本 / 比例 / 配色）
//
//  这一层把"数据"翻译成"界面要的东西"，但不碰任何 GDI 绘制：
//  ui 层只负责把这里给出的数字和颜色画出来。
// ============================================================================
#pragma once

#include <windows.h>

#include <string>

// 每 1 秒一次的常规采样：CPU 温度、CPU 占用、内存。
void SampleAll();

// 低频采样（每 kSsdEvery 个周期）：磁盘温度走 IOCTL，比较慢。
// 顺带维护"哪块盘能上报温度"的缓存盘号。
void SsdSample();

// 当前指标值；<0 表示没采到。
double MetricValue(int metric);

// 球面 / 卡片上的数值文本（没采到时为 "--"），并回填单位。
std::wstring MetricText(int metric, std::wstring& unit);

// 环形仪表 / 进度条比例，0~1；<0 表示没采到。
double MetricRatio(int metric);

// 强调色（按数值分档）。没采到时返回次要文字色。
COLORREF MetricAccent(int metric);
