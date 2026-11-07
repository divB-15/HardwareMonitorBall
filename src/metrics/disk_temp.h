// ============================================================================
//  metrics/disk_temp.h — 磁盘温度（IOCTL_STORAGE_QUERY_PROPERTY）
//
//  不需要管理员权限：以"仅查询属性"的意图打开设备（dwDesiredAccess = 0），
//  然后发 StorageDeviceTemperatureProperty(52) / StorageAdapterTemperatureProperty(51)。
//  实测（KIOXIA NVMe + Win11）返回 2 个传感器，读数与 HWiNFO 的"磁盘温度"完全一致。
// ============================================================================
#pragma once

#include <string>

struct DiskProbe {
    std::wstring model;
    int          busType = -1;
    double       tempC   = -1.0;
    int          sensors = 0;
};

// 探测单个物理盘（index = 0..N），失败时 tempC < 0。
DiskProbe ProbeDisk(int index);

struct DiskReading {
    int          drive = -1;    // -1 = 本轮没找到能上报温度的盘
    double       tempC = -1.0;
    std::wstring model;
};

// 读一次磁盘温度。
//  cached_drive >= 0 时只探那一块（命中就返回），失败则退回全盘扫描；
//  cached_drive < 0 时扫描 PhysicalDrive0..5，取第一个能上报温度的。
DiskReading ReadDiskTemperature(int cached_drive);
