#include <CUnit/CUnit.h>
#include <math.h>
#include "wu.h"

void test_crossover_initialization(void) {
    CrossOverStrat strat = cross_over_strat_new(5, 10, 0.01);
    CU_ASSERT_PTR_NOT_NULL(strat);
    CU_ASSERT_PTR_NOT_NULL(strat->short_ma);
    CU_ASSERT_PTR_NOT_NULL(strat->long_ma);
    CU_ASSERT_DOUBLE_EQUAL(strat->threshold, 0.01, 0.0001);
    CU_ASSERT_EQUAL(strat->last_signal, SIDE_HOLD);
    cross_over_strat_free(strat);
}

void test_crossover_holds_during_warmup(void) {
    CrossOverStrat strat = cross_over_strat_new(3, 5, 0.01);
    SingleValue sv1 = single_value_init(1000, 100.0);
    Signal signal = strategy_update((Strategy)strat, &sv1);
    CU_ASSERT_EQUAL(signal.side, SIDE_HOLD);
    SingleValue sv2 = single_value_init(2000, 105.0);
    signal = strategy_update((Strategy)strat, &sv2);
    CU_ASSERT_EQUAL(signal.side, SIDE_HOLD);
    cross_over_strat_free(strat);
}

void test_crossover_generates_buy_signal(void) {
    CrossOverStrat strat = cross_over_strat_new(2, 5, 0.0);
    for (int i = 0; i < 10; i++) {
        SingleValue sv = single_value_init(1000 + i, 100.0);
        strategy_update((Strategy)strat, &sv);
    }
    for (int i = 0; i < 3; i++) {
        SingleValue sv = single_value_init(2000 + i, 150.0);
        Signal signal = strategy_update((Strategy)strat, &sv);
        if (signal.side == SIDE_BUY) {
            CU_ASSERT_EQUAL(signal.side, SIDE_BUY);
            cross_over_strat_free(strat);
            return;
        }
    }
    CU_FAIL("Expected to generate a BUY signal");
    cross_over_strat_free(strat);
}

void test_crossover_generates_sell_signal(void) {
    CrossOverStrat strat = cross_over_strat_new(2, 5, 0.0);
    for (int i = 0; i < 10; i++) {
        SingleValue sv = single_value_init(1000 + i, 100.0);
        strategy_update((Strategy)strat, &sv);
    }
    for (int i = 0; i < 3; i++) {
        SingleValue sv = single_value_init(2000 + i, 50.0);
        Signal signal = strategy_update((Strategy)strat, &sv);
        if (signal.side == SIDE_SELL) {
            CU_ASSERT_EQUAL(signal.side, SIDE_SELL);
            cross_over_strat_free(strat);
            return;
        }
    }
    CU_FAIL("Expected to generate a SELL signal");
    cross_over_strat_free(strat);
}

void test_crossover_no_repeat_signals(void) {
    CrossOverStrat strat = cross_over_strat_new(2, 5, 0.0);
    for (int i = 0; i < 10; i++) {
        SingleValue sv = single_value_init(1000 + i, 100.0);
        strategy_update((Strategy)strat, &sv);
    }
    bool got_buy = false;
    for (int i = 0; i < 5; i++) {
        SingleValue sv = single_value_init(2000 + i, 150.0);
        Signal signal = strategy_update((Strategy)strat, &sv);
        if (signal.side == SIDE_BUY) {
            got_buy = true;
            break;
        }
    }
    CU_ASSERT_TRUE(got_buy);
    SingleValue sv3 = single_value_init(3000, 160.0);
    Signal signal2 = strategy_update((Strategy)strat, &sv3);
    CU_ASSERT_EQUAL(signal2.side, SIDE_HOLD);
    cross_over_strat_free(strat);
}

void test_crossover_with_real_data(void) {
    FILE* file = fopen("tests/data/btcusd_price.csv", "r");
    CU_ASSERT_PTR_NOT_NULL(file);
    CsvReader reader = csv_reader_new(file, DATA_TYPE_SINGLE_VALUE, false);
    CU_ASSERT_PTR_NOT_NULL(reader);
    CrossOverStrat strat = cross_over_strat_new(5, 20, 0.02);
    int total = 0;
    SingleValue* sv;
    while ((sv = (SingleValue*)reader_next(reader)) != NULL && total < 100) {
        // Verify data_type is set correctly
        CU_ASSERT_EQUAL(sv->data_type, DATA_TYPE_SINGLE_VALUE);
        Signal signal = strategy_update((Strategy)strat, sv);
        // Just verify signal is valid
        CU_ASSERT_TRUE(signal.side == SIDE_BUY || signal.side == SIDE_SELL || signal.side == SIDE_HOLD);
        total++;
    }
    
    // Verify we processed data
    CU_ASSERT_TRUE(total > 0);
    
    cross_over_strat_free(strat);
    csv_reader_free(reader);
    fclose(file);
}

void test_crossover_threshold_prevents_noise(void) {
    CrossOverStrat strat = cross_over_strat_new(2, 5, 0.05);  // 5% threshold
    for (int i = 0; i < 10; i++) {
        SingleValue sv = single_value_init(1000 + i, 100.0);
        strategy_update((Strategy)strat, &sv);
    }
    for (int i = 0; i < 3; i++) {
        SingleValue sv = single_value_init(2000 + i, 103.0);
        Signal signal = strategy_update((Strategy)strat, &sv);
        CU_ASSERT_EQUAL(signal.side, SIDE_HOLD);
    }
    cross_over_strat_free(strat);
}
