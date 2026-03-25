#include <stdlib.h>
#include "wu.h"

static double update(WU_SMA ma, double value) {
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

static void delete(WU_SMA ma) {
    free(ma->prev_values);
    free(ma);
}

WU_SMA wu_sma_new(int window_size) {
    WU_SMA ma = malloc(sizeof(struct WU_SMA_));
    if (!ma) return NULL;
    
    ma->prev_values = malloc(window_size * sizeof(double));
    if (!ma->prev_values) {
        free(ma);
        return NULL;
    }
    
    ma->value = NAN;
    ma->window_size = window_size;
    for (int i = 0; i < window_size; i++) {
        ma->prev_values[i] = NAN;
    }
    ma->pos = 0;
    ma->len = 0;
    ma->sum = 0.0;
    ma->update = update;
    ma->delete = delete;
    return ma;
}
