#ifndef WU_INDICATOR_H
#define WU_INDICATOR_H

/**
 * Header file for technical indicators. Defines the structures and
 * function prototypes for various indicators. All the indicator should
 * follow the same interface:
 *
 * - `update`: A function that takes a new value (or candle) and updates
 *   the indicator's state, returning the new indicator value.
 * - `delete`: A function that frees any resources allocated by the
 *   indicator.
 * - `value`: A field that holds the current value of the indicator.
 *
 * Once the indicator has been created, it should only be accessed using
 * the provided macros:
 *
 * - `wu_indicator_update(indicator, value)`: Updates the indicator with
 *   a new value and returns the updated indicator value.
 * - `wu_indicator_get(indicator)`: Retrieves the current value of the
 *   indicator.
 * - `wu_indicator_delete(indicator)`: Frees any resources allocated by
 *   the indicator.
 *
 * This design allows for a consistent interface across different types
 * of indicators, making it easier to use them interchangeably in
 * trading strategies.
 */

#include "types.h"
#include "data.h"
#include <stdlib.h>
#include <math.h>

/**
 * Update the indicator state with a new value a return the updated
 * indicator state. This macro assumes that the indicator has an
 * `update` function that takes the indicator itself and a new value,
 * and returns the updated indicator value.
 */
#define wu_indicator_update(indicator, value) \
    (indicator)->update((indicator), (value))

/**
 * Get the current value of the indicator. This macro assume that the
 * indicator has a `value` field that holds the current state of the
 * indicator.
 */
#define wu_indicator_get(indicator) ((indicator)->value)

/**
 * Delete the indicator and free any resources allocated by it. This
 * macro assumes that the indicator has a `delete` function that takes
 * the indicator itself and frees any resources allocated by it.
 */
#define wu_indicator_delete(indicator) (indicator)->delete((indicator))

/**
 * MovingAverage is a simple moving average indicator that calculates
 * the average of the last N values, where N is the window size.
 */
typedef struct WU_SMA_ {
    double (*update)(struct WU_SMA_ *self, double value);
    void (*delete)(struct WU_SMA_ *self);
    double value;
    double* prev_values;
    int window_size;
    int pos;
    int len;
    double sum;
}* WU_SMA;

/**
 * Creates a new WU_SMA (Simple Moving Average) indicator with the
 * specified window size.
 */
WU_SMA wu_sma_new(int window_size);

/**
 * The exponential moving average (WU_EMA) is a type of moving average that
 * gives more weight to recent values, making it more responsive to
 * recent value changes. The WU_EMA is calculated using a smoothing factor
 * that determines how much weight is given to the most recent value.
 */
typedef struct WU_EMA_ {
    double (*update)(struct WU_EMA_ *self, double value);
    void (*delete)(struct WU_EMA_ *self);
    double value;
    double prev_value;
    double alpha;
    int len;
    int period;
}* WU_EMA;

/**
 * Creates a new WU_EMA (Exponential Moving Average) indicator with the
 * specified period and smoothing factor. Usually, 2.0 is used as the
 * smoothing factor.
 */
WU_EMA wu_ema_new(int window_size, double smoothing);

/**
 * The WU_MVar (Moving Variance) is an indicator that calculates the variance
 * of the last N values, where N is the window size. It is used to measure
 * the volatility of a time series. `dof` stands for "degree of freedom"
 * and is used to adjust the variance calculation. If `dof` is 0, the
 * variance is calculated using the population variance formula. If
 * `dof` is 1, the variance is calculated using the sample variance
 * formula.
 */
typedef struct WU_MVar_ *WU_MVar;

struct WU_MVar_ {
    double (*update)(struct WU_MVar_ *self, double value);
    void (*delete)(struct WU_MVar_ *self);
    double value;
    WU_SMA sma;
    double* prev_values;
    double sum2;
    int window_size;
    int pos;
    int len;
    int dof;
};

/**
 * Creates a new WU_MVar (Moving Variance) indicator with the specified
 * window size and degree of freedom.
 */
WU_MVar wu_mvar_new(int window_size, int dof);

/**
 * The WU_MStDev (Moving Standard Deviation) is an indicator that calculates
 * the standard deviation of the last N values, where N is the window size.
 * It is used to measure the volatility of a time series. `dof` stands for
 * "degree of freedom" and is used to adjust the standard deviation
 * calculation. If `dof` is 0, the standard deviation is calculated using
 * the population standard deviation formula. If `dof` is 1, the standard
 * deviation is calculated using the sample standard deviation formula.
 */
typedef struct WU_MStDev_ {
    double (*update)(struct WU_MStDev_ *self, double value);
    void (*delete)(struct WU_MStDev_ *self);
    double value;
    WU_MVar mvar;
}* WU_MStDev;

/**
 * Creates a new WU_MStDev (Moving Standard Deviation) indicator with the
 * specified window size and degree of freedom.
 */
WU_MStDev wu_mstdev_new(int window_size, int dof);

/**
 * The WU_RSI (Relative Strength Index) is a momentum oscillator that measures
 * the speed and change of price movements. It is calculated using the
 * average gain and average loss over a specified period. The RSI ranges
 * from 0 to 100, with values above 70 typically indicating overbought
 * conditions and values below 30 indicating oversold conditions.
 */
typedef struct WU_RSI_ {
    double (*update)(struct WU_RSI_ *self, const WU_Candle* candle);
    void (*delete)(struct WU_RSI_ *self);
    double value;
    WU_EMA gain;
    WU_EMA loss;
}* WU_RSI;

/**
 * Creates a new WU_RSI (Relative Strength Index) indicator with the
 * specified window size.
 */
WU_RSI wu_rsi_new(int window_size);

/**
 * The WU_MACDResult structure holds the current values of the MACD line,
 * signal line, and histogram. The MACD line is the difference between the
 * short-term EMA and long-term EMA. The signal line is an EMA of the MACD
 * line, and the histogram is the difference between the MACD line and the
 * signal line.
 */
typedef struct MACDResult_ {
    double macd;
    double signal;
    double histogram;
} WU_MACDResult;

/**
 * The WU_MACD (Moving Average Convergence Divergence) is a
 * trend-following momentum indicator that shows the relationship
 * between two moving averages of a security's price. The MACD is
 * calculated by subtracting the long-term EMA from the short-term EMA.
 * A signal line, which is an EMA of the MACD, is then plotted on top of
 * the MACD to identify buy and sell signals. The histogram represents
 * the difference between the MACD and the signal line, providing a
 * visual representation of the momentum of the price movement.
 */
typedef struct WU_MACD_ {
    WU_MACDResult (*update)(struct WU_MACD_ *self, double value);
    void (*delete)(struct WU_MACD_ *self);
    WU_MACDResult value;
    WU_EMA ema_short;
    WU_EMA ema_long;
    WU_EMA signal_line;
    int len;
    int start;
}* WU_MACD;

/**
 * Creates a new WU_MACD (Moving Average Convergence Divergence) indicator
 * with the specified short and long window sizes, and signal line window
 * size.
 */
WU_MACD wu_macd_new(int short_window, int long_window, int signal_window,
        double smoothing);

/**
 * Maximum Drawdown - tracks the largest peak-to-trough decline.
 * Updates with portfolio values and returns the drawdown as a negative
 * percentage.  Calculated as (current_value - peak) / peak when current
 * is below peak.
 */
typedef struct WU_MaxDrawdown_ {
    double (*update)(struct WU_MaxDrawdown_* self, double portfolio_value);
    void (*delete)(struct WU_MaxDrawdown_* self);
    double value;
    double peak;
}* WU_MaxDrawdown;

/**
 * Creates a new WU_MaxDrawdown indicator. The initial value is set to 0
 * and the initial peak is set to the first portfolio value observed.
 */
WU_MaxDrawdown wu_max_drawdown_new(void);

/**
 * Performance Update - value and timestamp for performance metric calculations.
 */
typedef struct {
    double portfolio_value;  // Current portfolio value
    WU_TimeStamp timestamp;  // Timestamp of this value
} WU_PerformanceUpdate;


/**
 * Tracks the average downside, calculate as the square root of the
 * average of the squared negative values. It is used with returns to
 * measure the volatility of negative returns.
 */
typedef struct WU_Downside_ {
    double (*update)(struct WU_Downside_ *self, double value);
    void (*delete)(struct WU_Downside_ *self);
    double value;
    double downside_m2;    // Sum of squared negative returns
    int64_t count;         // Number of returns observed (for denominator)
}* WU_Downside;

/**
 * Creates a new WU_Downside indicator.
 */
WU_Downside wu_downside_new(void);

/**
 * A global mean calculator that updates with new values and maintains
 * the current mean.
 */
typedef struct WU_Mean_ {
    double (*update)(struct WU_Mean_ *self, double value);
    void (*delete)(struct WU_Mean_ *self);
    double value;
    int count;
    double accum;
}* WU_Mean;

/**
 * Creates a new WU_Mean indicator.
 */
WU_Mean wu_mean_new(void);

/** 
 * A glabal variance calculator. It reports the variance for all passed
 * values
 */
typedef struct WU_Var_ {
    double (*update)(struct WU_Var_ *self, double value);
    void (*delete)(struct WU_Var_ *self);
    double value;
    WU_Mean mean;
    double sum2;
    int dof;
    int count;
}* WU_Var;

/**
 * Createas a new variance indicator
 */
WU_Var wu_var_new(int dof);

/** A global standard deviation calcular. It reports the standard
 * deviation for all passed values.
 */
typedef struct WU_StDev_ {
    double (*update)(struct WU_StDev_ *self, double value);
    void (*delete)(struct WU_StDev_ *self);
    double value;
    WU_Var var;
}* WU_StDev;

/**
 * Creates a new standard deviation indicator
 */
WU_StDev wu_stdev_new(int dof);

#endif // WU_INDICATOR_H
