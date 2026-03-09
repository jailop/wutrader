#include <stdlib.h>
#include "wu.h"

static void* exponential_moving_average_update(struct Indicator_* ind, void* new_value) {
    ExponentialMovingAverage ema = (ExponentialMovingAverage)ind;
    double value = *(double*)new_value;
    
    ema->len++;
    if (ema->len < ema->period) {
        ema->prev_value += value;
    } else if (ema->len == ema->period) {
        ema->prev_value += value;
        ema->prev_value /= ema->period;
    } else {
        ema->prev_value = ema->alpha * value + (1 - ema->alpha) * ema->prev_value;
    }
    return &ema->prev_value;
}

static void* exponential_moving_average_value(struct Indicator_* ind) {
    ExponentialMovingAverage ema = (ExponentialMovingAverage)ind;
    if (ema->len < ema->period) {
        static double nan_value = NAN;
        return &nan_value;
    }
    return &ema->prev_value;
}

static void exponential_moving_average_free(struct Indicator_* ind) {
    free(ind);
}

ExponentialMovingAverage exponential_moving_average_new(int period, double smoothing) {
    ExponentialMovingAverage ema = malloc(sizeof(struct ExponentialMovingAverage_));
    ema->period = period;
    ema->alpha = smoothing / (period + 1);
    ema->prev_value = 0.0;
    ema->len = 0;
    ema->base.update = exponential_moving_average_update;
    ema->base.value = exponential_moving_average_value;
    ema->base.delete = exponential_moving_average_free;
    return ema;
}


