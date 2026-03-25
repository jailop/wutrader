## Indicators

Quick reference for available indicator types and constructors (source: include/wu/indicators.h).

Common macros

- wu_indicator_update(indicator, value) -> call indicator->update(indicator, value)
- wu_indicator_get(indicator) -> read indicator->value (may be NAN during warmup)
- wu_indicator_delete(indicator) -> call indicator->delete(indicator)

Available indicators

- Simple Moving Average (SMA)
  - Type: WU_SMA
  - Constructor: WU_SMA wu_sma_new(int window_size)
  - Notes: returns NAN until warmup (window_size samples)

- Exponential Moving Average (EMA)
  - Type: WU_EMA
  - Constructor: WU_EMA wu_ema_new(int window_size, double smoothing)
  - Notes: smoothing controls alpha; faster responsiveness for smaller smoothing

- Moving Variance (MVar)
  - Type: WU_MVar
  - Constructor: WU_MVar wu_mvar_new(int window_size, int dof)
  - dof: 0 => population variance (divide by N), 1 => sample variance (divide by N-1)

- Moving Standard Deviation (MStDev)
  - Type: WU_MStDev
  - Constructor: WU_MStDev wu_mstdev_new(int window_size, int dof)
  - Notes: wraps WU_MVar and returns sqrt(variance)

- Relative Strength Index (RSI)
  - Type: WU_RSI
  - Constructor: WU_RSI wu_rsi_new(int window_size)
  - Update: expects a const WU_Candle*; use the close price for calculations

- MACD
  - Type: WU_MACD, result WU_MACDResult { macd, signal, histogram }
  - Constructor: WU_MACD wu_macd_new(int short_window, int long_window, int signal_window, double smoothing)

- Maximum Drawdown
  - Type: WU_MaxDrawdown
  - Constructor: WU_MaxDrawdown wu_max_drawdown_new(void)
  - Notes: update with portfolio value; value is drawdown (negative when below peak)

- Downside Deviation
  - Type: WU_Downside
  - Constructor: WU_Downside wu_downside_new(void)
  - Notes: tracks negative returns for Sortino metrics

- Global Mean
  - Type: WU_Mean
  - Constructor: WU_Mean wu_mean_new(void)

- Global Variance (WU_Var) and Standard Deviation (WU_StDev)
  - Types: WU_Var, WU_StDev
  - Constructors: WU_Var wu_var_new(int dof), WU_StDev wu_stdev_new(int dof)
  - Notes: WU_Var maintains an internal mean and sum2; choose dof appropriately

Usage notes

- Check isnan(wu_indicator_get(...)) during warmup before using values.
- Use wu_indicator_delete to free resources.
- For variance/stddev, prefer dof=1 for sample estimates when computing sample statistics.

Example

- WU_SMA s = wu_sma_new(5);
- double v = wu_indicator_update(s, 10.0);
- if (!isnan(wu_indicator_get(s))) printf("SMA=%.3f\n", wu_indicator_get(s));
- wu_indicator_delete(s);
