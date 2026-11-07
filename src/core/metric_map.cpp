#include "metric_map.h"

#include <cwchar>

namespace core {
namespace {

double Clamp01(double v) {
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

}  // namespace

double temp_ratio(double celsius) {
    if (celsius < 0.0) return -1.0;   // 没采到，画空环而不是画满环
    return Clamp01((celsius - 25.0) / 80.0);
}

double percent_ratio(double percent) {
    if (percent < 0.0) return -1.0;
    return Clamp01(percent / 100.0);
}

Level level_by_threshold(int value, int warn_at, int hot_at) {
    if (value < warn_at) return kNormal;
    if (value < hot_at)  return kWarn;
    return kHot;
}

int round_to_int(double v) {
    return (int)(v + 0.5);
}

std::wstring format_value(double v) {
    if (v < 0.0) return L"--";
    wchar_t buf[32] = {};
    swprintf(buf, 32, L"%.0f", v);
    return std::wstring(buf);
}

}  // namespace core
