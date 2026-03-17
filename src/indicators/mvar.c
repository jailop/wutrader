#include <stdlib.h>
#include "wu/indicators.h"

static double update(WU_MVar mvar, double value) {
    if (isnan(value)) return NAN;
    if (mvar->len < mvar->window_size)
        mvar->len++;
    mvar->prev_values[mvar->pos] = value;
    mvar->pos = (mvar->pos + 1) % mvar->window_size;
    wu_indicator_update(mvar->sma, value);
    if (mvar->len < mvar->window_size)
        mvar->value = NAN;
    double accum = 0.0;
    for (int i = 0; i < mvar->len; i++) {
        if (isnan(mvar->prev_values[i]))
            return NAN;
        double diff = mvar->prev_values[i] - wu_indicator_get(mvar->sma);
        accum += diff * diff;
    }
    mvar->value = accum / (mvar->window_size - mvar->dof);
    return mvar->value;
}

static inline double get(const struct WU_MVar_ *mvar) {
    return mvar->value;
}

static void delete(WU_MVar mvar) {
    wu_indicator_delete(mvar->sma);
    free(mvar->prev_values);
    free(mvar);
}

WU_MVar wu_mvar_new(int window_size, int dof) {
    WU_MVar mvar = malloc(sizeof(struct WU_MVar_));
    mvar->value = NAN;
    mvar->window_size = window_size;
    mvar->dof = dof;
    mvar->prev_values = malloc(window_size * sizeof(double));
    mvar->pos = 0;
    mvar->len = 0;
    mvar->sma = wu_sma_new(window_size);
    mvar->update = update;
    mvar->get = get;
    mvar->delete = delete;
    return mvar;
}
