/**
 * The Sortino Ratio is a variation of the Sharpe Ratio that differentiates
 * between harmful volatility (downside) and total volatility. It uses only
 * the standard deviation of negative returns (downside deviation) in the
 * denominator.
 *
 * Formula: Sortino = (R_p - R_f) / σ_d
 * where:
 *
 *   R_p = mean portfolio return (annualized)
 *   R_f = risk-free rate (annualized)
 *   σ_d = downside deviation (annualized)
 */

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "wu.h"

/**
 * Minimum number of observations required for statistically meaningful
 * Sortino ratio calculation. Below this threshold, the ratio is unreliable.
 */
#define SORTINO_MIN_OBSERVATIONS 30

/**
 * Updates the Sortino Ratio calculation with a new portfolio value.
 */
static double wu_sortino_ratio_update(WU_SortinoRatio self,
        WU_PerformanceUpdate perf) {
    if (!self) return NAN;

    double portfolio_value = perf.portfolio_value;

    /* Initialize on first observation */
    if (self->count == 0) {
        self->start_time = perf.timestamp.mark;
        self->time_unit = perf.timestamp.units;
        self->prev_value = portfolio_value;
        self->count++;
        self->value = NAN;
        return self->value;
    }

    /* Calculate simple return from previous period */
    double ret = (portfolio_value - self->prev_value) / self->prev_value;
    self->prev_value = portfolio_value;
    self->end_time = perf.timestamp.mark;
    self->count++;

    /* Update running statistics */
    double mean = wu_indicator_update(self->mean, ret);
    double downside_dev = wu_indicator_update(self->downside, ret);

    /* Need minimum observations for reliable calculation */
    if (self->count < SORTINO_MIN_OBSERVATIONS) {
        self->value = NAN;
        return self->value;
    }

    /* Validate downside deviation */
    if (isnan(downside_dev) || downside_dev <= 0.0) {
        self->value = NAN;
        return self->value;
    }

    /* Calculate annualization factor */
    double annualization_factor = wu_annualization_factor(
            perf.timestamp.units);
    double periods_elapsed = self->end_time - self->start_time;

    /* Avoid division by zero or negative time */
    if (periods_elapsed <= 0) {
        self->value = NAN;
        return self->value;
    }

    /* Calculate periods per year based on observed frequency */
    double periods_per_year = (annualization_factor * self->count)
            / periods_elapsed;
    if (periods_per_year <= 0) {
        self->value = NAN;
        return self->value;
    }

    double annualized_downside_dev = downside_dev * sqrt(periods_per_year);
    double per_period_rf = self->risk_free_rate / periods_per_year;
    self->value = (mean - per_period_rf) / annualized_downside_dev;
    return self->value;
}

/**
 * Frees resources allocated by the Sortino Ratio object.
 */
static void wu_sortino_ratio_free(WU_SortinoRatio self) {
    if (!self) return;
    wu_indicator_delete(self->mean);
    wu_indicator_delete(self->downside);
    free(self);
}

/**
 * Creates a new Sortino Ratio calculator.
 */
WU_SortinoRatio wu_sortino_ratio_new(double initial_value,
        double risk_free_rate) {
    WU_SortinoRatio sr = malloc(sizeof(struct WU_SortinoRatio_));
    if (!sr) return NULL;

    sr->mean = wu_mean_new();
    if (!sr->mean) {
        fprintf(stderr, "Sortino Ratio Error: mean object creation failed\n");
        free(sr);
        return NULL;
    }

    sr->downside = wu_downside_new();
    if (!sr->downside) {
        fprintf(stderr, "Sortino Ratio Error: downside object creation failed\n");
        wu_indicator_delete(sr->mean);
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
    sr->update = wu_sortino_ratio_update;
    sr->delete = wu_sortino_ratio_free;

    return sr;
}
