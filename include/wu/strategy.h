#ifndef WU_STRATEGY_H
#define WU_STRATEGY_H

#include "types.h"
#include "data.h"
#include "indicator.h"

/**
 * Base for a strategy, which defines the minimal interface for updating
 * the strategy with new data and generating a trading signal, as well
 * as a method to free the strategy's resources. The delete method
 * should be called by the runner taking ownership of the strategy.  It
 * is expected that specific strategy implementations will extend this
 * base structure and implement the defined methods.
 */
typedef struct WU_Strategy_ {
    WU_Signal (*update)(struct WU_Strategy_* strategy, const void* data);
    void (*delete)(struct WU_Strategy_* strategy);
}* WU_Strategy;

#define wu_strategy_update(strategy, data) ((strategy)->update((strategy), (data)))

#define wu_strategy_delete(strategy) do { \
    if ((strategy)->delete) \
        (strategy)->delete((WU_Strategy)(strategy)); \
} while(0)

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

#endif // WU_STRATEGY_H
