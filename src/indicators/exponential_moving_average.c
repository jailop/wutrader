#include <stdlib.h>
#include "wu.h"

double exponential_moving_average_update(ExponentialMovingAverage ema, double new_value) {
    ema->len++;
    if (ema->len < ema->period) {
        ema->prev_value += new_value;
        ema->base.value = NAN;
    } else if (ema->len == ema->period) {
        ema->prev_value += new_value;
        ema->prev_value /= ema->period;
        ema->base.value = ema->prev_value;
    } else {
        ema->prev_value = ema->alpha * new_value + (1 - ema->alpha) * ema->prev_value;
        ema->base.value = ema->prev_value;
    }
    return ema->base.value;
}

ExponentialMovingAverage exponential_moving_average_new(int period, double smoothing) {
    ExponentialMovingAverage ema = malloc(sizeof(struct ExponentialMovingAverage_));
    ema->period = period;
    ema->alpha = smoothing / (period + 1);
    ema->prev_value = 0.0;
    ema->base.value = NAN;
    ema->len = 0;
    ema->base.update = (void (*)(struct Indicator_*, double))exponential_moving_average_update;
    return ema;
}

void inline exponential_moving_average_free(ExponentialMovingAverage ema) {
    free(ema);
}

