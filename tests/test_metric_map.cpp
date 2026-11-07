// ============================================================================
//  test_metric_map.cpp — core/metric_map 的单元测试
//
//  覆盖三件事：
//    · 比例映射（环形仪表 / 进度条）：温度 25~105 归一化，百分比 0~100
//    · 档位判定：阈值按"四舍五入后的显示值"比较，保证数字与配色口径一致
//    · 数值格式化：没采到显示 "--"，否则取整
//
//  不引第三方测试框架：一个 CHECK 宏 + 计数就够，失败打印文件行号并返回非 0，
//  CTest 据此判红。
// ============================================================================

#include "metric_map.h"

#include <cmath>
#include <cstdio>
#include <string>

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        if (cond) {                                                     \
            ++g_pass;                                                   \
        } else {                                                        \
            ++g_fail;                                                   \
            printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);      \
        }                                                               \
    } while (0)

#define CHECK_NEAR(actual, expect, eps)                                          \
    do {                                                                         \
        const double a_ = (actual);                                              \
        const double e_ = (expect);                                              \
        if (std::fabs(a_ - e_) <= (eps)) {                                       \
            ++g_pass;                                                            \
        } else {                                                                 \
            ++g_fail;                                                            \
            printf("FAIL %s:%d  %s = %.6f, expect %.6f\n", __FILE__, __LINE__,   \
                   #actual, a_, e_);                                             \
        }                                                                        \
    } while (0)

#define CHECK_WSTR(actual, expect)                                              \
    do {                                                                        \
        const std::wstring a_ = (actual);                                       \
        const std::wstring e_ = (expect);                                       \
        if (a_ == e_) {                                                         \
            ++g_pass;                                                           \
        } else {                                                                \
            ++g_fail;                                                           \
            printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #actual);            \
        }                                                                       \
    } while (0)

int main() {
    // --- 温度比例：25 以下空环，105 满环 -----------------------------------
    CHECK_NEAR(core::temp_ratio(25.0), 0.0, 1e-9);
    CHECK_NEAR(core::temp_ratio(65.0), 0.5, 1e-9);
    CHECK_NEAR(core::temp_ratio(105.0), 1.0, 1e-9);
    CHECK_NEAR(core::temp_ratio(10.0), 0.0, 1e-9);    // 低于下限：钳到 0
    CHECK_NEAR(core::temp_ratio(150.0), 1.0, 1e-9);   // 高于上限：钳到 1
    CHECK_NEAR(core::temp_ratio(-1.0), -1.0, 1e-9);   // 没采到：返回 -1（不画环）

    // --- 百分比比例 ---------------------------------------------------------
    CHECK_NEAR(core::percent_ratio(0.0), 0.0, 1e-9);
    CHECK_NEAR(core::percent_ratio(50.0), 0.5, 1e-9);
    CHECK_NEAR(core::percent_ratio(100.0), 1.0, 1e-9);
    CHECK_NEAR(core::percent_ratio(120.0), 1.0, 1e-9);
    CHECK_NEAR(core::percent_ratio(-1.0), -1.0, 1e-9);

    // --- 档位：value < warn_at 正常，< hot_at 警告，否则危险 ----------------
    // CPU 温度 75/90
    CHECK(core::level_by_threshold(74, 75, 90) == core::kNormal);
    CHECK(core::level_by_threshold(75, 75, 90) == core::kWarn);
    CHECK(core::level_by_threshold(89, 75, 90) == core::kWarn);
    CHECK(core::level_by_threshold(90, 75, 90) == core::kHot);
    CHECK(core::level_by_threshold(120, 75, 90) == core::kHot);
    // SSD 温度 55/70
    CHECK(core::level_by_threshold(54, 55, 70) == core::kNormal);
    CHECK(core::level_by_threshold(70, 55, 70) == core::kHot);
    // 内存 61/86 与 CPU 占用 51/81（阈值写法不同，但语义是 <=60 / <=85）
    CHECK(core::level_by_threshold(60, 61, 86) == core::kNormal);
    CHECK(core::level_by_threshold(61, 61, 86) == core::kWarn);
    CHECK(core::level_by_threshold(85, 61, 86) == core::kWarn);
    CHECK(core::level_by_threshold(86, 61, 86) == core::kHot);

    // --- 四舍五入：与球面显示口径一致 --------------------------------------
    CHECK(core::round_to_int(74.4) == 74);
    CHECK(core::round_to_int(74.5) == 75);
    CHECK(core::round_to_int(74.6) == 75);
    CHECK(core::round_to_int(0.0) == 0);

    // --- 数值文本 -----------------------------------------------------------
    CHECK_WSTR(core::format_value(-1.0), L"--");    // 没采到
    CHECK_WSTR(core::format_value(-0.5), L"--");
    CHECK_WSTR(core::format_value(0.0), L"0");
    CHECK_WSTR(core::format_value(47.5), L"48");
    CHECK_WSTR(core::format_value(100.0), L"100");

    // --- 显示值 → 档位的联动（这才是屏幕上真正发生的事）--------------------
    // 74.6 °C 显示成 75，配色也就该按 75 判成"警告"而不是"正常"
    CHECK(core::level_by_threshold(core::round_to_int(74.6), 75, 90) == core::kWarn);
    CHECK(core::level_by_threshold(core::round_to_int(74.4), 75, 90) == core::kNormal);

    printf("\n  metric_map: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
