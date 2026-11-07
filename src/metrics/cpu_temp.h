// ============================================================================
//  metrics/cpu_temp.h — CPU 温度（PDH 计数器）
//
//  数据源：\Thermal Zone Information(*)\Temperature，单位开尔文。
//  这是 ACPI 热区温度，普通用户权限即可读取，不需要驱动。
//
//  ⚠ 该计数器是**整数开尔文**（分辨率 1 °C），且相邻采样之间会跳 ±1~2 K，
//    这是 EC 固件的量化行为，不是读取错误。实测与 HWiNFO 的 CPU (Tctl/Tdie)
//    相差 −1.1 ~ −0.2 °C（见 docs/ARCHITECTURE.md 的实测记录）。
//
//  ⚠ High Precision Temperature 并不是更高精度：它恒等于 Temperature + 0.2 K，
//    步长同样是 1 K。别指望换它能把分辨率变好。
// ============================================================================
#pragma once

// 打开查询并注册计数器。失败后 CpuTempRead 恒返回 -1。
bool CpuTempInit();

// 读一次，返回摄氏温度；没采到返回 -1。
double CpuTempRead();

// 关闭查询句柄（进程退出时调用即可）。
void CpuTempClose();
