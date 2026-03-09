#include <CUnit/CUnit.h>
#include <math.h>
#include "wu.h"

void test_mvar_returns_nan_during_warmup(void) {
    WU_MVar mvar = wu_mvar_new(3, 0);
    wu_indicator_update(mvar, 1.0);
    CU_ASSERT_TRUE(isnan(wu_indicator_get(mvar)));
    wu_indicator_update(mvar, 2.0);
    CU_ASSERT_TRUE(isnan(wu_indicator_get(mvar)));
    wu_indicator_update(mvar, 3.0);
    CU_ASSERT_FALSE(isnan(wu_indicator_get(mvar)));
    wu_indicator_delete(mvar);
}

void test_mvar_calculates_population_variance(void) {
    WU_MVar mvar = wu_mvar_new(3, 0);
    wu_indicator_update(mvar, 2.0);
    wu_indicator_update(mvar, 4.0);
    wu_indicator_update(mvar, 6.0);
    double result = wu_indicator_get(mvar);
    double expected = ((2.0 - 4.0) * (2.0 - 4.0) + (4.0 - 4.0) * (4.0 - 4.0) + (6.0 - 4.0) * (6.0 - 4.0)) / 3.0;
    CU_ASSERT_DOUBLE_EQUAL(result, expected, 0.0001);
    wu_indicator_delete(mvar);
}

void test_mvar_calculates_sample_variance(void) {
    WU_MVar mvar = wu_mvar_new(3, 1);
    wu_indicator_update(mvar, 2.0);
    wu_indicator_update(mvar, 4.0);
    wu_indicator_update(mvar, 6.0);
    double result = wu_indicator_get(mvar);
    double expected = ((2.0 - 4.0) * (2.0 - 4.0) + (4.0 - 4.0) * (4.0 - 4.0) + (6.0 - 4.0) * (6.0 - 4.0)) / 2.0;
    CU_ASSERT_DOUBLE_EQUAL(result, expected, 0.0001);
    wu_indicator_delete(mvar);
}

void test_mvar_sliding_window_updates_correctly(void) {
    WU_MVar mvar = wu_mvar_new(3, 0);
    wu_indicator_update(mvar, 2.0);
    wu_indicator_update(mvar, 4.0);
    wu_indicator_update(mvar, 6.0);
    wu_indicator_update(mvar, 8.0);
    double result = wu_indicator_get(mvar);
    double expected = ((4.0 - 6.0) * (4.0 - 6.0) + (6.0 - 6.0) * (6.0 - 6.0) + (8.0 - 6.0) * (8.0 - 6.0)) / 3.0;
    CU_ASSERT_DOUBLE_EQUAL(result, expected, 0.0001);
    wu_indicator_delete(mvar);
}

