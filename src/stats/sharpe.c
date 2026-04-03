#include <stdlib.h>
#include <math.h>
#include "wu.h"

static double wu_sharpe_ratio_update(WU_SharpeRatio self,
        WU_PerformanceUpdate perf) {
    if (self->count == 0) {
        self->start_time = perf.timestamp.mark;
        self->time_unit = perf.timestamp.units;
    }
    double ret = (perf.portfolio_value - self->initial_value)
            / self->initial_value;
    self->end_time = perf.timestamp.mark;
    self->count++;
    double mean = wu_indicator_update(self->mean, ret);
    double stdev = wu_indicator_update(self->stdev, ret);
    if (isnan(stdev) || stdev <= 0.0) {
        self->value = NAN;
        return self->value;
    }
    double annualization_factor = wu_annualization_factor(perf.timestamp.units);
    double periods_elapsed = self->end_time - self->start_time;
    if (periods_elapsed <= 0) periods_elapsed = 1;
    double periods_per_year = annualization_factor / periods_elapsed 
            * self->count;
    if (periods_per_year <= 0) periods_per_year = 1;
    double annualized_stdev = stdev * sqrt(periods_per_year);
    double per_period_rf = self->risk_free_rate / periods_per_year;
    self->value = (mean - per_period_rf) / annualized_stdev;
    return self->value;
}

static void wu_sharpe_ratio_free(WU_SharpeRatio self) {
    wu_indicator_delete(self->mean);
    wu_indicator_delete(self->stdev);
    if (self) free(self);
}

WU_SharpeRatio wu_sharpe_ratio_new(double initial_value, double risk_free_rate) {
    WU_SharpeRatio sr = malloc(sizeof(struct WU_SharpeRatio_));
    if (!sr) return NULL;
    if (!(sr->mean = wu_mean_new())) {
        fprintf(stderr, "Sharpe Ratio Error: mean object creation failed\n");
        free(sr);
        return NULL;
    }
    if (!(sr->stdev = wu_stdev_new(1))) {
        fprintf(stderr, "Sharpe Ratio Error: stdev object creation failed\n");
        wu_indicator_delete(sr->mean);
        free(sr);
        return NULL;
    }
    sr->risk_free_rate = risk_free_rate;
    sr->initial_value = initial_value;
    sr->value = NAN;
    sr->prev_value = initial_value;
    sr->count = 0;
    sr->start_time = 0;
    sr->end_time = 0;
    sr->time_unit = WU_TIME_UNIT_SECONDS; // todo: it seems it shouldn't be here
    sr->update = wu_sharpe_ratio_update;
    sr->delete = wu_sharpe_ratio_free;
    return sr;
}
