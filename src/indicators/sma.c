#include <stdlib.h>
#include "wu.h"

static double wu_sma_update(WU_SMA ma, double value) {
    if (isnan(value)) return ma->value;
    if (ma->len < ma->window_size)
        ma->len++;
    else
        ma->sum -= ma->prev_values[ma->pos];
    ma->sum += value;
    ma->prev_values[ma->pos] = value;
    ma->pos = (ma->pos + 1) % ma->window_size;
    ma->value = ma->len < ma->window_size ? NAN : ma->sum / ma->window_size;
    return ma->value;
}

static inline double wu_sma_value(const struct WU_SMA_ *ma) {
    return ma->value;
}

static void wu_sma_free(WU_SMA ma) {
    free(ma->prev_values);
    free(ma);
}

WU_SMA wu_sma_new(int window_size) {
    WU_SMA ma = malloc(sizeof(struct WU_SMA_));
    ma->value = NAN;
    ma->window_size = window_size;
    ma->prev_values = malloc(window_size * sizeof(double));
    ma->pos = 0;
    ma->len = 0;
    ma->sum = 0.0;
    ma->update = wu_sma_update;
    ma->get = wu_sma_value;
    ma->delete = wu_sma_free;
    return ma;
}
