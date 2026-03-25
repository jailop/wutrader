#ifndef WU_STRATEGY_H
#define WU_STRATEGY_H

#include "types.h"
#include "data.h"
#include "indicators.h"

typedef struct WU_Strategy_ {
    WU_Signal* (*update)(struct WU_Strategy_* strategy,
            const void* inputs[]);
    void (*delete)(struct WU_Strategy_* strategy);
    const WU_DataType* input_types;
    // const WU_AssetSymbol* output_symbols;
    WU_Signal* signal_buffer;
    int num_inputs;
    int num_outputs;
}* WU_Strategy;

#define WU_STRATEGY(strategy) ((WU_Strategy)(strategy))

#define wu_strategy_update(strategy, inputs) \
    ((strategy)->update((strategy), (inputs)))

#define wu_strategy_delete(strategy) do { \
    if ((strategy)->delete) \
        (strategy)->delete((WU_Strategy)(strategy)); \
} while(0)

#define wu_strategy_num_inputs(strategy) ((strategy)->num_inputs)
#define wu_strategy_input_type(strategy, idx) ((strategy)->input_types[idx])
#define wu_strategy_num_outputs(strategy) ((strategy)->num_outputs)
#define wu_strategy_output_symbol(strategy, idx) \
    ((strategy)->output_symbols[idx])

/**
 * WU_CrossOverStrat is a simple crossover strategy that generates buy and
 * sell signals based on the crossover of two moving averages.
 */
typedef struct WU_CrossOverStrat_ {
    struct WU_Strategy_ base;
    WU_SMA short_ma;
    WU_SMA long_ma;
    double threshold;
    WU_Side last_signal;
}* WU_CrossOverStrat;

WU_CrossOverStrat wu_crossover_strat_new(int short_window, int long_window,
        double threshold);

/**
 * WU_PairsTradingStrat implements a classic pairs trading strategy that
 * trades the spread between two correlated assets (statistical
 * arbitrage).
 * 
 * Strategy Logic:
 * - Calculates the spread: spread = asset_a - (ratio * asset_b)
 * - Tracks the mean and standard deviation of the spread using a
 *   rolling window
 * - Generates BUY signal when spread falls below (mean - threshold *
 *   stdev) → Asset A is undervalued relative to Asset B, so buy A and
 *   sell B
 * - Generates SELL signal when spread rises above (mean + threshold *
 *   stdev) → Asset A is overvalued relative to Asset B, so sell A and
 *   buy B
 * - Generates CLOSE signal when spread returns to mean (exit position)
 * 
 * Typical use cases:
 * - Coca-Cola (KO) vs PepsiCo (PEP)
 * - ExxonMobil (XOM) vs Chevron (CVX)
 * - Gold (GLD) vs Gold Miners (GDX)
 * 
 * Parameters:
 * - window: lookback period for calculating spread statistics
 * - threshold: number of standard deviations for entry signals
 *   (typically 1.5-2.5)
 * - ratio: hedge ratio between the two assets (typically 1.0 or
 *   calculated via regression)
 */
typedef struct WU_PairsTradingStrat_ {
    struct WU_Strategy_ base;
    WU_SMA spread_ma;
    WU_MStDev spread_std;
    double threshold;
    double ratio;
    WU_Side last_signal;
}* WU_PairsTradingStrat;

/**
 * Creates a new pairs trading strategy.
 * 
 * @param window Lookback period for spread statistics
 * @param threshold Number of standard deviations for entry (e.g., 2.0)
 * @param ratio Hedge ratio between assets
 * @return New pairs trading strategy instance
 */
WU_PairsTradingStrat wu_pairs_trading_strat_new(int window, double threshold,
        double ratio);

#endif // WU_STRATEGY_H
