#include <CUnit/CUnit.h>
#include <math.h>
#include "wu.h"

void test_crossover_initialization(void) {
    WU_CrossOverStrat strat = wu_crossover_strat_new(5, 10, 0.01);
    CU_ASSERT_PTR_NOT_NULL(strat);
    CU_ASSERT_PTR_NOT_NULL(strat->short_ma);
    CU_ASSERT_PTR_NOT_NULL(strat->long_ma);
    CU_ASSERT_DOUBLE_EQUAL(strat->threshold, 0.01, 0.0001);
    CU_ASSERT_EQUAL(strat->last_signal, WU_SIDE_HOLD);
    wu_strategy_delete((WU_Strategy)strat);
}

void test_crossover_holds_during_warmup(void) {
    WU_CrossOverStrat strat = wu_crossover_strat_new(3, 5, 0.01);
    WU_Single sv1 = wu_single_init(1000, 100.0);
    const void* inputs1[] = {&sv1};
    WU_Signal* signals = wu_strategy_update((WU_Strategy)strat, inputs1);
    CU_ASSERT_EQUAL(signals[0].side, WU_SIDE_HOLD);
    WU_Single sv2 = wu_single_init(2000, 105.0);
    const void* inputs2[] = {&sv2};
    signals = wu_strategy_update((WU_Strategy)strat, inputs2);
    CU_ASSERT_EQUAL(signals[0].side, WU_SIDE_HOLD);
    wu_strategy_delete((WU_Strategy)strat);
}

void test_crossover_generates_buy_signal(void) {
    WU_CrossOverStrat strat = wu_crossover_strat_new(2, 5, 0.0);
    for (int i = 0; i < 10; i++) {
        WU_Single sv = wu_single_init(1000 + i, 100.0);
        const void* inputs[] = {&sv};
        wu_strategy_update((WU_Strategy)strat, inputs);
    }
    for (int i = 0; i < 3; i++) {
        WU_Single sv = wu_single_init(2000 + i, 150.0);
        const void* inputs[] = {&sv};
        WU_Signal* signals = wu_strategy_update((WU_Strategy)strat, inputs);
        if (signals[0].side == WU_SIDE_BUY) {
            CU_ASSERT_EQUAL(signals[0].side, WU_SIDE_BUY);
            wu_strategy_delete((WU_Strategy)strat);
            return;
        }
    }
    CU_FAIL("Expected to generate a BUY signal");
    wu_strategy_delete((WU_Strategy)strat);
}

void test_crossover_generates_sell_signal(void) {
    WU_CrossOverStrat strat = wu_crossover_strat_new(2, 5, 0.0);
    for (int i = 0; i < 10; i++) {
        WU_Single sv = wu_single_init(1000 + i, 100.0);
        const void* inputs[] = {&sv};
        wu_strategy_update((WU_Strategy)strat, inputs);
    }
    for (int i = 0; i < 3; i++) {
        WU_Single sv = wu_single_init(2000 + i, 50.0);
        const void* inputs[] = {&sv};
        WU_Signal* signals = wu_strategy_update((WU_Strategy)strat, inputs);
        if (signals[0].side == WU_SIDE_SELL) {
            CU_ASSERT_EQUAL(signals[0].side, WU_SIDE_SELL);
            wu_strategy_delete((WU_Strategy)strat);
            return;
        }
    }
    CU_FAIL("Expected to generate a SELL signal");
    wu_strategy_delete((WU_Strategy)strat);
}

void test_crossover_no_repeat_signals(void) {
    WU_CrossOverStrat strat = wu_crossover_strat_new(2, 5, 0.0);
    for (int i = 0; i < 10; i++) {
        WU_Single sv = wu_single_init(1000 + i, 100.0);
        const void* inputs[] = {&sv};
        wu_strategy_update((WU_Strategy)strat, inputs);
    }
    bool got_buy = false;
    for (int i = 0; i < 5; i++) {
        WU_Single sv = wu_single_init(2000 + i, 150.0);
        const void* inputs[] = {&sv};
        WU_Signal* signals = wu_strategy_update((WU_Strategy)strat, inputs);
        if (signals[0].side == WU_SIDE_BUY) {
            got_buy = true;
            break;
        }
    }
    CU_ASSERT_TRUE(got_buy);
    WU_Single sv3 = wu_single_init(3000, 160.0);
    const void* inputs3[] = {&sv3};
    WU_Signal* signals2 = wu_strategy_update((WU_Strategy)strat, inputs3);
    CU_ASSERT_EQUAL(signals2[0].side, WU_SIDE_HOLD);
    wu_strategy_delete((WU_Strategy)strat);
}

void test_crossover_with_real_data(void) {
    FILE* file = fopen("tests/data/btcusd_price.csv", "r");
    CU_ASSERT_PTR_NOT_NULL(file);
    WU_CsvReader reader = wu_csv_reader_new(file, WU_DATA_TYPE_SINGLE_VALUE, false);
    CU_ASSERT_PTR_NOT_NULL(reader);
    WU_CrossOverStrat strat = wu_crossover_strat_new(5, 20, 0.02);
    int total = 0;
    WU_Single* sv;
    while ((sv = (WU_Single*)wu_reader_next(reader)) != NULL && total < 100) {
        // Verify data_type is set correctly
        CU_ASSERT_EQUAL(sv->data_type, WU_DATA_TYPE_SINGLE_VALUE);
        const void* inputs[] = {sv};
        WU_Signal* signals = wu_strategy_update((WU_Strategy)strat, inputs);
        // Just verify signal is valid
        CU_ASSERT_TRUE(signals[0].side == WU_SIDE_BUY || 
                       signals[0].side == WU_SIDE_SELL || 
                       signals[0].side == WU_SIDE_HOLD);
        total++;
    }
    
    // Verify we processed data
    CU_ASSERT_TRUE(total > 0);
    
    wu_strategy_delete((WU_Strategy)strat);
    wu_reader_delete((WU_Reader)reader);
    fclose(file);
}

void test_crossover_threshold_prevents_noise(void) {
    WU_CrossOverStrat strat = wu_crossover_strat_new(2, 5, 0.05);  // 5% threshold
    for (int i = 0; i < 10; i++) {
        WU_Single sv = wu_single_init(1000 + i, 100.0);
        const void* inputs[] = {&sv};
        wu_strategy_update((WU_Strategy)strat, inputs);
    }
    for (int i = 0; i < 3; i++) {
        WU_Single sv = wu_single_init(2000 + i, 103.0);
        const void* inputs[] = {&sv};
        WU_Signal* signals = wu_strategy_update((WU_Strategy)strat, inputs);
        CU_ASSERT_EQUAL(signals[0].side, WU_SIDE_HOLD);
    }
    wu_strategy_delete((WU_Strategy)strat);
}
