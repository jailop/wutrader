# Frequently Asked Questions

## General Questions

**What is WU (WuTrader)?**

WU is a C library for backtesting trading strategies. It provides core components for portfolio management, signal generation, and performance analysis without imposing a specific trading framework or strategy implementation.

**What platforms does WU support?**

WU is written in C and compiles on any platform with a C compiler. The library has been tested on Linux systems. Python bindings are available for those preferring to work in Python.

**Is WU suitable for production trading?**

WU is designed for backtesting and strategy research. While the core logic is sound, using any backtesting library for live trading requires careful consideration of slippage, liquidity, and other real-world factors not fully captured in historical simulation.

## Data and Input

**What data formats does WU accept?**

WU supports CSV files and JSON Lines format (one valid JSON object per line). The exact field mappings depend on your data source and can be configured via the reader interface.

**How should timestamps be formatted?**

Timestamps are stored as numeric values in specified units (seconds, milliseconds, minutes, hours, days). The library handles different time units consistently across all calculations. See the [Timestamps and Data](guides/timestamps.md) guide for details.

**Can I use minute-level data? Daily data? Tick data?**

Yes. WU works with any uniform time interval. Just ensure your timestamps are consistent and specify the correct time unit when creating indicators or calculating metrics that depend on time (like Sharpe ratio annualization).

## Portfolio and Positions

**Does WU support multiple assets?**

Yes. You can trade multiple assets simultaneously. Each position is tracked separately with its own quantity, entry price, and unrealized P&L.

**Can I short stocks?**

Yes. Short positions are represented as negative quantities. Borrowing costs are tracked separately through the borrow interest mechanism.

**What order types are supported?**

Currently only market orders are supported. The library does not implement an order book, which would be required for limit orders, stop orders, or other conditional order types. All orders execute immediately at the current price.

**How is slippage handled?**

Slippage is modeled through the execution policy. You can specify the execution price as a fixed amount away from the signal price, or as a random offset based on volatility (using standard deviations). This allows you to approximate realistic trading costs beyond just transaction fees. See [Portfolio Configuration](portfolio-configuration.md) for execution policy details.

## Performance Analysis

**What performance metrics does WU calculate?**

WU calculates several key metrics:

- **Return metrics**: Total P&L, percentage return, average P&L per trade
- **Risk metrics**: Maximum Drawdown, return standard deviation
- **Risk-adjusted returns**: Sharpe ratio and Sortino ratio (both annualized)
- **Trade statistics**: Win rate, maximum win/loss, number of winning/losing trades
- **Cost tracking**: Transaction fees and borrowing interest

See the [Portfolio Configuration](portfolio-configuration.md) guide for details on how to configure metrics tracking.

**How is Sharpe ratio calculated?**

Sharpe ratio = (portfolio return - risk-free rate) / return standard deviation, annualized. The risk-free rate must be provided in the portfolio parameters. The annualization factor depends on your data's time unit (e.g., 252 for daily data, 252 * 6.5 for hourly data during trading hours).

**What is the difference between Sharpe and Sortino ratio?**

Both measure risk-adjusted returns. Sharpe uses total return volatility, while Sortino uses only downside volatility (negative returns). Sortino penalizes beneficial volatility less, making it useful for strategies with asymmetric return distributions.

**Can I track custom metrics?**

The indicator interface lets you implement custom calculations that update sequentially as data arrives. See the [Design Notes](design/architecture.md) section for examples of implementing custom indicators.

## Strategy Development

**How do I implement a trading signal?**

Signals are typically derived from indicators (like moving averages). Your strategy logic compares indicators to generate buy/sell signals. The backtest engine then processes these signals according to the execution policy you specify.

**How do I handle position sizing?**

WU supports several position sizing methods through the portfolio parameters:
- Fixed size (always trade X shares)
- Percentage-based (risk Y% of portfolio)
- Volatility-adjusted (scale size based on recent volatility)

**Can I modify positions mid-trade?**

The current design assumes each signal results in a new position. To implement scaling in/out or partial exits, emit appropriate signals and let the execution policy handle position adjustments.

## Technical Questions

**What is the overhead of backtesting?**

Performance depends on data size and strategy complexity. Most backtests on years of daily data complete in seconds. The library is optimized for memory efficiency and sequential processing.

**Can I run multiple backtests in parallel?**

Yes. The library is stateless—each backtest instance maintains its own portfolio state. You can run independent backtests in separate threads without coordination.

**How do I contribute or report bugs?**

The project is maintained on Codeberg. Bug reports and contributions are welcome. See the [repository](https://codeberg.org/jailop/wutrader) for details.

## Limitations and Design Choices

**Why no limit orders or stop orders?**

Implementing these would require maintaining an order book to track unfilled orders across bars. For backtesting research, market orders are sufficient. Advanced users can simulate limit behavior by implementing custom signal logic.

**Why is the library in C instead of Python?**

C provides performance and portability. The core library is lightweight and can be called from Python, giving you both speed and the convenience of Python for strategy development.

**Is WU opinionated about portfolio construction?**

By design, WU is flexible. It provides building blocks (indicators, position sizing, execution policies) but doesn't mandate a specific strategy pattern. This makes it suitable for diverse research approaches.

**Why sequential updating of indicators?**

Sequential processing (one bar at a time) matches how real trading works and prevents lookahead bias. It also keeps memory usage constant regardless of data size, making the library suitable for long backtests and live applications.
