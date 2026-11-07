// ============================================================================
//  metrics/system_load.h — CPU 占用率与内存占用
//
//  两者都是纯系统查询，不需要驱动：
//    CPU 占用  GetSystemTimes 差分（kernel 时间已包含 idle）
//    内存      GlobalMemoryStatusEx
// ============================================================================
#pragma once

// 读一次 CPU 占用率（0~100）。首次调用只建立差分基线，返回 -1。
double CpuUsageRead();

// 读一次内存占用。任何一项失败时保持调用前的值不变。
void RamRead(double& used_gb, double& total_gb, double& percent);
