#include <CUnit/CUnit.h>
#include <math.h>
#include "wu.h"

void test_sma_returns_nan_during_warmup(void) {
    MovingAverage sma = moving_average_new(3);
    indicator_update(sma, 1.0);
    CU_ASSERT_TRUE(isnan(DOUBLE(sma)));
    indicator_update(sma, 2.0);
    CU_ASSERT_TRUE(isnan(DOUBLE(sma)));
    indicator_update(sma, 3.0);
    CU_ASSERT_FALSE(isnan(DOUBLE(sma)));
    indicator_delete(sma);
}

void test_sma_calculates_correct_moving_average(void) {
    MovingAverage sma = moving_average_new(3);
    indicator_update(sma, 10.0);
    indicator_update(sma, 20.0);
    indicator_update(sma, 30.0);
    double result = DOUBLE(sma);
    CU_ASSERT_DOUBLE_EQUAL(result, 20.0, 0.0001);
    indicator_delete(sma);
}

void test_sma_sliding_window_updates_correctly(void) {
    MovingAverage sma = moving_average_new(3);
    indicator_update(sma, 10.0);
    indicator_update(sma, 20.0);
    indicator_update(sma, 30.0);
    indicator_update(sma, 40.0);
    double result = DOUBLE(sma);
    CU_ASSERT_DOUBLE_EQUAL(result, 30.0, 0.0001);
    indicator_delete(sma);
}


