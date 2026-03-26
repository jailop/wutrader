#include <CUnit/CUnit.h>
#include <math.h>
#include "wu.h"

void test_mean_basic(void) {
    WU_Mean m = wu_mean_new();
    wu_indicator_update(m, 1.0);
    wu_indicator_update(m, 2.0);
    wu_indicator_update(m, 3.0);
    double val = wu_indicator_get(m);
    CU_ASSERT_DOUBLE_EQUAL(val, 2.0, 1e-9);
    wu_indicator_delete(m);
}

void test_downside_basic(void) {
    WU_Downside d = wu_downside_new();
    wu_indicator_update(d, 0.01);
    wu_indicator_update(d, -0.02);
    wu_indicator_update(d, -0.03);
    double val = wu_indicator_get(d);
    double expected = sqrt((0.0004 + 0.0009) / 3.0);
    CU_ASSERT_DOUBLE_EQUAL(val, expected, 1e-12);
    wu_indicator_delete(d);
}

/*
void test_pnlstats_basic(void) {
    WU_PnLStats ps = wu_pnl_stats_new();
    WU_PnLStatsResult r;
    r = ps->update(ps, 1.0);
    r = ps->update(ps, 2.0);
    r = ps->update(ps, 3.0);
    CU_ASSERT_DOUBLE_EQUAL(r.mean, 2.0, 1e-9);
    CU_ASSERT_DOUBLE_EQUAL(r.stdev, 1.0, 1e-9);
    ps->delete(ps);
}
*/

void test_sharpe_basic(void) {
    WU_SharpeRatio sr = wu_sharpe_ratio_new(100.0, 0.0);
    WU_PerformanceUpdate p1 = {.portfolio_value = 101.0, .timestamp = {.mark = 0, .units = WU_TIME_UNIT_SECONDS}};
    WU_PerformanceUpdate p2 = {.portfolio_value = 102.0, .timestamp = {.mark = (int64_t)(365.25*24*3600), .units = WU_TIME_UNIT_SECONDS}};
    sr->update(sr, p1);
    double val = sr->update(sr, p2);
    CU_ASSERT_DOUBLE_EQUAL(val, 1.5, 1e-6);
    sr->delete(sr);
}
