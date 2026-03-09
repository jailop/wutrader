# WU Python Bindings

Python bindings for the WU backtesting library using SWIG.

## Building

```bash
make          # Build the Python module
make test     # Test the bindings
make docs     # Generate API documentation
make clean    # Clean build artifacts
```

## Quick Start

```python
import wu

# Create a moving average indicator
ma = wu.moving_average_new(20)
ma.update(100.0)
value = ma.value()

# Run a backtest
portfolio = wu.create_single_asset_portfolio(initial_cash=10000.0)
strategy = wu.cross_over_strat_new(10, 30, 0.0)
reader = wu.csv_reader_open("data.csv", wu.DATA_TYPE_CANDLE, True)
runner = wu.basic_runner_new(portfolio, strategy, reader)
runner.execute(verbose=False)
```

## Requirements

- Python 3.7+
- SWIG 4.0+
- C library built with `make` in the parent directory

For documentation generation:

- pdoc (automatically installed in venv by `make docs`)
