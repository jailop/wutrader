#include <stdlib.h>
#include <math.h>
#include "wu.h"

// Welford's online algorithm for mean and variance of PnL
static WU_PnLStatsResult wu_pnl_stats_update_impl(WU_PnLStats self, double pnl) {
    self->count++;
    // update mean and variance using Welford's algorithm
    double delta = pnl - self->pnl_mean;
    self->pnl_mean += delta / self->count;
    double delta2 = pnl - self->pnl_mean;
    self->pnl_m2 += delta * delta2;
    // compute and return result
    double var = (self->count < 2) ? NAN : self->pnl_m2 / (self->count - 1);
    double stddev = isnan(var) ? NAN : sqrt(var);
    self->value = (WU_PnLStatsResult){
        .mean = self->pnl_mean,
        .stddev = stddev
    };
    return self->value;
}

static void wu_pnl_stats_free(WU_PnLStats self) {
    free(self);
}

WU_PnLStats wu_pnl_stats_new(void) {
    WU_PnLStats ps = malloc(sizeof(struct WU_PnLStats_));
    if (!ps) return NULL;
    ps->pnl_mean = 0.0;
    ps->pnl_m2 = 0.0;
    ps->count = 0;
    ps->update = wu_pnl_stats_update_impl;
    ps->delete = wu_pnl_stats_free;
    ps->value = (WU_PnLStatsResult){.mean = NAN, .stddev = NAN};
    return ps;
}
