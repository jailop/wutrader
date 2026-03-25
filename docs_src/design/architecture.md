# Architecture Overview

Wu uses a modular design where components communicate through defined
interfaces. This document explains the structural decisions and their
rationale.

## Component Model

The library organizes around five core abstractions:

```
┌─────────────┐
│   Runner    │  ← Orchestrates execution
└──────┬──────┘
       │
   ┌───┴───────────────────┐
   ▼                       ▼
┌─────────┐          ┌──────────┐
│ Reader  │──────────│ Strategy │  ← Generates signals
└─────────┘          └────┬─────┘
   │                      │
   │                      ▼
   │                 ┌──────────┐
   │                 │Portfolio │  ← Executes trades
   └─────────────────┤          │
                     └──────────┘
```

**Data flow** moves in one direction. Readers fetch data from sources
(CSV files, databases, APIs). Strategies consume that data, update
indicators, and produce signals. Portfolios execute signals as trades,
managing cash and positions. The runner coordinates these operations in
sequence.

**Polymorphism** uses C's struct-and-function-pointer pattern. A base
struct defines the interface (function pointers). Concrete
implementations embed the base struct as their first member, then add
specific state. This gives inheritance-like behavior without vtables or
dynamic dispatch overhead.

**State management** is explicit. Every component carries its state in
its struct. No hidden globals, no singletons. You can serialize a
portfolio's state by copying its struct. You can inspect indicator
internal buffers directly in a debugger.

## Interface Contracts

Each abstraction defines a minimal contract:

**WU_Reader** provides one method: `next()`. Call it repeatedly to fetch
data points. It returns NULL when exhausted. The reader owns the returned
data and may reuse buffers between calls.

**WU_Strategy** has one method: `update()`. Pass it an array of data
points (one per asset). It returns an array of signals (also one per
asset). The strategy owns the signal buffer.

**WU_Portfolio** has three methods: `update()` to process signals,
`value()` to get current portfolio value, and `pnl()` to calculate
profit/loss. The portfolio maintains positions and cash internally.

**WU_Indicator** provides an `update()` method that accepts a value and
returns the computed indicator value. Indicators maintain their own
internal buffers and state.

These minimal contracts keep implementations simple. A CSV reader doesn't
need to know about strategies. A strategy doesn't care where data comes
from. A portfolio doesn't care how signals were generated.

## Composition Over Configuration

The library provides building blocks rather than a framework. You wire
components together explicitly:

```c
WU_Runner runner = wu_runner_new(
    portfolio,
    strategy,
    wu_reader_list(reader1, reader2)
);
```

This explicit wiring makes data flow visible. You can see what connects
to what. No dependency injection, no service locators, no magic
auto-wiring.

If the runner's behavior doesn't fit your needs, write your own loop.
The components work independently. The runner is convenience, not
requirement.

## Type System

Wu defines three data types: Candles (OHLCV bars), Trades (tick data),
and Singles (scalar values with timestamps). Strategies declare what
types they accept. Readers declare what types they produce. The runner
validates compatibility at construction.

This type system is intentionally minimal. It covers common use cases
without trying to model every possible market data structure. If you need
something different, wrap it in one of these three types or extend the
type enum.

## Extension Points

**Custom Portfolios**: Implement the `WU_Portfolio` interface to create
portfolios with different execution logic, risk models, or position
management approaches.

**Custom Strategies**: Implement the `WU_Strategy` interface. Your
strategy can use any indicator, maintain any state, and generate signals
based on any logic.

**Custom Readers**: Implement the `WU_Reader` interface to pull data from
any source. PostgreSQL, REST APIs, ZeroMQ feeds, synthetic generators—if
you can read it, you can backtest against it.

**Custom Indicators**: Indicators are just structs with an update
function. Implement new indicators following the existing patterns.

## Limitations by Design

**No order book**: Wu executes trades immediately at given prices. There's
no order matching, no queue, no price-time priority. This simplification
works for backtesting but doesn't model market microstructure.

**No event system**: Components communicate through synchronous function
calls. No event bus, no message queue, no async callbacks. This keeps
reasoning about execution order straightforward.

**No persistence layer**: Wu doesn't save or load state. Backtests run
in memory from start to finish. If you need persistence, implement it
around the library.

## Why C?

C provides control over memory layout and allocation patterns. When
performance matters, you can see exactly what the processor executes. No
garbage collection pauses. No hidden allocations.

C also forces explicit design decisions. Without automatic memory
management or inheritance hierarchies, you think carefully about
ownership, lifetimes, and interfaces.

The downside is verbosity and manual memory management. The Python
bindings provide a higher-level interface when C's explicitness isn't
needed.
