# Wu - Low Level Backtesting Library

This is a personal project—a backtesting library for trading strategies written in C. I use it for learning, experimenting with different designs, and exploring ideas in algorithmic trading. It provides core abstractions: indicators, strategies, data readers, a portfolio, and a runner, that compose together.

It is not a production-ready product. It lacks sophisticated risk metrics, realistic execution modeling, market microstructure simulation, and a proper out-of-sample validation framework. Use it for learning and experimentation; do not use it for live trading.

## Core Components

The library provides an architecture for implementing components that can be combined into a backtesting system:

**Indicators:** These serve as building blocks for technical analysis and performance metrics. They maintain internal state and update incrementally as new data arrives.

**Strategies:** These generate trading signals by consuming market data, updating internal indicators, and producing buy/sell/hold decisions.

**Data Readers:** Generic data feeders for CSV and JSON inputs that can be used to ingest data from files, databases, or network streams.

**Portfolios:** These enable simulation to manage cash, execute trades, and track positions across multiple assets. The basic portfolio implementation supports long and short positions, transaction costs, borrowing costs, and basic risk management and execution policies.

**Runner:** This orchestrates the backtest loop, coordinating readers, strategies, and portfolios. It handles data synchronization and component lifecycle.

The implemented components serve as examples that can be extended or used as a reference for implementing custom modules.

## Design Principles

Components carry state explicitly in structs. No hidden globals. When
debugging, you can inspect memory directly. Struct definitions show all
parameters and internal state.

The components compose freely. Swap a CSV file reader for a database reader.
Replace a strategy implementation. Use portfolio code without the runner.
Nothing enforces rigid patterns.

Every component uses function pointers for generic behavior and polymorphism without overhead. This lets you implement custom components with different execution logic or risk models, and plug them into an automated runner.

## Contributing

If you have any questions or suggestions, I’m more than happy to hear them. If you’d like to review the code or propose changes, I’d really appreciate that. If you find the project interesting and want to brainstorm design ideas or explore concepts together, please don’t hesitate to reach out.

Email: jailop AT protonmail DOT com
