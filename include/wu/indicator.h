#ifndef WU_INDICATOR_H
#define WU_INDICATOR_H

#include "types.h"
#include <stdlib.h>
#include <math.h>

#define wu_indicator_update(indicator, value) \
    (indicator)->update((indicator), (value))
#define wu_indicator_get(indicator) (indicator)->get((indicator))
#define wu_indicator_delete(indicator) (indicator)->delete((indicator))

/**
 * MovingAverage is a simple moving average indicator that calculates
 * the average of the last N values, where N is the window size.
 */
typedef struct WU_SMA_ {
    double (*update)(struct WU_SMA_ *self, double value);
    double (*get)(const struct WU_SMA_ *self);
    void (*delete)(struct WU_SMA_ *self);
    double value;
    double* prev_values;
    int window_size;
    int pos;
    int len;
    double sum;
}* WU_SMA;

WU_SMA wu_sma_new(int window_size);

/**
 * The exponential moving average (WU_EMA) is a type of moving average that
 * gives more weight to recent values, making it more responsive to
 * recent value changes. The WU_EMA is calculated using a smoothing factor
 * that determines how much weight is given to the most recent value.
 */
typedef struct WU_EMA_ {
    double (*update)(struct WU_EMA_ *self, double value);
    double (*get)(const struct WU_EMA_ *self);
    void (*delete)(struct WU_EMA_ *self);
    double value;
    double prev_value;
    double alpha;
    int len;
    int period;
}* WU_EMA;

WU_EMA wu_ema_new(int period,
        double smoothing);

typedef struct WU_MVar {
    double (*update)(struct WU_MVar *self, double value);
    double (*get)(const struct WU_MVar *self);
    void (*delete)(struct WU_MVar *self);
    double value;
    WU_SMA sma;
    double* prev_values;
    int pos;
    int len;
    int dof;
}* WU_MVar;

WU_MVar wu_mvar_new(int window_size, int dof);

#endif // WU_INDICATOR_H
