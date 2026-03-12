#ifndef WU_INDICATOR_H
#define WU_INDICATOR_H

#include "types.h"
#include "data.h"
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
    double (*get)(const struct WU_EMA_ *self);
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
    double (*get)(const struct WU_MVar_ *self);
    void (*delete)(struct WU_MVar_ *self);
    double value;
    WU_SMA sma;
    double* prev_values;
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
 * The WU_StDev (Moving Standard Deviation) is an indicator that calculates
 * the standard deviation of the last N values, where N is the window size.
 * It is used to measure the volatility of a time series. `dof` stands for
 * "degree of freedom" and is used to adjust the standard deviation
 * calculation. If `dof` is 0, the standard deviation is calculated using
 * the population standard deviation formula. If `dof` is 1, the standard
 * deviation is calculated using the sample standard deviation formula.
 */
typedef struct WU_StDev_ {
    double (*update)(struct WU_StDev_ *self, double value);
    double (*get)(const struct WU_StDev_ *self);
    void (*delete)(struct WU_StDev_ *self);
    WU_MVar mvar;
}* WU_StDev;

/**
 * Creates a new WU_StDev (Moving Standard Deviation) indicator with the
 * specified window size and degree of freedom.
 */
WU_StDev wu_stdev_new(int window_size, int dof);

/**
 * The WU_RSI (Relative Strength Index) is a momentum oscillator that measures
 * the speed and change of price movements. It is calculated using the
 * average gain and average loss over a specified period. The RSI ranges
 * from 0 to 100, with values above 70 typically indicating overbought
 * conditions and values below 30 indicating oversold conditions.
 */
typedef struct WU_RSI_ {
    double (*update)(struct WU_RSI_ *self, const WU_Candle* candle);
    double (*get)(const struct WU_RSI_ *self);
    void (*delete)(struct WU_RSI_ *self);
    WU_EMA gain;
    WU_EMA loss;
    double data;
}* WU_RSI;

/**
 * Creates a new WU_RSI (Relative Strength Index) indicator with the
 * specified window size.
 */
WU_RSI wu_rsi_new(int window_size);

typedef struct MACDResult_ {
    double macd;
    double signal;
    double histogram;
} WU_MACDResult;

typedef struct WU_MACD_ {
    WU_MACDResult (*update)(struct WU_MACD_ *self, double value);
    WU_MACDResult (*get)(const struct WU_MACD_ *self);
    void (*delete)(struct WU_MACD_ *self);
    WU_EMA ema_short;
    WU_EMA ema_long;
    WU_EMA signal_line;
    WU_MACDResult data;
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

#endif // WU_INDICATOR_H
