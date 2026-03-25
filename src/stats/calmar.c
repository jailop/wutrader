#include <stdlib.h>
#include <math.h>
#include "wu.h"

static double wu_calmar_ratio_update(WU_CalmarRatio self, WU_PerformanceUpdate perf) {
    double portfolio_value = perf.portfolio_value;
    if (self->prev_value == 0.0) {
        self->prev_value = portfolio_value;
        self->start_time = perf.timestamp.mark;
        self->time_unit = perf.timestamp.units;
        return (self->value = NAN);
    }

    double ret = (portfolio_value - self->prev_value) / self->prev_value;
    self->prev_value = portfolio_value;
    self->end_time = perf.timestamp.mark;
    self->count++;

    self->max_drawdown->update(self->max_drawdown, perf.portfolio_value);

    if (self->count < 1 || self->initial_value == 0.0) {
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
    double periods_elapsed = self->end_time - self->start_time;
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
    if (self->max_drawdown)
        self->max_drawdown->delete(self->max_drawdown);
    free(self);
}

WU_CalmarRatio wu_calmar_ratio_new(double initial_value) {
    WU_CalmarRatio cr = malloc(sizeof(struct WU_CalmarRatio_));
    if (!cr) return NULL;
    
    cr->max_drawdown = wu_max_drawdown_new();
    
    if (!cr->max_drawdown) {
        if (cr->max_drawdown) cr->max_drawdown->delete(cr->max_drawdown);
        free(cr);
        return NULL;
    }
    
    cr->initial_value = initial_value;
    cr->value = NAN;
    cr->prev_value = initial_value;
    cr->count = 0;
    cr->start_time = 0;
    cr->end_time = 0;
    cr->time_unit = WU_TIME_UNIT_SECONDS;
    
    cr->update = wu_calmar_ratio_update;
    cr->delete = wu_calmar_ratio_free;
    
    return cr;
}
