# Indicators

Quick reference table for indicators (from include/wu/indicators.h).

Common macros
- wu_indicator_update(ind, value)
- wu_indicator_get(ind)
- wu_indicator_delete(ind)

| Indicator | Type | Constructor | Notes |
|---|---|---|---|
| Simple Moving Average | WU_SMA | WU_SMA wu_sma_new(int window_size) | Returns NAN until warmup (window_size samples) |
| Exponential Moving Average | WU_EMA | WU_EMA wu_ema_new(int window_size, double smoothing) | smoothing controls alpha; smaller -> more responsive |
| Moving Variance | WU_MVar | WU_MVar wu_mvar_new(int window_size, int dof) | dof=0 population (divide by N); dof=1 sample (divide by N-1) |
| Moving Standard Deviation | WU_MStDev | WU_MStDev wu_mstdev_new(int window_size, int dof) | Wraps WU_MVar; value = sqrt(variance) |
| Relative Strength Index (RSI) | WU_RSI | WU_RSI wu_rsi_new(int window_size) | Update expects const WU_Candle* (use close price) |
| MACD | WU_MACD (WU_MACDResult) | WU_MACD wu_macd_new(int short_window, int long_window, int signal_window, double smoothing) | Returns WU_MACDResult { macd, signal, histogram } |
| Maximum Drawdown | WU_MaxDrawdown | WU_MaxDrawdown wu_max_drawdown_new(void) | Update with portfolio value; value is negative drawdown when below peak |
| Downside Deviation | WU_Downside | WU_Downside wu_downside_new(void) | Tracks squared negative returns for Sortino metrics |
| Global Mean | WU_Mean | WU_Mean wu_mean_new(void) | Online mean calculator |
| Global Variance | WU_Var | WU_Var wu_var_new(int dof) | Global variance over all observed values; maintains mean & sum2 |
| Global StDev | WU_StDev | WU_StDev wu_stdev_new(int dof) | Wrapper around WU_Var; returns sqrt(variance) |

Usage notes
- Many indicators return NAN while warming up; guard callers with isnan(wu_indicator_get(...)).
- Free indicators with wu_indicator_delete() to avoid leaks.
- For sample statistics use dof=1; for population use dof=0.

Example (C)

```c
WU_SMA *s = wu_sma_new(5);
double v = wu_indicator_update(s, 10.0);
if (!isnan(wu_indicator_get(s)))
    printf("SMA=%.3f\n", wu_indicator_get(s));
wu_indicator_delete(s);
```

This cheatsheet mirrors include/wu/indicators.h; expand into per-indicator pages if desired.
