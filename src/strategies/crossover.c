#include <assert.h>
#include <stdlib.h>
#include "wu.h"

WU_Signal cross_over_strat_update(WU_CrossOverStrat strat, const void* new_value) {
    WU_Single* val = (WU_Single*)new_value;
    assert(val->data_type == WU_DATA_TYPE_SINGLE_VALUE);
    wu_indicator_update(strat->short_ma, val->value);
    wu_indicator_update(strat->long_ma, val->value);
    WU_Signal signal = {.timestamp = val->timestamp,
                     .side = WU_SIDE_HOLD,
                     .price = val->value,
                     .quantity = 1.0};
    double short_val = wu_indicator_get(strat->short_ma);
    double long_val = wu_indicator_get(strat->long_ma);
    
    if (isnan(short_val) || isnan(long_val))
        return signal;
    if ((short_val > long_val * (1.0 + strat->threshold)) &&
        strat->last_signal != WU_SIDE_BUY) {
        strat->last_signal = signal.side = WU_SIDE_BUY;
    } else if ((short_val < long_val * (1.0 - strat->threshold)) &&
               strat->last_signal != WU_SIDE_SELL) {
        strat->last_signal = signal.side = WU_SIDE_SELL;
    }
    return signal;
}

static void cross_over_strat_free(struct WU_Strategy_* strategy) {
    WU_CrossOverStrat strat = (WU_CrossOverStrat)strategy;
    wu_indicator_delete(strat->short_ma);
    wu_indicator_delete(strat->long_ma);
    free(strat);
}

WU_CrossOverStrat wu_crossover_strat_new(int short_window, int long_window,
                                    double threshold) {
    WU_CrossOverStrat strat = malloc(sizeof(struct WU_CrossOverStrat_));
    
    strat->base.update = (WU_Signal (*)(struct WU_Strategy_*, const void*))cross_over_strat_update;
    strat->base.delete = cross_over_strat_free;
    
    strat->short_ma = wu_sma_new(short_window);
    strat->long_ma = wu_sma_new(long_window);
    strat->threshold = threshold;
    strat->last_signal = WU_SIDE_HOLD;
    
    return strat;
}


