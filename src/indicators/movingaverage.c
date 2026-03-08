#include <stdlib.h>
#include "wu.h"

double moving_average_update(MovingAverage ma, double new_value) {
    if (ma->len < ma->window_size)
        ma->len++;
    else
        ma->sum -= ma->prev_values[ma->pos];
    ma->sum += new_value;
    ma->prev_values[ma->pos] = new_value;
    ma->pos = (ma->pos + 1) % ma->window_size;
    ma->base.value = ma->len < ma->window_size ? NAN : ma->sum / ma->window_size;
    return ma->base.value;
}

MovingAverage moving_average_new(int window_size) {
    MovingAverage ma = malloc(sizeof(struct MovingAverage_));
    ma->window_size = window_size;
    ma->prev_values = malloc(window_size * sizeof(double));
    ma->pos = 0;
    ma->len = 0;
    ma->sum = 0.0;
    ma->base.value = NAN;
    ma->base.update = (void (*)(struct Indicator_*, double))moving_average_update;
    return ma;
}

void inline moving_average_free(MovingAverage ma) {
    free(ma->prev_values);
    free(ma);
}


