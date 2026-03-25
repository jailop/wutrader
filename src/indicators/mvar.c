#include <stdlib.h>
#include <stdio.h>
#include "wu/indicators.h"

static double update(WU_MVar self, double value) {
    if (isnan(value)) return NAN;
    if (self->len < self->window_size)
        self->len++;
    else {
        double prev = self->prev_values[self->pos];
        self->sum2 -= prev * prev;
    }
    self->prev_values[self->pos] = value;
    self->pos = (self->pos + 1) % self->window_size;
    self->sum2 += value * value;
    double mean = wu_indicator_update(self->sma, value);
    if (self->len < self->window_size) {
        self->value = NAN;
    } else {
        self->value = (self->sum2 - self->window_size * mean * mean) /
            (self->window_size - self->dof);
    }
    return self->value;
}

static void delete(WU_MVar mvar) {
    wu_indicator_delete(mvar->sma);
    free(mvar->prev_values);
    free(mvar);
}

WU_MVar wu_mvar_new(int window_size, int dof) {
    if (window_size <= dof + 1) {
        fprintf(stderr, "Window size must be greater that degrees of freedom\n");
        return NULL;
    }
    WU_MVar mvar = malloc(sizeof(struct WU_MVar_));
    if (!mvar) return NULL;
    
    mvar->prev_values = malloc(window_size * sizeof(double));
    if (!mvar->prev_values) {
        free(mvar);
        return NULL;
    }
    
    mvar->sma = wu_sma_new(window_size);
    if (!mvar->sma) {
        free(mvar->prev_values);
        free(mvar);
        return NULL;
    }
    
    mvar->value = NAN;
    mvar->window_size = window_size;
    mvar->dof = dof;
    for (int i = 0; i < window_size; i++) {
        mvar->prev_values[i] = NAN;
    }
    mvar->sum2 = 0.0;
    mvar->pos = 0;
    mvar->len = 0;
    mvar->update = update;
    mvar->delete = delete;
    return mvar;
}
