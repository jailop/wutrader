/**
 * The Sharpe Ratio measures risk-adjusted return by comparing excess return
 * (over risk-free rate) to total volatility (standard deviation).
 *
 * Formula: Sharpe = (R_p - R_f) / σ_p
 * where:
 *
 *   R_p = mean portfolio return (annualized)
 *   R_f = risk-free rate (annualized)
 *   σ_p = portfolio standard deviation (annualized)
 */

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "wu.h"

/**
 * Minimum number of observations required for statistically meaningful
 * Sharpe ratio calculation. Below this threshold, the ratio is unreliable.
 */
#define SHARPE_MIN_OBSERVATIONS 30

/**
 * Updates the Sharpe Ratio calculation with a new portfolio value.
 *
 * @param self The Sharpe Ratio object
 * @param perf Performance update containing portfolio value and timestamp
 * @return The updated Sharpe Ratio value, or NAN if insufficient data
 */
static double wu_sharpe_ratio_update(WU_SharpeRatio self,
        WU_PerformanceUpdate perf) {
    if (!self) return NAN;

    double portfolio_value = perf.portfolio_value;

    /* initialize on first observation */
    if (self->count == 0) {
        self->start_time = perf.timestamp.mark;
        self->time_unit = perf.timestamp.units;
        self->prev_value = portfolio_value;
        self->count++;
        self->value = NAN;
        return self->value;
    }

    /* calculate simple return from previous period */
    double ret = (portfolio_value - self->prev_value) / self->prev_value;
    self->prev_value = portfolio_value;
    self->end_time = perf.timestamp.mark;
    self->count++;

    /* update running statistics */
    double mean = wu_indicator_update(self->mean, ret);
    double stdev = wu_indicator_update(self->stdev, ret);

    /* need minimum observations for reliable calculation */
    if (self->count < SHARPE_MIN_OBSERVATIONS) {
        self->value = NAN;
        return self->value;
    }

    /* validate standard deviation */
    if (isnan(stdev) || stdev <= 0.0) {
        self->value = NAN;
        return self->value;
    }

    /* calculate annualization factor */
    double annualization_factor = wu_annualization_factor(
            perf.timestamp.units);
    double periods_elapsed = self->end_time - self->start_time;

    /* avoid division by zero or negative time */
    if (periods_elapsed <= 0) {
        self->value = NAN;
        return self->value;
    }

    double periods_per_year = (annualization_factor * self->count)
            / periods_elapsed;
    if (periods_per_year <= 0) {
        self->value = NAN;
        return self->value;
    }

    double annualized_stdev = stdev * sqrt(periods_per_year);
    double per_period_rf = self->risk_free_rate / periods_per_year;
    self->value = (mean - per_period_rf) / annualized_stdev;
    return self->value;
}

/**
 * Frees resources allocated by the Sharpe Ratio object.
 */
static void wu_sharpe_ratio_free(WU_SharpeRatio self) {
    if (!self) return;
    wu_indicator_delete(self->mean);
    wu_indicator_delete(self->stdev);
    free(self);
}

/**
 * Creates a new Sharpe Ratio calculator.
 */
WU_SharpeRatio wu_sharpe_ratio_new(double initial_value,
        double risk_free_rate) {
    WU_SharpeRatio sr = malloc(sizeof(struct WU_SharpeRatio_));
    if (!sr) return NULL;

    sr->mean = wu_mean_new();
    if (!sr->mean) {
        fprintf(stderr, "Sharpe Ratio Error: mean object creation failed\n");
        free(sr);
        return NULL;
    }

    sr->stdev = wu_stdev_new(1);  /* Sample standard deviation (dof=1) */
    if (!sr->stdev) {
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
    sr->time_unit = WU_TIME_UNIT_SECONDS;
    sr->update = wu_sharpe_ratio_update;
    sr->delete = wu_sharpe_ratio_free;

    return sr;
}
