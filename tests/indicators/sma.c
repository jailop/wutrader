#include <CUnit/CUnit.h>
#include <math.h>
#include "wu.h"

void test_sma_returns_nan_during_warmup(void) {
    WU_SMA sma = wu_sma_new(3);
    wu_indicator_update(sma, 1.0);
    CU_ASSERT_TRUE(isnan(wu_indicator_get(sma)));
    wu_indicator_update(sma, 2.0);
    CU_ASSERT_TRUE(isnan(wu_indicator_get(sma)));
    wu_indicator_update(sma, 3.0);
    CU_ASSERT_FALSE(isnan(wu_indicator_get(sma)));
    wu_indicator_delete(sma);
}

void test_sma_calculates_correct_sma(void) {
    WU_SMA sma = wu_sma_new(3);
    wu_indicator_update(sma, 10.0);
    wu_indicator_update(sma, 20.0);
    wu_indicator_update(sma, 30.0);
    double result = wu_indicator_get(sma);
    CU_ASSERT_DOUBLE_EQUAL(result, 20.0, 0.0001);
    wu_indicator_delete(sma);
}

void test_sma_sliding_window_updates_correctly(void) {
    WU_SMA sma = wu_sma_new(3);
    wu_indicator_update(sma, 10.0);
    wu_indicator_update(sma, 20.0);
    wu_indicator_update(sma, 30.0);
    wu_indicator_update(sma, 40.0);
    double result = wu_indicator_get(sma);
    CU_ASSERT_DOUBLE_EQUAL(result, 30.0, 0.0001);
    wu_indicator_delete(sma);
}
