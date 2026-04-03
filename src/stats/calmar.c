/**
 * The Calmar Ratio measures risk-adjusted return by comparing annualized
 * returns to the maximum drawdown. Unlike Sharpe/Sortino, it uses maximum
 * drawdown as the risk measure instead of volatility.
 *
 * Formula: Calmar = Annualized_Return / |Maximum_Drawdown|
 * where:
 *
 *   Annualized_Return = (Total_Return)^(1/years) - 1  [compound]
 *   or Total_Return * periods_per_year  [simple]
 *
 * This implementation uses simple annualization for consistency with other
 * risk metrics in this library.
 */

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "wu.h"

/**
 * Minimum observations required before calculating Calmar ratio.
 * This ensures we have enough data for a meaningful drawdown measurement.
 */
#define CALMAR_MIN_OBSERVATIONS 30

/**
 * Updates the Calmar Ratio calculation with a new portfolio value.
 *
 * @param self The Calmar Ratio object
 * @param perf Performance update containing portfolio value and timestamp
 * @return The updated Calmar Ratio value, or NAN if insufficient data
 */
static double wu_calmar_ratio_update(WU_CalmarRatio self,
        WU_PerformanceUpdate perf) {
    if (!self) return NAN;

    double portfolio_value = perf.portfolio_value;

    /* initialize on first observation */
    if (self->count == 0) {
        self->prev_value = portfolio_value;
        self->start_time = perf.timestamp.mark;
        self->time_unit = perf.timestamp.units;
        self->count++;
        self->value = NAN;
        return self->value;
    }

    self->prev_value = portfolio_value;
    self->end_time = perf.timestamp.mark;
    self->count++;

    if (self->max_drawdown) {
        wu_indicator_update(self->max_drawdown, portfolio_value);
    }

    if (self->count < CALMAR_MIN_OBSERVATIONS) {
        self->value = NAN;
        return self->value;
    }

    if (self->initial_value == 0.0) {
        self->value = NAN;
        return self->value;
    }

    double mdd = wu_indicator_get(self->max_drawdown);

    if (isnan(mdd) || mdd >= 0.0) {
        self->value = NAN;
        return self->value;
    }

    double annualization_factor = wu_annualization_factor(
            perf.timestamp.units);
    double periods_elapsed = self->end_time - self->start_time;

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

    /* calculate total return from inception */
    double total_return = (portfolio_value - self->initial_value)
            / self->initial_value;

    double annualized_return = total_return * periods_per_year;

    self->value = annualized_return / fabs(mdd);
    return self->value;
}

/**
 * Frees resources allocated by the Calmar Ratio object.
 */
static void wu_calmar_ratio_free(WU_CalmarRatio self) {
    if (!self) return;
    if (self->max_drawdown) {
        wu_indicator_delete(self->max_drawdown);
    }
    free(self);
}

WU_CalmarRatio wu_calmar_ratio_new(double initial_value) {
    WU_CalmarRatio cr = malloc(sizeof(struct WU_CalmarRatio_));
    if (!cr) return NULL;
    cr->max_drawdown = wu_max_drawdown_new();
    if (!cr->max_drawdown) {
        fprintf(stderr, "Calmar Ratio Error: max_drawdown object creation failed\n");
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
