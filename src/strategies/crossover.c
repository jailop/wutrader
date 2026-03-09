#include <assert.h>
#include <stdlib.h>
#include "wu.h"

Signal cross_over_strat_update(CrossOverStrat strat, const void* new_value) {
    SingleValue* val = (SingleValue*)new_value;
    assert(val->data_type == DATA_TYPE_SINGLE_VALUE);
    indicator_update(strat->short_ma, val->value);
    indicator_update(strat->long_ma, val->value);
    Signal signal = {.timestamp = val->timestamp,
                     .side = SIDE_HOLD,
                     .price = val->value,
                     .quantity = 1.0};
    double short_val = DOUBLE(strat->short_ma);
    double long_val = DOUBLE(strat->long_ma);
    
    if (isnan(short_val) || isnan(long_val))
        return signal;
    if ((short_val > long_val * (1.0 + strat->threshold)) &&
        strat->last_signal != SIDE_BUY) {
        strat->last_signal = signal.side = SIDE_BUY;
    } else if ((short_val < long_val * (1.0 - strat->threshold)) &&
               strat->last_signal != SIDE_SELL) {
        strat->last_signal = signal.side = SIDE_SELL;
    }
    return signal;
}

static void cross_over_strat_free(struct Strategy_* strategy) {
    CrossOverStrat strat = (CrossOverStrat)strategy;
    indicator_delete(strat->short_ma);
    indicator_delete(strat->long_ma);
    free(strat);
}

CrossOverStrat cross_over_strat_new(int short_window, int long_window,
                                    double threshold) {
    CrossOverStrat strat = malloc(sizeof(struct CrossOverStrat_));
    
    strat->base.update = (Signal (*)(struct Strategy_*, const void*))cross_over_strat_update;
    strat->base.delete = cross_over_strat_free;
    
    strat->short_ma = moving_average_new(short_window);
    strat->long_ma = moving_average_new(long_window);
    strat->threshold = threshold;
    strat->last_signal = SIDE_HOLD;
    
    return strat;
}


