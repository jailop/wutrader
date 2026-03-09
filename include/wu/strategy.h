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
typedef struct Strategy_ {
    Signal (*update)(struct Strategy_* strategy, const void* data);
    void (*delete)(struct Strategy_* strategy);
}* Strategy;

#define strategy_update(strategy, data) ((strategy)->update((strategy), (data)))

#define strategy_delete(strategy) do { \
    if ((strategy)->delete) \
        (strategy)->delete((Strategy)(strategy)); \
} while(0)

/**
 * CrossOverStrat is a simple crossover strategy that generates buy and
 * sell signals based on the crossover of two moving averages.
 */
typedef struct CrossOverStrat_ {
    struct Strategy_ base;
    MovingAverage short_ma;
    MovingAverage long_ma;
    double threshold;
    Side last_signal;
}* CrossOverStrat;

CrossOverStrat cross_over_strat_new(int short_window, int long_window,
        double threshold);

#endif // WU_STRATEGY_H
