#ifndef WU_INDICATOR_H
#define WU_INDICATOR_H

#include "types.h"
#include <math.h>

/**
 * Base definition for an indicator, which defines the minimal interface
 * for updating the indicator with a new value and a method to free the
 * indicator's resources. The delete method should be called by the
 * module that takes ownership of the indicator.  It is expected that
 * specific indicator implementations will extend this base structure
 * and implement the defined methods.
 */
typedef struct Indicator_ {
    void* (*update)(struct Indicator_* ind, void* new_value);
    void* (*value)(struct Indicator_* ind);
    void (*delete)(struct Indicator_* ind);
}* Indicator;

#define indicator_update(ind, value) do { \
    double __ind_val = (value); \
    if ((ind)->base.update) \
        (ind)->base.update((Indicator)(ind), &__ind_val); \
} while(0)

#define indicator_value(ind) ((ind)->base.value((Indicator)(ind)))

#define DOUBLE(ind) (*(double*)indicator_value(ind))

#define indicator_delete(ind) do { \
    if ((ind)->base.delete) \
        (ind)->base.delete((Indicator)(ind)); \
} while(0)

/**
 * MovingAverage is a simple moving average indicator that calculates
 * the average of the last N values, where N is the window size.
 */
typedef struct MovingAverage_ {
    struct Indicator_ base;
    double* prev_values;
    int window_size;
    int pos;
    int len;
    double sum;
}* MovingAverage;

MovingAverage moving_average_new(int window_size);

/**
 * The exponential moving average (EMA) is a type of moving average that
 * gives more weight to recent values, making it more responsive to
 * recent value changes. The EMA is calculated using a smoothing factor
 * that determines how much weight is given to the most recent value.
 */
typedef struct ExponentialMovingAverage_ {
    struct Indicator_ base;
    double prev_value;
    double alpha;
    int len;
    int period;
}* ExponentialMovingAverage;

ExponentialMovingAverage exponential_moving_average_new(int period, double smoothing);

#endif // WU_INDICATOR_H
