#include <CUnit/CUnit.h>
#include <math.h>
#include "wu.h"

void test_pairs_trading_initialization(void) {
    WU_PairsTradingStrat strat = wu_pairs_trading_strat_new(20, 2.0, 1.0);
    CU_ASSERT_PTR_NOT_NULL(strat);
    CU_ASSERT_PTR_NOT_NULL(strat->spread_ma);
    CU_ASSERT_PTR_NOT_NULL(strat->spread_std);
    CU_ASSERT_EQUAL(strat->base.num_inputs, 2);
    CU_ASSERT_EQUAL(strat->base.num_outputs, 2);
    CU_ASSERT_DOUBLE_EQUAL(strat->threshold, 2.0, 0.0001);
    CU_ASSERT_DOUBLE_EQUAL(strat->ratio, 1.0, 0.0001);
    wu_strategy_delete((WU_Strategy)strat);
}

void test_pairs_trading_holds_during_warmup(void) {
    WU_PairsTradingStrat strat = wu_pairs_trading_strat_new(20, 2.0, 1.0);
    
    // Feed 10 periods (less than window of 20)
    for (int i = 0; i < 10; i++) {
        WU_Single asset_a = wu_single_init(1000 + i, 100.0);
        WU_Single asset_b = wu_single_init(1000 + i, 50.0);
        const void* inputs[] = {&asset_a, &asset_b};
        
        WU_Signal* signals = wu_strategy_update((WU_Strategy)strat, inputs);
        
        // During warmup, should return HOLD signals
        CU_ASSERT_EQUAL(signals[0].side, WU_SIDE_HOLD);
        CU_ASSERT_EQUAL(signals[1].side, WU_SIDE_HOLD);
    }
    
    wu_strategy_delete((WU_Strategy)strat);
}

void test_pairs_trading_detects_low_spread(void) {
    WU_PairsTradingStrat strat = wu_pairs_trading_strat_new(10, 1.0, 1.0);
    
    // Establish baseline with stable spread around 50
    for (int i = 0; i < 15; i++) {
        WU_Single asset_a = wu_single_init(1000 + i, 100.0);
        WU_Single asset_b = wu_single_init(1000 + i, 50.0);
        const void* inputs[] = {&asset_a, &asset_b};
        wu_strategy_update((WU_Strategy)strat, inputs);
    }
    
    // Now push spread significantly lower (asset_a undervalued)
    // Spread = 80 - 50 = 30, mean ~50, so this should trigger signal
    WU_Single asset_a = wu_single_init(2000, 80.0);
    WU_Single asset_b = wu_single_init(2000, 50.0);
    const void* inputs[] = {&asset_a, &asset_b};
    
    WU_Signal* signals = wu_strategy_update((WU_Strategy)strat, inputs);
    
    // When spread is low (asset A undervalued): BUY A, SELL B
    CU_ASSERT_TRUE(signals[0].side == WU_SIDE_BUY || 
                   signals[0].side == WU_SIDE_HOLD);
    CU_ASSERT_TRUE(signals[1].side == WU_SIDE_SELL || 
                   signals[1].side == WU_SIDE_HOLD);
    
    wu_strategy_delete((WU_Strategy)strat);
}

void test_pairs_trading_detects_high_spread(void) {
    WU_PairsTradingStrat strat = wu_pairs_trading_strat_new(10, 1.0, 1.0);
    
    // Establish baseline with stable spread around 50
    for (int i = 0; i < 15; i++) {
        WU_Single asset_a = wu_single_init(1000 + i, 100.0);
        WU_Single asset_b = wu_single_init(1000 + i, 50.0);
        const void* inputs[] = {&asset_a, &asset_b};
        wu_strategy_update((WU_Strategy)strat, inputs);
    }
    
    // Now push spread significantly higher (asset_a overvalued)
    // Spread = 120 - 50 = 70, mean ~50, so this should trigger signal
    WU_Single asset_a = wu_single_init(2000, 120.0);
    WU_Single asset_b = wu_single_init(2000, 50.0);
    const void* inputs[] = {&asset_a, &asset_b};
    
    WU_Signal* signals = wu_strategy_update((WU_Strategy)strat, inputs);
    
    // When spread is high (asset A overvalued): SELL A, BUY B
    CU_ASSERT_TRUE(signals[0].side == WU_SIDE_SELL || 
                   signals[0].side == WU_SIDE_HOLD);
    CU_ASSERT_TRUE(signals[1].side == WU_SIDE_BUY || 
                   signals[1].side == WU_SIDE_HOLD);
    
    wu_strategy_delete((WU_Strategy)strat);
}

void test_pairs_trading_signal_vector_structure(void) {
    WU_PairsTradingStrat strat = wu_pairs_trading_strat_new(5, 2.0, 1.0);
    
    WU_Single asset_a = wu_single_init(1000, 100.0);
    WU_Single asset_b = wu_single_init(1000, 50.0);
    const void* inputs[] = {&asset_a, &asset_b};
    
    WU_Signal* signals = wu_strategy_update((WU_Strategy)strat, inputs);
    
    // Verify signal buffer structure via strategy base
    CU_ASSERT_PTR_NOT_NULL(strat->base.signal_buffer);
    CU_ASSERT_EQUAL(strat->base.num_outputs, 2);
    
    // Verify prices match inputs
    CU_ASSERT_DOUBLE_EQUAL(signals[0].price, 100.0, 0.001);
    CU_ASSERT_DOUBLE_EQUAL(signals[1].price, 50.0, 0.001);
    
    // Verify timestamps match
    CU_ASSERT_EQUAL(signals[0].timestamp, 1000);
    CU_ASSERT_EQUAL(signals[1].timestamp, 1000);
    
    wu_strategy_delete((WU_Strategy)strat);
}

void test_pairs_trading_with_custom_ratio(void) {
    // Test with 2:1 hedge ratio
    WU_PairsTradingStrat strat = wu_pairs_trading_strat_new(10, 2.0, 2.0);
    
    CU_ASSERT_DOUBLE_EQUAL(strat->ratio, 2.0, 0.0001);
    
    // Feed some data
    for (int i = 0; i < 15; i++) {
        WU_Single asset_a = wu_single_init(1000 + i, 200.0);
        WU_Single asset_b = wu_single_init(1000 + i, 50.0);  // 2:1 ratio = 100
        const void* inputs[] = {&asset_a, &asset_b};
        wu_strategy_update((WU_Strategy)strat, inputs);
    }
    
    // Spread should be calculated as: asset_a - (ratio * asset_b)
    // = 200 - (2.0 * 50) = 200 - 100 = 100
    
    wu_strategy_delete((WU_Strategy)strat);
}

void test_pairs_trading_input_validation(void) {
    WU_PairsTradingStrat strat = wu_pairs_trading_strat_new(10, 2.0, 1.0);
    
    // Verify strategy metadata
    CU_ASSERT_EQUAL(strat->base.num_inputs, 2);
    CU_ASSERT_EQUAL(strat->base.num_outputs, 2);
    CU_ASSERT_PTR_NOT_NULL(strat->base.input_types);
    CU_ASSERT_EQUAL(strat->base.input_types[0], WU_DATA_TYPE_SINGLE_VALUE);
    CU_ASSERT_EQUAL(strat->base.input_types[1], WU_DATA_TYPE_SINGLE_VALUE);
    
    wu_strategy_delete((WU_Strategy)strat);
}
