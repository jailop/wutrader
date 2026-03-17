# Features

Wu is a low-level backtesting library designed for quantitative research and algorithmic trading strategy development. Here's an inventory of its core capabilities.

## Core Backtesting

- **Multi-asset portfolio simulation** - Trade multiple assets simultaneously with coordinated position management
- **Realistic execution models** - Market orders (immediate execution) and next-close orders (execution at the following bar's close)
- **Position sizing strategies** - Notional and percentage-based position sizing with customizable scaling
- **Leverage and margin** - Support for short positions and borrowing with configurable borrow rates and limits
- **Transaction costs** - Model slippage and transaction fees with flexible cost structures (fixed, percentage, or value-based)

## Risk Management

- **Stop-loss and take-profit** - Automatic position exit triggers based on percentage levels
- **Portfolio risk tracking** - Monitor portfolio-level risk exposure and constraints
- **Drawdown analysis** - Maximum drawdown calculation for strategy evaluation

## Technical Analysis

- **Standard indicators** - SMA, EMA, RSI, MACD, Bollinger Bands, Stochastic, ATR, ADX, and more
- **Sequential indicator updates** - All indicators update efficiently as new data arrives
- **Custom indicator support** - Easy extension for strategy-specific calculations

## Performance Analytics

- **Trade statistics** - Win rate, maximum win/loss, trade count and distribution
- **Return metrics** - Total and percentage returns, profit/loss tracking
- **Performance indicators** - Sharpe ratio, Sortino ratio, maximum drawdown, with annualization support
- **Time-aware metrics** - All performance calculations account for the data's time units
- **PnL analysis** - Average PnL, standard deviation of PnL per trade

## Data Input

- **CSV reader** - Load OHLCV data from standard CSV files
- **JSON Lines reader** - Parse newline-delimited JSON data efficiently
- **Flexible data formats** - Support for various timestamp units and data structures

## Output and Reporting

- **Comprehensive backtest results** - Summary statistics including positions, trades, and costs
- **JSON export** - Structured output for integration with other tools
- **Trade-by-trade details** - Access to individual trade history for analysis

## Architecture

- **Low-level C library** - Performance-oriented implementation suitable for research and production
- **Python bindings** - Pythonic interface for strategy development and experimentation
- **Indicator pattern** - Consistent interface for all technical indicators and performance metrics
- **Sequential processing** - Data processed one bar at a time for realistic simulation

## Experimental Status

Wu is an ongoing experimentation project. Features are continuously refined based on research requirements and practical use cases. The library prioritizes accuracy and flexibility over feature completeness, allowing researchers to build exactly what they need.
