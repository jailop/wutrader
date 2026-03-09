# WU Python Bindings

Python bindings for the WU backtesting library using SWIG.

## Building

```bash
make          # Build the Python module
make test     # Test the bindings
make docs     # Generate API documentation
make clean    # Clean build artifacts
make version  # Show version
```

## Installation

### From Codeberg with pip

```bash
# Install C library first
git clone https://codeberg.org/jailop/wu.git
cd wu
make && sudo make install && sudo ldconfig

# Install Python bindings
pip install git+https://codeberg.org/jailop/wu.git#subdirectory=python
```

See [INSTALL.md](INSTALL.md) for detailed installation instructions.

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
- C compiler (gcc or clang)
- Make
- **SWIG 4.0+ (only for development)** - Pre-generated wrapper files are included

For documentation generation:

- pdoc (automatically installed in venv by `make docs`)
