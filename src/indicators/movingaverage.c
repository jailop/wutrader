#include <stdlib.h>
#include "wu.h"

static void* moving_average_update(struct Indicator_* ind, void* new_value) {
    MovingAverage ma = (MovingAverage)ind;
    double value = *(double*)new_value;
    
    if (ma->len < ma->window_size)
        ma->len++;
    else
        ma->sum -= ma->prev_values[ma->pos];
    ma->sum += value;
    ma->prev_values[ma->pos] = value;
    ma->pos = (ma->pos + 1) % ma->window_size;
    return &ma->sum;
}

static void* moving_average_value(struct Indicator_* ind) {
    MovingAverage ma = (MovingAverage)ind;
    if (ma->len < ma->window_size) {
        static double nan_value = NAN;
        return &nan_value;
    }
    static double result;
    result = ma->sum / ma->window_size;
    return &result;
}

static void moving_average_free(struct Indicator_* ind) {
    MovingAverage ma = (MovingAverage)ind;
    free(ma->prev_values);
    free(ma);
}

MovingAverage moving_average_new(int window_size) {
    MovingAverage ma = malloc(sizeof(struct MovingAverage_));
    ma->window_size = window_size;
    ma->prev_values = malloc(window_size * sizeof(double));
    ma->pos = 0;
    ma->len = 0;
    ma->sum = 0.0;
    ma->base.update = moving_average_update;
    ma->base.value = moving_average_value;
    ma->base.delete = moving_average_free;
    return ma;
}


