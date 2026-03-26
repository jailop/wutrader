#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include "wu.h"

#define NUM_INPUTS 2
#define NUM_OUTPUTS 2

static const WU_DataType input_types[] = {
    WU_DATA_TYPE_SINGLE_VALUE,
    WU_DATA_TYPE_SINGLE_VALUE
};

static WU_Signal signal_buffer[NUM_OUTPUTS];

static WU_Signal* pairs_trading_strat_update(struct WU_Strategy_* strat_,
                                             const void* inputs[]) {
    WU_PairsTradingStrat strat = (WU_PairsTradingStrat)strat_;
    
    // Validate input count
    assert(strat->base.num_inputs == NUM_INPUTS);
    
    // Cast inputs with proper types
    const WU_Single* asset_a = (const WU_Single*)inputs[0];
    const WU_Single* asset_b = (const WU_Single*)inputs[1];
    
    // Validate input types
    assert(asset_a->data_type == WU_DATA_TYPE_SINGLE_VALUE);
    assert(asset_b->data_type == WU_DATA_TYPE_SINGLE_VALUE);
    
    // Initialize signal buffer with HOLD signals for both assets
    strat->base.signal_buffer[0] = wu_signal_init(asset_a->timestamp, WU_SIDE_HOLD, 
                                                     asset_a->value, 1.0);
    strat->base.signal_buffer[1] = wu_signal_init(asset_b->timestamp, WU_SIDE_HOLD, 
                                                     asset_b->value, 1.0);
    
    // Calculate the spread: spread = asset_a - (ratio * asset_b)
    double spread = asset_a->value - (strat->ratio * asset_b->value);
    
    // Update spread statistics
    double spread_mean = wu_indicator_update(strat->spread_ma, spread);
    double spread_stdev = wu_indicator_update(strat->spread_std, spread);
    
    // Wait for indicators to warm up
    if (isnan(spread_mean) || isnan(spread_stdev))
        return strat->base.signal_buffer;
    
    // Calculate entry/exit thresholds
    double upper_band = spread_mean + strat->threshold * spread_stdev;
    double lower_band = spread_mean - strat->threshold * spread_stdev;
    
    // Pairs trading logic:
    // When spread is below lower band: Asset A is cheap relative to B
    //   → BUY Asset A, SELL Asset B (expecting mean reversion)
    // When spread is above upper band: Asset A is expensive relative to B
    //   → SELL Asset A, BUY Asset B (expecting mean reversion)
    // When spread returns near mean: Close position
    
    if (spread < lower_band && strat->last_signal != WU_SIDE_BUY) {
        // Spread below lower band: Asset A undervalued
        // Signal to BUY Asset A and SELL Asset B
        strat->last_signal = WU_SIDE_BUY;
        strat->base.signal_buffer[0].side = WU_SIDE_BUY;
        strat->base.signal_buffer[1].side = WU_SIDE_SELL;
    } 
    else if (spread > upper_band && strat->last_signal != WU_SIDE_SELL) {
        // Spread above upper band: Asset A overvalued
        // Signal to SELL Asset A and BUY Asset B
        strat->last_signal = WU_SIDE_SELL;
        strat->base.signal_buffer[0].side = WU_SIDE_SELL;
        strat->base.signal_buffer[1].side = WU_SIDE_BUY;
    }
    else if (strat->last_signal != WU_SIDE_HOLD) {
        // Check if spread has mean-reverted (exit condition)
        // Exit when spread crosses back through the mean
        bool spread_reverted = false;
        
        if (strat->last_signal == WU_SIDE_BUY && spread > spread_mean) {
            // We were long (bought at low spread), now spread returned to mean
            spread_reverted = true;
        }
        else if (strat->last_signal == WU_SIDE_SELL && spread < spread_mean) {
            // We were short (sold at high spread), now spread returned to mean
            spread_reverted = true;
        }
        
        if (spread_reverted) {
            // Mean reversion occurred - close position
            // Generate opposite signals to close both positions
            strat->base.signal_buffer[0].side = (strat->last_signal == WU_SIDE_BUY) ? WU_SIDE_SELL : WU_SIDE_BUY;
            strat->base.signal_buffer[1].side = (strat->last_signal == WU_SIDE_BUY) ? WU_SIDE_BUY : WU_SIDE_SELL;
            strat->last_signal = WU_SIDE_HOLD;
        }
    }
    
    return strat->base.signal_buffer;
}

static void pairs_trading_strat_free(struct WU_Strategy_* strategy) {
    WU_PairsTradingStrat strat = (WU_PairsTradingStrat)strategy;
    wu_indicator_delete(strat->spread_ma);
    wu_indicator_delete(strat->spread_std);
    free(strat);
}

WU_PairsTradingStrat wu_pairs_trading_strat_new(int window, double threshold, double ratio) {
    WU_PairsTradingStrat strat = malloc(sizeof(struct WU_PairsTradingStrat_));
    
    // Set up base strategy interface
    strat->base.update = pairs_trading_strat_update;
    strat->base.delete = pairs_trading_strat_free;
    
    // Declare input requirements (point to static const)
    strat->base.input_types = input_types;
    strat->base.num_inputs = NUM_INPUTS;
    
    // Declare output count (symbols set by backtest runner)
    // strat->base.output_symbols = NULL;  // Will be set by runner
    strat->base.num_outputs = NUM_OUTPUTS;
    strat->base.signal_buffer = signal_buffer;
    
    // Initialize spread statistics indicators
    strat->spread_ma = wu_sma_new(window);
    strat->spread_std = wu_mstdev_new(window, 1);  // dof=1 for sample std deviation
    
    // Initialize strategy parameters
    strat->threshold = threshold;
    strat->ratio = ratio;
    strat->last_signal = WU_SIDE_HOLD;
    
    return strat;
}
