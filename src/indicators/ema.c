#include <stdlib.h>
#include "wu.h"

static double exponential_moving_average_update(WU_EMA ema, double value) {
    if (isnan(value)) {
        return ema->value;
    }
    ema->len++;
    if (ema->len < ema->period) {
        ema->prev_value += value;
    } else if (ema->len == ema->period) {
        ema->prev_value += value;
        ema->prev_value /= ema->period;
        ema->value = ema->prev_value;
    } else {
        ema->prev_value = ema->alpha * value + (1 - ema->alpha)
            * ema->prev_value;
        ema->value = ema->prev_value;
    }
    return ema->value;
}

static double exponential_moving_average_value(const struct WU_EMA_ *ema) {
    return ema->value;
}

static void exponential_moving_average_free(WU_EMA ema) {
    free(ema);
}

WU_EMA wu_ema_new(int period, double smoothing) {
    WU_EMA ema = malloc(sizeof(struct WU_EMA_));
    ema->value = NAN;
    ema->period = period;
    ema->alpha = smoothing / (period + 1);
    ema->prev_value = 0.0;
    ema->len = 0;
    ema->update = exponential_moving_average_update;
    ema->get = exponential_moving_average_value;
    ema->delete = exponential_moving_average_free;
    return ema;
}


