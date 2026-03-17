#include <stdlib.h>
#include <math.h>
#include "wu.h"

// Welford's online algorithm for mean and variance
static WU_ReturnStatsResult wu_return_stats_update_impl(WU_ReturnStats self, WU_PerformanceUpdate perf) {
    double portfolio_value = perf.portfolio_value;
    
    if (self->prev_value == 0.0) {
        self->prev_value = portfolio_value;
        self->start_time = perf.timestamp.mark;
        self->time_unit = perf.timestamp.units;
        return (WU_ReturnStatsResult){.mean = NAN, .variance = NAN, .stddev = NAN, .downside_deviation = NAN};
    }
    
    // Calculate return as percentage change
    double ret = (portfolio_value - self->prev_value) / self->prev_value;
    self->prev_value = portfolio_value;
    self->end_time = perf.timestamp.mark;
    
    self->count++;
    
    // Update mean and variance using Welford's algorithm
    double delta = ret - self->return_mean;
    self->return_mean += delta / self->count;
    double delta2 = ret - self->return_mean;
    self->return_m2 += delta * delta2;
    
    // Track downside deviation (only negative returns)
    if (ret < 0.0) {
        self->downside_m2 += ret * ret;
    }
    
    // Compute and return result
    double var = (self->count < 2) ? NAN : self->return_m2 / (self->count - 1);
    double stddev = isnan(var) ? NAN : sqrt(var);
    double downside_dev = (self->count < 1) ? NAN : sqrt(self->downside_m2 / self->count);
    
    return (WU_ReturnStatsResult){
        .mean = self->return_mean,
        .variance = var,
        .stddev = stddev,
        .downside_deviation = downside_dev
    };
}

static WU_ReturnStatsResult wu_return_stats_get_impl(const struct WU_ReturnStats_* self) {
    double var = (self->count < 2) ? NAN : self->return_m2 / (self->count - 1);
    double stddev = isnan(var) ? NAN : sqrt(var);
    double downside_dev = (self->count < 1) ? NAN : sqrt(self->downside_m2 / self->count);
    
    return (WU_ReturnStatsResult){
        .mean = self->return_mean,
        .variance = var,
        .stddev = stddev,
        .downside_deviation = downside_dev
    };
}

static void wu_return_stats_free(WU_ReturnStats self) {
    free(self);
}

WU_ReturnStats wu_return_stats_new(double initial_value) {
    WU_ReturnStats rs = malloc(sizeof(struct WU_ReturnStats_));
    if (!rs) return NULL;
    
    rs->prev_value = initial_value;
    rs->return_mean = 0.0;
    rs->return_m2 = 0.0;
    rs->downside_m2 = 0.0;
    rs->count = 0;
    rs->start_time = 0;
    rs->end_time = 0;
    rs->time_unit = WU_TIME_UNIT_SECONDS;
    
    rs->update = wu_return_stats_update_impl;
    rs->get = wu_return_stats_get_impl;
    rs->delete = wu_return_stats_free;
    
    return rs;
}
