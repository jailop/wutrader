#include <CUnit/CUnit.h>
#include <math.h>
#include "wu.h"

void test_ema_returns_nan_during_warmup(void) {
    ExponentialMovingAverage ema = exponential_moving_average_new(3, 2.0);
    indicator_update(ema, 10.0);
    CU_ASSERT_TRUE(isnan(DOUBLE(ema)));
    indicator_update(ema, 20.0);
    CU_ASSERT_TRUE(isnan(DOUBLE(ema)));
    indicator_update(ema, 30.0);
    CU_ASSERT_FALSE(isnan(DOUBLE(ema)));
    indicator_delete(ema);
}

void test_ema_first_value_is_sma_of_warmup_period(void) {
    ExponentialMovingAverage ema = exponential_moving_average_new(3, 2.0);
    indicator_update(ema, 10.0);
    indicator_update(ema, 20.0);
    indicator_update(ema, 30.0);
    double result = DOUBLE(ema);
    CU_ASSERT_DOUBLE_EQUAL(result, (10.0 + 20.0 + 30.0) / 3.0, 0.0001);
    indicator_delete(ema);
}

void test_ema_applies_exponential_smoothing_after_warmup(void) {
    ExponentialMovingAverage ema = exponential_moving_average_new(3, 2.0);
    indicator_update(ema, 10.0);
    indicator_update(ema, 20.0);
    indicator_update(ema, 30.0);
    double prevEMA = DOUBLE(ema);
    double alpha = 2.0 / (3.0 + 1.0);
    double newPrice = 40.0;
    double expectedEMA = (newPrice * alpha) + (prevEMA * (1.0 - alpha));
    indicator_update(ema, newPrice);
    double result = DOUBLE(ema);
    CU_ASSERT_DOUBLE_EQUAL(result, expectedEMA, 1e-10);
    indicator_delete(ema);
}
