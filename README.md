# Wu - Low Level Backtesting Library for Trading

This is a personal project—a backtesting library for trading strategies
written in C. I use it for learning, experimenting with different
designs, and exploring ideas in algorithmic trading.

Read the [onlinen  documentation](https://jailop.codeberg.page/wutrader/docs) 
for tutorials, design notes, and examples.

## Status

This is an ongoing experiment. The design evolves as I explore different
approaches to modeling trading systems. Code may change significantly as
I refine ideas or discover better patterns.

Expect breaking changes. This is a learning project, not stable
infrastructure.

## Quick Start

Build the library:

```bash
make
```

Run the examples:

```bash
# Single asset crossover strategy
./examples/backtest/example01 ./tests/data/spy.csv

# Pairs trading strategy
./examples/backtest/pairs_trading ./tests/data/spy.csv ./tests/data/qqq.csv
```

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
- MkDocs (optional, for user documentation)

For Python bindings:
- Python 3.7+
- SWIG (only for development)

## Documentation

- [Online docs](https://jailop.codeberg.page/wutrader/) - Tutorial, design notes, examples
- [Tutorial](./tutorial.md) - Step-by-step introduction
- API reference - Run `make docs` to generate Doxygen documentation

## Design

Wu uses C's struct-and-function-pointer pattern for polymorphism.
Components compose freely—swap readers, strategies, or portfolios without
constraints.

The library provides tools, not a framework. Use what you need. Write
your own runner if the default doesn't fit. Implement custom portfolios
with different execution models.

I use C because I enjoy working at this level. The library also includes
Python bindings for prototyping. I maintain a similar C++ implementation
([tzutrader](https://jailop.codeberg.page/tzutrader/docs/)) to explore
different design patterns.

## Contributing

This is a personal project. I'm not actively seeking contributions, but
questions and suggestions are welcome. If you find the project
interesting and want to discuss design ideas, reach out.

Email: jailop AT protonmail DOT com

## License

See LICENSE file for details.
