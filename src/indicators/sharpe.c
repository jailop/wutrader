#include <stdlib.h>
#include <math.h>
#include "wu.h"

// Annualization factor based on time unit
// Returns periods per year for the given time unit
static double wu_annualization_factor(WU_TimeUnit unit) {
    switch (unit) {
        case WU_TIME_UNIT_SECONDS:    return 365.25 * 24 * 3600;      // seconds per year
        case WU_TIME_UNIT_MILLIS:     return 365.25 * 24 * 3600 * 1000; // millis per year
        case WU_TIME_UNIT_MICROS:     return 365.25 * 24 * 3600 * 1e6;  // micros per year
        case WU_TIME_UNIT_NANOS:      return 365.25 * 24 * 3600 * 1e9;  // nanos per year
        default:                      return 365.25;                   // daily default
    }
}

static double wu_sharpe_ratio_update(WU_SharpeRatio self, WU_PerformanceUpdate perf) {
    WU_ReturnStatsResult result = self->return_stats->update(self->return_stats, perf);
    
    if (isnan(result.stddev) || result.stddev <= 0.0) {
        self->value = NAN;
        return self->value;
    }
    
    // Annualize metrics based on time period
    double annualization_factor = wu_annualization_factor(perf.timestamp.units);
    double periods_elapsed = self->return_stats->end_time - self->return_stats->start_time;
    if (periods_elapsed <= 0) periods_elapsed = 1;
    
    double periods_per_year = annualization_factor / periods_elapsed * self->return_stats->count;
    if (periods_per_year <= 0) periods_per_year = 1;
    
    double annualized_stddev = result.stddev * sqrt(periods_per_year);
    
    // Sharpe = (mean_return - risk_free_rate) / annualized_stddev
    // Convert annual risk-free rate to per-period
    double per_period_rf = self->risk_free_rate / periods_per_year;
    
    self->value = (result.mean - per_period_rf) / annualized_stddev;
    return self->value;
}

static double wu_sharpe_ratio_get(const struct WU_SharpeRatio_* self) {
    return self->value;
}

static void wu_sharpe_ratio_free(WU_SharpeRatio self) {
    if (self->return_stats)
        self->return_stats->delete(self->return_stats);
    free(self);
}

WU_SharpeRatio wu_sharpe_ratio_new(double initial_value, double risk_free_rate) {
    WU_SharpeRatio sr = malloc(sizeof(struct WU_SharpeRatio_));
    if (!sr) return NULL;
    
    sr->return_stats = wu_return_stats_new(initial_value);
    if (!sr->return_stats) {
        free(sr);
        return NULL;
    }
    
    sr->risk_free_rate = risk_free_rate;
    sr->value = NAN;
    
    sr->update = wu_sharpe_ratio_update;
    sr->get = wu_sharpe_ratio_get;
    sr->delete = wu_sharpe_ratio_free;
    
    return sr;
}
