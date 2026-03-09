#include <CUnit/CUnit.h>
#include <math.h>
#include "wu.h"

void test_stdev_returns_nan_during_warmup(void) {
    WU_StDev stdev = wu_stdev_new(3, 0);
    wu_indicator_update(stdev, 1.0);
    CU_ASSERT_TRUE(isnan(wu_indicator_get(stdev)));
    wu_indicator_update(stdev, 2.0);
    CU_ASSERT_TRUE(isnan(wu_indicator_get(stdev)));
    wu_indicator_update(stdev, 3.0);
    CU_ASSERT_FALSE(isnan(wu_indicator_get(stdev)));
    wu_indicator_delete(stdev);
}

void test_stdev_calculates_population_stdev(void) {
    WU_StDev stdev = wu_stdev_new(3, 0);
    wu_indicator_update(stdev, 2.0);
    wu_indicator_update(stdev, 4.0);
    wu_indicator_update(stdev, 6.0);
    double result = wu_indicator_get(stdev);
    double variance = ((2.0 - 4.0) * (2.0 - 4.0) + (4.0 - 4.0) * (4.0 - 4.0) + (6.0 - 4.0) * (6.0 - 4.0)) / 3.0;
    double expected = sqrt(variance);
    CU_ASSERT_DOUBLE_EQUAL(result, expected, 0.0001);
    wu_indicator_delete(stdev);
}

void test_stdev_calculates_sample_stdev(void) {
    WU_StDev stdev = wu_stdev_new(3, 1);
    wu_indicator_update(stdev, 2.0);
    wu_indicator_update(stdev, 4.0);
    wu_indicator_update(stdev, 6.0);
    double result = wu_indicator_get(stdev);
    double variance = ((2.0 - 4.0) * (2.0 - 4.0) + (4.0 - 4.0) * (4.0 - 4.0) + (6.0 - 4.0) * (6.0 - 4.0)) / 2.0;
    double expected = sqrt(variance);
    CU_ASSERT_DOUBLE_EQUAL(result, expected, 0.0001);
    wu_indicator_delete(stdev);
}

void test_stdev_sliding_window_updates_correctly(void) {
    WU_StDev stdev = wu_stdev_new(3, 0);
    wu_indicator_update(stdev, 2.0);
    wu_indicator_update(stdev, 4.0);
    wu_indicator_update(stdev, 6.0);
    wu_indicator_update(stdev, 8.0);
    double result = wu_indicator_get(stdev);
    double variance = ((4.0 - 6.0) * (4.0 - 6.0) + (6.0 - 6.0) * (6.0 - 6.0) + (8.0 - 6.0) * (8.0 - 6.0)) / 3.0;
    double expected = sqrt(variance);
    CU_ASSERT_DOUBLE_EQUAL(result, expected, 0.0001);
    wu_indicator_delete(stdev);
}

