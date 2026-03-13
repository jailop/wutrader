# Wu - Low Level Backtesting Library for Trading

This is a personal project – a backtesting library for trading strategies written in C. I use it for learning, experimenting with different designs, and exploring ideas in algorithmic trading.

Check the [tutorial](./tutorial.md) for a step-by-step introduction to
using the library.

## Core Components

- **Portfolios**: Multi-asset portfolio management with shared cash pool
- **Strategies**: Pluggable strategy interface for signal generation
- **Indicators**: Technical analysis building blocks
- **Data Readers**: Flexible data input abstraction
- **Runner**: Backtest execution engine

## Requirements

- C11 compatible compiler
- Make
- Doxygen (optional, for API documentation)

For Python bindings:
- Python 3.7+
- SWIG (only for development, not for pip install)

## Installation

Build the library:

```bash
make
```

Run the tests:

```bash
make run_tests
```

Install system-wide:

```bash
sudo make install
sudo ldconfig
```

Generate API documentation:

```bash
make docs
```

This generates HTML documentation in `docs/html/index.html`.

Install the library (requires sudo):

```bash
sudo make install
```

This installs to `/usr/local` by default. To install to a different location:

```bash
make install PREFIX=/custom/path
```

Uninstall:

```bash
sudo make uninstall
```

## Examples

### Single Asset Strategy

Run a moving average crossover strategy on Bitcoin:

```bash
./examples/backtest/example01 ./tests/data/btcusd.csv -v
```

This reads OHLCV data, calculates short and long moving averages, generates buy/sell signals when they cross, and tracks portfolio performance including transaction costs and slippage.

### Pairs Trading Strategy

Run a pairs trading strategy on SPY and QQQ:

```bash
./examples/backtest/pairs_trading ./tests/data/spy.csv ./tests/data/qqq.csv -v
```

This monitors the spread between two correlated assets, enters positions when the spread deviates from its mean, and exits when it reverts. The multi-asset portfolio manages both positions with a shared cash pool.

## Design

The library uses C's struct-and-function-pointer pattern for polymorphism. Each abstraction – Portfolio, Strategy, Reader, and Indicator – is a struct with function pointers, allowing different implementations with a consistent interface.

Wu is a toolkit, not a framework. The core abstractions compose freely. You can swap implementations, combine strategies, or bypass components without constraints.

The Runner shows this composability. It wires together a portfolio, strategy, and one or more assets. You can also write custom runners with different logic or bypass the runner entirely.

Wu supports three data types:

- **Candles** (OHLCV): Standard bar data
- **Trades**: Tick-level data with price, volume, and side
- **Single Values**: Generic time-series data

The Reader abstraction accepts any data source—CSV files, databases, real-time feeds, or synthetic generators. It just needs a next() method.

Every component maintains its state explicitly in its struct. This makes debugging straightforward and allows state snapshots at any point.

I use C because I enjoy it. I like having control over the details while building high-level abstractions. That said, I'm not dogmatic about it. This library includes Python bindings, and I maintain a similar library in C++ ([tzutrader](https://jailop.codeberg.page/tzutrader/docs/)). I move between implementations to explore different ideas.

## Contributing

This is a personal project, so I'm not hopeful for contributions. However, if you have questions, suggestions, or want to share ideas, feel free to reach out to me. If you feel interested in learning more about this project, I'll be happy to share more details about the design and implementation. Furthermore, if you want to contribute – by reviewing the code, opening issues, or even submitting pull requests – I'll be very grateful.

My email is: >> jailop \AT/ protonmail \DOT/ com <<
