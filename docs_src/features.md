# Features

## Core Backtesting

- Multi-asset portfolio simulation - Trade multiple assets simultaneously with coordinated position management
- Execution model - Market orders (immediate execution) and next-close orders (execution at the following bar's close)
- Position sizing strategies - Notional and percentage-based position sizing with customizable scaling
- Leverage and margin - Support for short positions and borrowing with configurable borrow rates and limits
- Transaction costs - Model slippage and transaction fees with flexible cost structures (fixed, percentage, or value-based)

## Risk Management

- Stop-loss and take-profit - Automatic position exit triggers based on percentage levels
- Portfolio risk tracking - Monitor portfolio-level risk exposure and constraints
- Drawdown analysis - Maximum drawdown calculation for strategy evaluation

## Technical Analysis

- Standard indicators - SMA, EMA, RSI, MACD, Bollinger Bands, Stochastic, ATR, ADX, and more
- Sequential indicator updates - All indicators update efficiently as new data arrives
- Custom indicator support - Easy extension for strategy-specific calculations

## Performance Analytics

- Trade statistics - Win rate, maximum win/loss, trade count and distribution
- Return metrics - Total and percentage returns, profit/loss tracking
- Performance indicators - Sharpe ratio, Sortino ratio, maximum drawdown, with annualization support
- Time-aware metrics - All performance calculations account for the data's time units
- PnL analysis - Average PnL, standard deviation of PnL per trade

## Data Input

- CSV reader - Load OHLCV data from standard CSV files
- JSON Lines reader - Parse newline-delimited JSON data efficiently
