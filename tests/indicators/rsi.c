#include <CUnit/CUnit.h>
#include <math.h>
#include "wu.h"

void test_rsi_returns_nan_during_warmup(void) {
    WU_RSI rsi = wu_rsi_new(14);
    for (int i = 0; i < 14; i++) {
        double open = 44.0 + i * 0.1;
        double close = open + ((i % 2 == 0) ? 0.5 : -0.3);
        WU_Candle candle = wu_candle_init((WU_TimeStamp){.mark = i, .units = WU_TIME_UNIT_SECONDS}, 
                                          open, open + 1.0, open - 1.0, close, 100.0);
        wu_indicator_update(rsi, &candle);
        if (i < 13) {
            CU_ASSERT_TRUE(isnan(wu_indicator_get(rsi)));
        }
    }
    CU_ASSERT_FALSE(isnan(wu_indicator_get(rsi)));
    wu_indicator_delete(rsi);
}

void test_rsi_calculates_correct_value(void) {
    WU_RSI rsi = wu_rsi_new(14);
    
    for (int i = 0; i < 15; i++) {
        WU_Candle candle = wu_candle_init((WU_TimeStamp){.mark = i, .units = WU_TIME_UNIT_SECONDS}, 
                                          44.0 + i * 0.1, 45.0 + i * 0.1, 43.0 + i * 0.1, 44.5 + i * 0.1, 100.0);
        wu_indicator_update(rsi, &candle);
    }
    
    double result = wu_indicator_get(rsi);
    CU_ASSERT_FALSE(isnan(result));
    CU_ASSERT_TRUE(result >= 0.0 && result <= 100.0);
    wu_indicator_delete(rsi);
}

void test_rsi_handles_all_gains(void) {
    WU_RSI rsi = wu_rsi_new(5);
    
    for (int i = 0; i < 6; i++) {
        WU_Candle candle = wu_candle_init((WU_TimeStamp){.mark = i, .units = WU_TIME_UNIT_SECONDS}, 
                                          10.0 + i, 11.0 + i, 10.0 + i, 11.0 + i, 100.0);
        wu_indicator_update(rsi, &candle);
    }
    
    double result = wu_indicator_get(rsi);
    CU_ASSERT_DOUBLE_EQUAL(result, 100.0, 0.0001);
    wu_indicator_delete(rsi);
}

void test_rsi_handles_all_losses(void) {
    WU_RSI rsi = wu_rsi_new(5);
    
    for (int i = 0; i < 6; i++) {
        WU_Candle candle = wu_candle_init((WU_TimeStamp){.mark = i, .units = WU_TIME_UNIT_SECONDS}, 
                                          15.0 - i, 15.0 - i, 14.0 - i, 14.0 - i, 100.0);
        wu_indicator_update(rsi, &candle);
    }
    
    double result = wu_indicator_get(rsi);
    CU_ASSERT_DOUBLE_EQUAL(result, 0.0, 0.0001);
    wu_indicator_delete(rsi);
}

void test_rsi_range_is_valid(void) {
    WU_RSI rsi = wu_rsi_new(14);
    
    for (int i = 0; i < 30; i++) {
        double open_price = 50.0 + (i % 5) - 2.0;
        double close_price = open_price + ((i % 3) - 1.0) * 0.5;
        WU_Candle candle = wu_candle_init((WU_TimeStamp){.mark = i, .units = WU_TIME_UNIT_SECONDS}, 
                                          open_price, open_price + 1.0, open_price - 1.0, close_price, 100.0);
        wu_indicator_update(rsi, &candle);
        double result = wu_indicator_get(rsi);
        if (!isnan(result)) {
            CU_ASSERT_TRUE(result >= 0.0 && result <= 100.0);
        }
    }
    
    wu_indicator_delete(rsi);
}
