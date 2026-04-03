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

void test_sharpe_basic(void) {
    WU_SharpeRatio sr = wu_sharpe_ratio_new(100.0, 0.0);
    CU_ASSERT_PTR_NOT_NULL(sr);
    int64_t one_day = 86400; /* seconds in a day */
    double portfolio_value = 100.0;
    for (int i = 0; i < 35; i++) {
        double daily_return = (i % 2 == 0) ? 0.02 : -0.01;
        portfolio_value *= (1.0 + daily_return);
        WU_PerformanceUpdate perf = {
            .portfolio_value = portfolio_value,
            .timestamp = {.mark = (i + 1) * one_day, .units = WU_TIME_UNIT_SECONDS}
        };
        sr->update(sr, perf);
    }

    double val = wu_indicator_get(sr);

    CU_ASSERT_FALSE(isnan(val));
    CU_ASSERT_TRUE(isfinite(val));

    sr->delete(sr);
}

void test_sharpe_with_volatility(void) {
    /* Test with varying returns to produce a valid Sharpe ratio */
    WU_SharpeRatio sr = wu_sharpe_ratio_new(100.0, 0.0);
    CU_ASSERT_PTR_NOT_NULL(sr);

    /* Simulate 35 days of returns with some volatility
     * Returns: +2%, -1%, +2%, -1%, ... pattern */
    int64_t one_day = 86400;
    double portfolio_value = 100.0;
    int num_updates = 35;

    for (int i = 0; i < num_updates; i++) {
        /* Alternating returns: even days +2%, odd days -1% */
        double daily_return = (i % 2 == 0) ? 0.02 : -0.01;
        portfolio_value *= (1.0 + daily_return);

        WU_PerformanceUpdate perf = {
            .portfolio_value = portfolio_value,
            .timestamp = {.mark = (i + 1) * one_day, .units = WU_TIME_UNIT_SECONDS}
        };
        sr->update(sr, perf);
    }

    double val = wu_indicator_get(sr);

    /* Should have a valid Sharpe ratio now (not NAN) */
    CU_ASSERT_FALSE(isnan(val));
    CU_ASSERT_TRUE(isfinite(val));

    sr->delete(sr);
}

void test_sharpe_insufficient_data(void) {
    /* Test that Sharpe returns NAN with insufficient data */
    WU_SharpeRatio sr = wu_sharpe_ratio_new(100.0, 0.0);
    CU_ASSERT_PTR_NOT_NULL(sr);

    /* Feed only 5 updates (less than SHARPE_MIN_OBSERVATIONS = 30) */
    int64_t one_day = 86400;
    double portfolio_value = 100.0;

    for (int i = 0; i < 5; i++) {
        portfolio_value *= 1.01;
        WU_PerformanceUpdate perf = {
            .portfolio_value = portfolio_value,
            .timestamp = {.mark = (i + 1) * one_day, .units = WU_TIME_UNIT_SECONDS}
        };
        sr->update(sr, perf);
    }

    double val = wu_indicator_get(sr);
    CU_ASSERT_TRUE(isnan(val));

    sr->delete(sr);
}
