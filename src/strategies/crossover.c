#include <assert.h>
#include <stdlib.h>
#include "wu.h"

Signal cross_over_strat_update(CrossOverStrat strat, void* new_value) {
    SingleValue* val = (SingleValue*)new_value;
    assert(val->data_type == DATA_TYPE_SINGLE_VALUE);
    indicator_update(strat->short_ma, val->value);
    indicator_update(strat->long_ma, val->value);
    Signal signal = {.timestamp = val->timestamp,
                     .side = SIDE_HOLD,
                     .price = val->value,
                     .quantity = 1.0};
    if (indicator_value(strat->short_ma) == NAN || indicator_value(strat->long_ma) == NAN)
        return signal;
    if ((indicator_value(strat->short_ma) >
         indicator_value(strat->long_ma) * (1.0 + strat->threshold)) &&
        strat->last_signal != SIDE_BUY) {
        strat->last_signal = signal.side = SIDE_BUY;
    } else if ((indicator_value(strat->short_ma) <
                indicator_value(strat->long_ma) * (1.0 - strat->threshold)) &&
               strat->last_signal != SIDE_SELL) {
        strat->last_signal = signal.side = SIDE_SELL;
    }
    return signal;
}

CrossOverStrat cross_over_strat_new(int short_window, int long_window,
                                    double threshold) {
    CrossOverStrat strat = malloc(sizeof(struct CrossOverStrat_));
    
    strat->base.update = (Signal (*)(struct Strategy_*, void*))cross_over_strat_update;
    
    strat->short_ma = moving_average_new(short_window);
    strat->long_ma = moving_average_new(long_window);
    strat->threshold = threshold;
    strat->last_signal = SIDE_HOLD;
    
    return strat;
}

void cross_over_strat_free(CrossOverStrat strat) {
    moving_average_free(strat->short_ma);
    moving_average_free(strat->long_ma);
    free(strat);
}
