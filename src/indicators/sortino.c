#include <stdlib.h>
#include <math.h>
#include "wu.h"

static double wu_sortino_ratio_update(WU_SortinoRatio self, WU_PerformanceUpdate perf) {
    WU_ReturnStatsResult result = self->return_stats->update(self->return_stats, perf);
    
    if (isnan(result.downside_deviation) || result.downside_deviation <= 0.0) {
        self->value = NAN;
        return self->value;
    }
    
    // Annualize metrics based on time period
    double annualization_factor = wu_annualization_factor(perf.timestamp.units);
    double periods_elapsed = self->return_stats->end_time - self->return_stats->start_time;
    if (periods_elapsed <= 0) periods_elapsed = 1;
    
    double periods_per_year = annualization_factor / periods_elapsed * self->return_stats->count;
    if (periods_per_year <= 0) periods_per_year = 1;
    
    double annualized_downside_dev = result.downside_deviation * sqrt(periods_per_year);
    
    // Sortino = (mean_return - risk_free_rate) / annualized_downside_deviation
    double per_period_rf = self->risk_free_rate / periods_per_year;
    
    self->value = (result.mean - per_period_rf) / annualized_downside_dev;
    return self->value;
}

static void wu_sortino_ratio_free(WU_SortinoRatio self) {
    if (self->return_stats)
        self->return_stats->delete(self->return_stats);
    free(self);
}

WU_SortinoRatio wu_sortino_ratio_new(double initial_value, double risk_free_rate) {
    WU_SortinoRatio sr = malloc(sizeof(struct WU_SortinoRatio_));
    if (!sr) return NULL;
    
    sr->return_stats = wu_return_stats_new(initial_value);
    if (!sr->return_stats) {
        free(sr);
        return NULL;
    }
    
    sr->risk_free_rate = risk_free_rate;
    sr->value = NAN;
    
    sr->update = wu_sortino_ratio_update;
    sr->delete = wu_sortino_ratio_free;
    
    return sr;
}
