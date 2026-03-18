#include <stdlib.h>
#include <math.h>
#include "wu.h"


static double wu_sharpe_ratio_update(WU_SharpeRatio self, WU_PerformanceUpdate perf) {
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

    WU_PnLStatsResult result = self->return_stats->update(self->return_stats, ret);

    if (isnan(result.stddev) || result.stddev <= 0.0) {
        self->value = NAN;
        return self->value;
    }

    double annualization_factor = wu_annualization_factor(perf.timestamp.units);
    double periods_elapsed = self->end_time - self->start_time;
    if (periods_elapsed <= 0) periods_elapsed = 1;

    double periods_per_year = annualization_factor / periods_elapsed * self->count;
    if (periods_per_year <= 0) periods_per_year = 1;

    double annualized_stddev = result.stddev * sqrt(periods_per_year);
    double per_period_rf = self->risk_free_rate / periods_per_year;

    self->value = (result.mean - per_period_rf) / annualized_stddev;
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

    sr->return_stats = wu_pnl_stats_new();
    if (!sr->return_stats) {
        free(sr);
        return NULL;
    }

    sr->risk_free_rate = risk_free_rate;
    sr->value = NAN;
    sr->prev_value = initial_value;
    sr->count = 0;
    sr->start_time = 0;
    sr->end_time = 0;
    sr->time_unit = WU_TIME_UNIT_SECONDS;

    sr->update = wu_sharpe_ratio_update;
    sr->delete = wu_sharpe_ratio_free;

    return sr;
}
