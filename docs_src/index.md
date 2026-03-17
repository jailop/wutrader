# Wu - Low Level Backtesting Library

This is a personal project—a backtesting library for trading strategies
written in C. I use it for learning, experimenting with different
designs, and exploring ideas in algorithmic trading.

## What This Is

Wu is a toolkit for building backtesting systems. It provides core
abstractions—portfolios, strategies, indicators, data readers, and a
runner—that compose together. The implementation uses C's
struct-and-function-pointer pattern for polymorphism without the
overhead of inheritance.

This is an ongoing experiment. The design evolves as I explore different
approaches to modeling trading systems. Code may change significantly as
I refine ideas or discover better patterns.

## What This Is Not

Wu is not production-ready trading infrastructure. It lacks many features
professional systems require: sophisticated risk metrics, realistic
execution modeling, market microstructure simulation, and proper
out-of-sample validation frameworks. Use it for learning and
experimentation, not live trading.

## Core Components

**Portfolios** manage cash, execute trades, and track positions across
multiple assets. The `WU_BasicPortfolio` implementation supports long and
short positions, transaction costs, borrowing costs, and basic risk
management.

**Strategies** generate trading signals. They consume market data, update
internal indicators, and produce buy/sell/hold decisions. The library
includes moving average crossover and pairs trading implementations.

**Indicators** provide technical analysis building blocks: moving
averages, standard deviation, RSI, MACD. They maintain state internally
and update incrementally as new data arrives.

**Data Readers** abstract data sources. The CSV reader handles file
input, but the interface supports any source—databases, APIs, or
synthetic generators.

**Runner** orchestrates the backtest loop, coordinating readers,
strategies, and portfolios. It handles data synchronization and component
lifecycle.

## Design Principles

Components carry state explicitly in structs. No hidden globals. When
debugging, you can inspect memory directly. Struct definitions show all
parameters and internal state.

The components compose freely. Swap a CSV reader for a database reader.
Replace a strategy implementation. Use portfolio code without the runner.
Nothing enforces rigid patterns.

Every component interface uses function pointers for polymorphism. The
runner works with `WU_Portfolio`, not `WU_BasicPortfolio`. This lets you
implement custom portfolios with different execution logic or risk models.

## Status and Direction

This project is under active development. I'm currently exploring:

- More realistic execution modeling
- Position-level risk tracking
- Event-driven architecture patterns
- Better separation between backtesting and live trading concerns

Expect breaking changes as the design evolves. This is a learning
project, not stable infrastructure.

## Getting Started

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

Read the [tutorial](tutorial.md) for a step-by-step introduction.

For detailed API documentation, see the [Doxygen API Reference](https://jailop.codeberg.page/wutrader/docs/html/).

See the [Guides](portfolio-configuration.md) for detailed explanations of portfolio configuration and timestamp handling.

Repository: [https://codeberg.org/jailop/wu](https://codeberg.org/jailop/wu)

## Contributing

This is a personal project. I'm not actively seeking contributions, but
questions and suggestions are welcome. If you find the project
interesting and want to discuss design ideas or explore concepts
together, reach out.

Email: jailop AT protonmail DOT com

## License

This project is open source. Check the repository for license details.
