#include "metrics.h"

#include "app.h"
#include "cpu_temp.h"
#include "disk_temp.h"
#include "metric_map.h"
#include "system_load.h"
#include "theme.h"

namespace {

// 各指标的告警阈值（单位同指标：°C 或 %）
struct Threshold {
    int warn_at;
    int hot_at;
};

Threshold ThresholdOf(int metric) {
    switch (metric) {
    case M_CPU_TEMP: return { 75, 90 };   // ≥75 黄，≥90 红
    case M_SSD_TEMP: return { 55, 70 };
    case M_RAM:      return { 61, 86 };   // ≤60 正常
    default:         return { 51, 81 };   // CPU 占用 ≤50 正常
    }
}

COLORREF AccentColor(int metric, core::Level level) {
    switch (level) {
    case core::kNormal:
        // CPU 占用用蓝色（它没有"温度"的语义），其余用青色
        return (metric == M_CPU_USE) ? g_pal.blue : g_pal.teal;
    case core::kWarn:
        return g_pal.amber;
    default:
        return g_pal.red;
    }
}

}  // namespace

void SampleAll() {
    g_m.cpuTempC = CpuTempRead();
    g_m.cpuUsage = CpuUsageRead();
    RamRead(g_m.ramUsedGB, g_m.ramTotalGB, g_m.ramPct);
}

void SsdSample() {
    DiskReading r = ReadDiskTemperature(g_ssdDrive);
    g_ssdDrive   = r.drive;
    g_m.ssdTempC = r.tempC;
    if (!r.model.empty()) g_ssdModel = r.model;
}

double MetricValue(int metric) {
    switch (metric) {
    case M_CPU_TEMP: return g_m.cpuTempC;
    case M_SSD_TEMP: return g_m.ssdTempC;
    case M_RAM:      return g_m.ramPct;
    default:         return g_m.cpuUsage;
    }
}

std::wstring MetricText(int metric, std::wstring& unit) {
    double v = MetricValue(metric);
    unit = (v < 0.0) ? L"" : kMetricDef[metric].unit;
    return core::format_value(v);
}

double MetricRatio(int metric) {
    double v = MetricValue(metric);
    if (v < 0.0) return -1.0;
    if (metric == M_CPU_TEMP || metric == M_SSD_TEMP) return core::temp_ratio(v);
    return core::percent_ratio(v);
}

COLORREF MetricAccent(int metric) {
    double v = MetricValue(metric);
    if (v < 0.0) return g_pal.text2;

    // 判定用四舍五入后的显示值，与球面/卡片上的数字口径一致
    const int shown = core::round_to_int(v);
    const Threshold t = ThresholdOf(metric);
    return AccentColor(metric, core::level_by_threshold(shown, t.warn_at, t.hot_at));
}
