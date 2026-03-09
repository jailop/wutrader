#include <CUnit/CUnit.h>
#include <math.h>
#include "wu.h"

void test_ema_returns_nan_during_warmup(void) {
    WU_EMA ema = wu_ema_new(3, 2.0);
    wu_indicator_update(ema, 10.0);
    CU_ASSERT_TRUE(isnan(wu_indicator_get(ema)));
    wu_indicator_update(ema, 20.0);
    CU_ASSERT_TRUE(isnan(wu_indicator_get(ema)));
    wu_indicator_update(ema, 30.0);
    CU_ASSERT_FALSE(isnan(wu_indicator_get(ema)));
    wu_indicator_delete(ema);
}

void test_ema_first_value_is_sma_of_warmup_period(void) {
    WU_EMA ema = wu_ema_new(3, 2.0);
    wu_indicator_update(ema, 10.0);
    wu_indicator_update(ema, 20.0);
    wu_indicator_update(ema, 30.0);
    double result = wu_indicator_get(ema);
    CU_ASSERT_DOUBLE_EQUAL(result, (10.0 + 20.0 + 30.0) / 3.0, 0.0001);
    wu_indicator_delete(ema);
}

void test_ema_applies_exponential_smoothing_after_warmup(void) {
    WU_EMA ema = wu_ema_new(3, 2.0);
    wu_indicator_update(ema, 10.0);
    wu_indicator_update(ema, 20.0);
    wu_indicator_update(ema, 30.0);
    double prevWU_EMA = wu_indicator_get(ema);
    double alpha = 2.0 / (3.0 + 1.0);
    double newPrice = 40.0;
    double expectedWU_EMA = (newPrice * alpha) + (prevWU_EMA * (1.0 - alpha));
    wu_indicator_update(ema, newPrice);
    double result = wu_indicator_get(ema);
    CU_ASSERT_DOUBLE_EQUAL(result, expectedWU_EMA, 1e-10);
    wu_indicator_delete(ema);
}
