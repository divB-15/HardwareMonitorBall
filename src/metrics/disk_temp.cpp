#include "disk_temp.h"

#include <windows.h>
#include <winioctl.h>

#include <vector>

#include "util.h"

namespace {

// MinGW 的 winioctl.h 未必定义这两个属性 ID，按 Win10 1809+ SDK 补上
#ifndef StorageAdapterTemperatureProperty
#define StorageAdapterTemperatureProperty ((STORAGE_PROPERTY_ID)51)
#endif
#ifndef StorageDeviceTemperatureProperty
#define StorageDeviceTemperatureProperty ((STORAGE_PROPERTY_ID)52)
#endif

// 实测布局（KIOXIA NVMe, Win11）：header 24 字节，之后每项 16 字节
// item: Index(2) Temperature(2) OverTemperature(2) Threshold(2) Max(4) Min(4)
#pragma pack(push, 1)
struct TempInfoRaw {
    USHORT Index;
    SHORT  Temperature;
    SHORT  OverTemperature;
    USHORT Threshold;
    ULONG  MaxTemperature;
    ULONG  MinTemperature;
};

struct TempDescRaw {
    ULONG      Version;
    ULONG      Size;
    USHORT     DeprecatedTemperature;
    UCHAR      CriticalTemperature;
    UCHAR      WarningTemperature;
    USHORT     InfoCount;
    UCHAR      Reserved0[2];
    ULONG      Reserved1[2];       // 8 字节 → header 共 24 字节
    TempInfoRaw Info[1];
};
#pragma pack(pop)

static_assert(sizeof(TempInfoRaw) == 16, "TempInfoRaw must be 16 bytes");

// 最多扫这么多块盘就放弃（本机只有 PhysicalDrive0）
constexpr int kMaxDisks = 6;

}  // namespace

DiskProbe ProbeDisk(int index) {
    DiskProbe out;
    std::wstring dev = Fmt(L"\\\\.\\PhysicalDrive%d", index);

    // 只申请"查询属性"权限；GENERIC_READ 会被拒绝（错误 5）
    HANDLE h = CreateFileW(dev.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                           OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return out;

    STORAGE_PROPERTY_QUERY q{};
    q.QueryType = PropertyStandardQuery;
    DWORD ret = 0;

    // --- 型号 / 总线 ---
    {
        q.PropertyId = StorageDeviceProperty;
        std::vector<BYTE> buf(4096);
        if (DeviceIoControl(h, IOCTL_STORAGE_QUERY_PROPERTY, &q, sizeof(q), buf.data(),
                            (DWORD)buf.size(), &ret, nullptr) &&
            ret >= sizeof(STORAGE_DEVICE_DESCRIPTOR)) {
            STORAGE_DEVICE_DESCRIPTOR* d = (STORAGE_DEVICE_DESCRIPTOR*)buf.data();
            std::wstring vendor, product;
            // 偏移为 0 表示字段不存在，不能读
            if (d->VendorIdOffset) {
                vendor = TrimW(AnsiToWide((const char*)buf.data() + d->VendorIdOffset));
            }
            if (d->ProductIdOffset) {
                product = TrimW(AnsiToWide((const char*)buf.data() + d->ProductIdOffset));
            }
            out.model   = TrimW(vendor + L" " + product);
            out.busType = (int)d->BusType;
        }
    }

    // --- 温度 ---
    for (STORAGE_PROPERTY_ID pid : { StorageDeviceTemperatureProperty,
                                     StorageAdapterTemperatureProperty }) {
        q.PropertyId = pid;
        std::vector<BYTE> tb(512, 0);
        DWORD r2 = 0;
        if (DeviceIoControl(h, IOCTL_STORAGE_QUERY_PROPERTY, &q, sizeof(q), tb.data(),
                            (DWORD)tb.size(), &r2, nullptr) &&
            r2 >= 24) {
            TempDescRaw* td = (TempDescRaw*)tb.data();
            int n = td->InfoCount;
            if (n < 1) n = 1;
            if (n > 8) n = 8;
            int fit = ((int)r2 - 24) / (int)sizeof(TempInfoRaw);   // 防御厂商少报 InfoCount
            if (n > fit) n = fit;
            double best = -1.0;
            for (int i = 0; i < n; ++i) {
                short t = td->Info[i].Temperature;
                if (t > 0 && t < 125 && t > best) best = t;        // 取多传感器里的最高值
            }
            if (best > 0) {
                out.tempC   = best;
                out.sensors = n;
                break;
            }
        }
    }

    CloseHandle(h);
    return out;
}

DiskReading ReadDiskTemperature(int cached_drive) {
    DiskReading out;

    // 已缓存盘号：只探这一块；探不到就退回全盘扫描（唤醒/掉盘后盘号可能变）
    if (cached_drive >= 0) {
        DiskProbe p = ProbeDisk(cached_drive);
        if (p.tempC > 0) {
            out.drive = cached_drive;
            out.tempC = p.tempC;
            out.model = p.model;
            return out;
        }
    }

    for (int i = 0; i < kMaxDisks; ++i) {
        DiskProbe p = ProbeDisk(i);
        if (p.tempC > 0) {
            out.drive = i;
            out.tempC = p.tempC;
            out.model = p.model;
            return out;
        }
    }
    return out;   // drive = -1：本轮没有任何盘上报温度
}
