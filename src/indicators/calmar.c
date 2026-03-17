#include <stdlib.h>
#include <math.h>
#include "wu.h"

// Annualization factor based on time unit
static double wu_annualization_factor(WU_TimeUnit unit) {
    switch (unit) {
        case WU_TIME_UNIT_SECONDS:    return 365.25 * 24 * 3600;
        case WU_TIME_UNIT_MILLIS:     return 365.25 * 24 * 3600 * 1000;
        case WU_TIME_UNIT_MICROS:     return 365.25 * 24 * 3600 * 1e6;
        case WU_TIME_UNIT_NANOS:      return 365.25 * 24 * 3600 * 1e9;
        default:                      return 365.25;
    }
}

static double wu_calmar_ratio_update(WU_CalmarRatio self, WU_PerformanceUpdate perf) {
    self->return_stats->update(self->return_stats, perf);
    self->max_drawdown->update(self->max_drawdown, perf.portfolio_value);
    
    if (self->return_stats->count < 2 || self->initial_value == 0.0) {
        self->value = NAN;
        return self->value;
    }
   
    double mdd = wu_indicator_get(self->max_drawdown);
    
    // MDD is zero or positive means no drawdown occurred
    if (mdd >= 0.0) {
        self->value = NAN;
        return self->value;
    }
    
    // Annualize return based on time period
    double annualization_factor = wu_annualization_factor(perf.timestamp.units);
    double periods_elapsed = self->return_stats->end_time - self->return_stats->start_time;
    if (periods_elapsed <= 0) periods_elapsed = 1;
    
    double periods_per_year = annualization_factor / periods_elapsed;
    if (periods_per_year <= 0) periods_per_year = 1;
    
    // Calculate total return
    double total_return = (perf.portfolio_value - self->initial_value) / self->initial_value;
    
    // Annualize the return
    double annualized_return = total_return * periods_per_year;
    
    // Calmar = annualized_return / abs(max_drawdown)
    self->value = annualized_return / fabs(mdd);
    return self->value;
}

static void wu_calmar_ratio_free(WU_CalmarRatio self) {
    if (self->return_stats)
        self->return_stats->delete(self->return_stats);
    if (self->max_drawdown)
        self->max_drawdown->delete(self->max_drawdown);
    free(self);
}

WU_CalmarRatio wu_calmar_ratio_new(double initial_value) {
    WU_CalmarRatio cr = malloc(sizeof(struct WU_CalmarRatio_));
    if (!cr) return NULL;
    
    cr->return_stats = wu_return_stats_new(initial_value);
    cr->max_drawdown = wu_max_drawdown_new();
    
    if (!cr->return_stats || !cr->max_drawdown) {
        if (cr->return_stats) cr->return_stats->delete(cr->return_stats);
        if (cr->max_drawdown) cr->max_drawdown->delete(cr->max_drawdown);
        free(cr);
        return NULL;
    }
    
    cr->initial_value = initial_value;
    cr->value = NAN;
    
    cr->update = wu_calmar_ratio_update;
    cr->delete = wu_calmar_ratio_free;
    
    return cr;
}
