#include <assert.h>
#include <stdlib.h>
#include "wu.h"

#define NUM_INPUTS 1
#define NUM_OUTPUTS 1

static const WU_DataType input_types[] = {WU_DATA_TYPE_SINGLE_VALUE};
static WU_Signal signal_buffer[NUM_OUTPUTS];

static WU_Signal* update(WU_Strategy strat_, const void* inputs[]) {
    WU_CrossOverStrat strat = (WU_CrossOverStrat)strat_;
    const WU_Single* val = (const WU_Single*)inputs[0];
    assert(val->data_type == WU_DATA_TYPE_SINGLE_VALUE);
    double short_val = wu_indicator_update(strat->short_ma, val->value);
    double long_val = wu_indicator_update(strat->long_ma, val->value);
    signal_buffer[0] = (WU_Signal){
        .timestamp = val->timestamp,
        .side = WU_SIDE_HOLD,
        .price = val->value,
        .quantity = 1.0
    };
    if (isnan(short_val) || isnan(long_val)) {
        return signal_buffer;
    }
    if ((short_val > long_val * (1.0 + strat->threshold)) &&
        strat->last_signal != WU_SIDE_BUY) {
        strat->last_signal = signal_buffer[0].side = WU_SIDE_BUY;
    } else if ((short_val < long_val * (1.0 - strat->threshold)) &&
               strat->last_signal != WU_SIDE_SELL) {
        strat->last_signal = signal_buffer[0].side = WU_SIDE_SELL;
    }
    return signal_buffer;
}

static void delete(struct WU_Strategy_* strategy) {
    WU_CrossOverStrat strat = (WU_CrossOverStrat)strategy;
    wu_indicator_delete(strat->short_ma);
    wu_indicator_delete(strat->long_ma);
    free(strat);
}

WU_CrossOverStrat wu_crossover_strat_new(int short_window, int long_window,
                                    double threshold) {
    WU_CrossOverStrat strat = malloc(sizeof(struct WU_CrossOverStrat_));
    strat->base.update = update;
    strat->base.delete = delete;
    strat->base.input_types = input_types;
    strat->base.num_inputs = NUM_INPUTS;
    // strat->base.output_symbols = NULL;
    strat->base.num_outputs = NUM_OUTPUTS;
    strat->base.signal_buffer = signal_buffer;
    strat->short_ma = wu_sma_new(short_window);
    strat->long_ma = wu_sma_new(long_window);
    strat->threshold = threshold;
    strat->last_signal = WU_SIDE_HOLD;
    return strat;
}


