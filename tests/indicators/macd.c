#include <CUnit/CUnit.h>
#include <math.h>
#include "wu.h"

void test_macd_returns_nan_during_warmup(void) {
    WU_MACD macd = wu_macd_new(12, 26, 9, 2.0);
    
    for (int i = 0; i < 27; i++) {
        WU_MACDResult result = wu_indicator_update(macd, 100.0 + i);
        if (i < 26) {
            CU_ASSERT_TRUE(isnan(result.macd));
        }
    }
    
    WU_MACDResult result = wu_indicator_get(macd);
    CU_ASSERT_FALSE(isnan(result.macd));
    wu_indicator_delete(macd);
}

void test_macd_signal_warmup(void) {
    WU_MACD macd = wu_macd_new(12, 26, 9, 2.0);
    
    for (int i = 0; i < 35; i++) {
        WU_MACDResult result = wu_indicator_update(macd, 100.0 + i * 0.5);
        if (i < 34) {
            CU_ASSERT_TRUE(isnan(result.signal));
            CU_ASSERT_TRUE(isnan(result.histogram));
        }
    }
    
    WU_MACDResult result = wu_indicator_get(macd);
    CU_ASSERT_FALSE(isnan(result.signal));
    CU_ASSERT_FALSE(isnan(result.histogram));
    wu_indicator_delete(macd);
}

void test_macd_calculates_correct_values(void) {
    WU_MACD macd = wu_macd_new(12, 26, 9, 2.0);
    
    for (int i = 0; i < 50; i++) {
        wu_indicator_update(macd, 100.0 + i * 0.1);
    }
    
    WU_MACDResult result = wu_indicator_get(macd);
    CU_ASSERT_FALSE(isnan(result.macd));
    CU_ASSERT_FALSE(isnan(result.signal));
    CU_ASSERT_FALSE(isnan(result.histogram));
    
    CU_ASSERT_DOUBLE_EQUAL(result.histogram, result.macd - result.signal, 1e-10);
    wu_indicator_delete(macd);
}

void test_macd_histogram_consistency(void) {
    WU_MACD macd = wu_macd_new(12, 26, 9, 2.0);
    
    for (int i = 0; i < 50; i++) {
        WU_MACDResult result = wu_indicator_update(macd, 100.0 + (i % 10) * 0.5);
        if (!isnan(result.histogram) && !isnan(result.macd) && !isnan(result.signal)) {
            CU_ASSERT_DOUBLE_EQUAL(result.histogram, result.macd - result.signal, 1e-10);
        }
    }
    
    wu_indicator_delete(macd);
}

void test_macd_uptrend_produces_positive_macd(void) {
    WU_MACD macd = wu_macd_new(12, 26, 9, 2.0);
    
    for (int i = 0; i < 100; i++) {
        wu_indicator_update(macd, 100.0 + i);
    }
    
    WU_MACDResult result = wu_indicator_get(macd);
    CU_ASSERT_TRUE(result.macd > 0.0);
    wu_indicator_delete(macd);
}

void test_macd_downtrend_produces_negative_macd(void) {
    WU_MACD macd = wu_macd_new(12, 26, 9, 2.0);
    
    for (int i = 0; i < 100; i++) {
        wu_indicator_update(macd, 200.0 - i);
    }
    
    WU_MACDResult result = wu_indicator_get(macd);
    CU_ASSERT_TRUE(result.macd < 0.0);
    wu_indicator_delete(macd);
}
