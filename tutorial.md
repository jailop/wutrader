# Wu Trading Library Tutorial: Building a Pairs Trading Strategy

Jaime Lopez  
Mar. 13, 2026 (updated Mar. 15, 2026)

**Who Is This Tutorial For?**: For developers and quantitative
enthusiasts who want to understand how to build backtesting systems from
scratch. You should be comfortable with C programming and have a basic
understanding of financial markets concepts like trading strategies and
technical indicators. No advanced financial background is required—we'll
explain the trading concepts as we go.

---

We'll build a pairs trading backtester from scratch, working through the
complete `examples/backtest/pairs_trading.c` example. This tutorial
explains how the Wu library is architected and how its pieces fit
together.

Think of this as a guided tour through both the code and the design.
By the end, you'll understand not just how to use the library, but its
structure and rationale.

**Important Note**: Wu is an experimental library designed for learning
and exploring algorithmic trading concepts. It's a tool for
understanding design patterns and architecture in backtesting systems,
not a production-ready trading platform. The examples demonstrate
functionality but come with significant limitations that we'll discuss
throughout this tutorial.

## Table of Contents

1. [Library Architecture Overview](#library-architecture-overview)
2. [Prerequisites and Setup](#prerequisites-and-setup)
3. [Understanding the Data Flow](#understanding-the-data-flow)
4. [Step 1: Setting Up Includes and Basic Structure](#step-1-setting-up-includes-and-basic-structure)
5. [Step 2: Understanding Data Types](#step-2-understanding-data-types)
6. [Step 3: Creating CSV Readers](#step-3-creating-csv-readers)
7. [Step 4: Creating the Strategy](#step-4-creating-the-strategy)
8. [Step 5: Configuring the Portfolio](#step-5-configuring-the-portfolio)
9. [Step 6: Creating the Runner](#step-6-creating-the-runner)
10. [Step 7: Running the Backtest](#step-7-running-the-backtest)
11. [Step 8: Analyzing Results](#step-8-analyzing-results)
12. [Critical Limitations and Warnings](#critical-limitations-and-warnings)
13. [Complete Code](#complete-code)
14. [Building and Running](#building-and-running)
15. [Next Steps](#next-steps)

---

## Library Architecture Overview

Wu is a low-level backtesting library written in C. It uses a modular,
composable design with C's struct-and-function-pointer pattern for
polymorphism. The library provides tools rather than a rigid framework.

### Core Components

The library has five main abstractions:

```
┌─────────────┐
│   Runner    │  ← Orchestrates the backtest execution
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

**Data Types** live in `wu/types.h` and `wu/data.h`. They're the raw
material flowing through the system. A **Candle** carries OHLCV bar data
with its timestamp. A **Trade** represents tick-level activity with
price, volume, and direction. A **Single** is just a timestamped value,
useful for generic time series. And a **Signal** is what strategies
generate: a trading instruction with a side (buy/sell/hold), price, and
quantity.

**Readers** (`wu/readers.h`) abstract away data sources. They have one
job: fetch the next data point. The `WU_CsvReader` handles CSV files,
but you could implement a database reader, an API client, or even a
synthetic data generator. They all share the same interface, so the rest
of the system doesn't care where the data comes from.

**Indicators** (`wu/indicators.h`) are the mathematical building blocks.
Simple Moving Averages, Exponential Moving Averages, Standard
Deviations—these are the tools strategies use to make sense of price
movements. They maintain their own internal state and update
incrementally as new data arrives.

**Strategies** (`wu/strategies.h`) consume market data and spit out
signals. They can work with one asset or many. The crossover strategy
watches two moving averages, while the pairs trading strategy monitors
the spread between two correlated assets. Each strategy declares what
types of inputs it expects and how many outputs it produces.

**Portfolios** (`wu/portfolios.h`) execute the trades and track the
money. They receive signals from strategies, calculate position sizes,
deduct transaction costs and slippage, and maintain positions across one
or more assets. The `WU_BasicPortfolio` is the workhorse implementation
that handles both single and multi-asset scenarios with a shared cash
pool.

**The Runner** (`wu/runners.h`) is the conductor. It wires everything
together, validates that data types match up, synchronizes reads from
multiple sources, and drives the backtest loop forward until the data
runs out.

### Design Philosophy

Every component carries its state explicitly in a struct. No hidden
globals. When debugging, you can inspect memory directly. Struct
definitions show all parameters and internal state.

The components compose freely. Swap out a CSV reader for a database
reader. Replace the crossover strategy with your own custom logic. Use
the portfolio standalone without the runner if you want to drive the
backtest loop yourself. Nothing forces you into a particular pattern.

And there's no framework lock-in. Wu provides building blocks, not rigid
structure. Use just the indicators if needed. Implement custom portfolio
logic if desired. Mix and match components as required.

---

## Prerequisites and Setup

### What You'll Need

You need a C11-compatible compiler like gcc or clang, the Make build
tool, and some historical price data in CSV format. That's it.

### Building the Library

Build Wu and verify everything works:

```bash
# Navigate to the wu directory
cd /path/to/wu

# Build the library
make

# Run the tests to verify everything works
make run_tests

# If you want it system-wide (optional)
sudo make install
sudo ldconfig
```

The build process is straightforward. Wu compiles into a shared library
that you can link against. The tests give you confidence that the
fundamentals are solid before you start writing your own code.

### Understanding the CSV Format

Wu expects CSV files with headers. For candle data, the format looks like this:

```csv
Date,Open,High,Low,Close,Volume
1142865000,130.64,130.90,130.21,130.41,45538500
1142951400,130.37,130.99,129.45,129.59,87102700
...
```

The timestamp in the `Date` column is a Unix epoch value (seconds since
January 1, 1970). This keeps things simple and avoids the complexity of
parsing date strings. You can convert dates to Unix timestamps using
tools like `date -d "2024-01-15" +%s` on Unix systems or Python's
`datetime` module.

### Getting Historical Data with yfnim

Need historical price data for your backtests? The
[yfnim](https://jailop.codeberg.page/yfnim/docs/) tool makes it trivial
to pull data from Yahoo Finance in exactly the format Wu expects. It's a
command-line utility that outputs clean CSV with Unix timestamps—no
parsing gymnastics required.

`yfnim` is written in Nim. To install it, you can use Nim's package
manager, Nimble:

```bash
nimble install yfnim
```

Here's how to fetch 20 years of SPY data:

```bash
yf history -s:spy --lookback:20y --format:csv --date_format:unix > spy.csv
```

For our pairs trading example, you need both SPY and QQQ:

```bash
# Pull SPY data
yf history -s:spy --lookback:20y --format:csv --date_format:unix > spy.csv

# Pull QQQ data
yf history -s:qqq --lookback:20y --format:csv --date_format:unix > qqq.csv

# Now run the backtest
./pairs_trading spy.csv qqq.csv -v
```

The `--date_format:unix` flag is key—it outputs timestamps as Unix epoch
values, which Wu expects. Without it, you'd get human-readable dates
that would require parsing. The `--format:csv` flag ensures the output
is CSV with proper headers.

Want to test different time periods? Adjust the lookback:

```bash
# Last 5 years
yf history -s:spy --lookback:5y --format:csv --date_format:unix > spy_5y.csv

# Last 1 year
yf history -s:spy --lookback:1y --format:csv --date_format:unix > spy_1y.csv

# Specific date range
yf history -s:spy --start:2020-01-01 --end:2023-12-31 --format:csv --date_format:unix > spy_2020_2023.csv
```

This makes it easy to test how your strategy performs across different
market regimes—bull markets, bear markets, high volatility periods, or
calm sideways action.

---

## Understanding the Data Flow

Before writing code, understand how data flows through a Wu backtest:

```
CSV File(s) -> Reader(s) -> Strategy -> Portfolio
```

CSV files hold historical price data. Readers parse files line by line,
converting text into C structs. The strategy processes data, updates
indicators, and decides whether to buy, sell, or hold. The portfolio
receives decisions as signals and executes trades.

The runner synchronizes these components.

For pairs trading with two assets:

```
SPY CSV -> SPY Reader ──┐
                        ├─ Pairs Strategy -> [Signal 0, Signal 1] -> Multi-Asset Portfolio
QQQ CSV -> QQQ Reader ──┘
```

The strategy receives data from both assets, calculates the spread, and
generates two signals. The portfolio manages both positions with shared
cash.

---

## Step 1: Setting Up Includes and Basic Structure

Start by creating `pairs_trading.c`:

```c
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "wu.h"

int main(int argc, char* argv[]) {
    // Parse command-line arguments
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <spy.csv> <qqq.csv> [-v]\n", argv[0]);
        fprintf(stderr, "  -v: verbose output\n");
        return 1;
    }
    
    bool verbose = (argc > 3 && strcmp(argv[3], "-v") == 0);
    
    // We'll add more code here
    
    return 0;
}
```

The `wu.h` header includes all necessary sub-headers. We accept two CSV
file paths on the command line—one for SPY and one for QQQ—with an
optional `-v` flag for verbose output during the backtest.

---

## Step 2: Understanding Data Types

Wu uses three data structures:

The **Candle** represents OHLCV bar data:

```c
typedef struct WU_Candle_ {
    WU_TimeStamp timestamp;
    double open;
    double high;
    double low;
    double close;
    double volume;
    WU_DataType data_type;  // Always WU_DATA_TYPE_CANDLE
} WU_Candle;
```

Then there's the **Trade**, which captures individual transactions at
the tick level:

```c
typedef struct {
    WU_TimeStamp timestamp;
    double price;
    double volume;
    WU_Side side;           // WU_SIDE_BUY or WU_SIDE_SELL
    WU_DataType data_type;  // Always WU_DATA_TYPE_TRADE
} WU_Trade;
```

And finally, the **Single**, which is just a timestamped value—perfect
for indicators, prices, or any other scalar time series:

```c
typedef struct {
    WU_TimeStamp timestamp;
    double value;
    WU_DataType data_type;  // Always WU_DATA_TYPE_SINGLE_VALUE
} WU_Single;
```

For our pairs trading example, we'll work with Candles. We want the full
OHLCV picture, though the strategy will focus on closing prices to
calculate the spread between SPY and QQQ.

Now, strategies don't just consume data—they produce **Signals**. A
signal is an instruction to the portfolio, telling it what to do:

```c
typedef enum {
    WU_SIDE_HOLD = 0,  // Do nothing
    WU_SIDE_BUY = 1,   // Open long or close short
    WU_SIDE_SELL = 2   // Open short or close long
} WU_Side;

typedef struct {
    WU_TimeStamp timestamp;
    WU_Side side;      // Buy, sell, or hold
    double price;      // Execution price
    double quantity;   // Amount to trade (interpretation depends on position sizing)
} WU_Signal;
```

Think of signals as the language strategies use to communicate with portfolios. The strategy does the thinking, the portfolio does the executing.

### Understanding Timestamps

Every piece of data in Wu carries a timestamp that tracks when it occurred. The `WU_TimeStamp` struct provides flexibility in time representation:

```c
typedef enum {
    WU_TIME_UNIT_SECONDS = 0,
    WU_TIME_UNIT_MILLIS = 1,
    WU_TIME_UNIT_MICROS = 2,
    WU_TIME_UNIT_NANOS = 3
} WU_TimeUnit;

typedef struct WU_TimeStamp_ {
    int64_t mark;
    WU_TimeUnit units;
} WU_TimeStamp;
```

The `mark` field holds the actual time value, while `units` specifies the scale. This design lets you work with different time granularities without conversion overhead. Daily bar data might use seconds (`mark = 1678896000` for Unix epoch time), while high-frequency data might use microseconds or nanoseconds.

When calculating time-based metrics like annualized returns or borrowing costs, Wu converts between units internally. For example, calculating borrowing interest on short positions requires knowing how long the position was held. The library converts both timestamps to a common unit, computes the difference, and converts to years for the interest calculation.

This approach keeps timestamps compact (just 16 bytes) while supporting everything from daily data to tick-level precision. You choose the appropriate unit when creating readers, and the library handles the rest.

---

## Step 3: Creating CSV Readers

Now we'll open our data sources and create readers to parse them. Here's
where the data starts flowing:

```c
// Open CSV files
FILE* spy_file = fopen(argv[1], "r");
FILE* qqq_file = fopen(argv[2], "r");

if (!spy_file || !qqq_file) {
    fprintf(stderr, "Error: Could not open CSV files\n");
    if (spy_file) fclose(spy_file);
    if (qqq_file) fclose(qqq_file);
    return 1;
}

// Create CSV readers for both assets
WU_CsvReader spy_csv = wu_csv_reader_new(spy_file, WU_DATA_TYPE_CANDLE, 
        WU_TIME_UNIT_SECONDS, true);
WU_CsvReader qqq_csv = wu_csv_reader_new(qqq_file, WU_DATA_TYPE_CANDLE,
        WU_TIME_UNIT_SECONDS, true);

if (!spy_csv || !qqq_csv) {
    fprintf(stderr, "Error: Could not create CSV readers\n");
    fclose(spy_file);
    fclose(qqq_file);
    return 1;
}
```

The `WU_CsvReader` is a concrete implementation of the abstract
`WU_Reader` interface. Every reader in Wu shares the same contract: it
has a `next()` function that returns the next data point, and a
`delete()` function for cleanup. The CSV reader happens to read from
files, but you could implement readers that pull from databases, REST
APIs, or even generate synthetic data on the fly.

```c
typedef struct WU_Reader_ {
    void* (*next)(struct WU_Reader_* reader);
    void (*delete)(struct WU_Reader_* reader);
}* WU_Reader;
```

When we call `wu_csv_reader_new()`, we pass three arguments: an open
file handle, the type of data we expect to find in that file, and
whether the file has headers. The reader will parse each line,
converting strings to numbers, and package them into `WU_Candle`
structs. The `true` for headers tells it to skip the first line.

Why does the reader return `void*` instead of a specific type? Because
different readers might produce different types of data. The runner will
handle type checking and conversion when it wires everything together.
This abstraction keeps the reader simple while maintaining flexibility.
When we pass these to the runner later, we'll use the `WU_READER()`
macro to cast them to the base interface type.

---

## Step 4: Creating the Strategy

Pairs trading is a mean-reversion strategy. When two historically
correlated assets drift apart, the strategy bets on them converging. Let's
create our strategy:

```c
// Create pairs trading strategy
WU_PairsTradingStrat pairs_strat = wu_pairs_trading_strat_new(20, 2.0, 1.0);
```

Here's how it works. The strategy calculates the spread between two
assets—in our case, `spread = SPY_close - QQQ_close`. Over a rolling
window (20 periods in our example), it tracks the mean and standard
deviation of this spread. When the spread deviates significantly from
its mean, that's our signal.

If the spread drops below `mean - (2.0 × stdev)`, we interpret that as
SPY being undervalued relative to QQQ. The strategy generates a buy
signal for SPY and a sell signal for QQQ. We're betting the spread will
widen back to normal.

Conversely, if the spread rises above `mean + (2.0 × stdev)`, SPY looks
overvalued relative to QQQ. We sell SPY and buy QQQ, betting the spread
will narrow.

When the spread returns to its mean, we close both positions and take
our profit (or loss).

The three parameters give you control over the strategy's behavior. The
**window** determines how much history we consider when calculating
statistics. A shorter window (10 periods) makes the strategy more
responsive to recent changes but also noisier. A longer window (30
periods) smooths things out but may lag when conditions shift.

The **threshold** controls how extreme a deviation needs to be before we
act. Setting it to 1.5 standard deviations means more trading
opportunities but weaker signals. Bumping it up to 2.5 means fewer
trades, only on very strong deviations.

The **ratio** is the hedge ratio—how much of asset B we trade relative
to asset A. We're using 1.0 for simplicity, meaning equal dollar amounts
in each position. In practice, you might calculate an optimal ratio
using linear regression to minimize the variance of the spread, but
that's an advanced topic for another day.

Under the hood, the strategy maintains its own indicators—a simple
moving average for the spread's mean and a standard deviation
calculator. Every time new data arrives, it updates these indicators,
compares the current spread to the thresholds, and decides what signals
to generate.

The strategy interface itself is straightforward. It declares how many
inputs it expects (2 for pairs trading), what data types those inputs
should be (Candles for both), and how many outputs it produces (2
signals, one per asset). The runner will use this metadata to validate
that everything's wired up correctly before the backtest starts.

---

## Step 5: Configuring the Portfolio

The portfolio manages cash, executes trades, tracks positions, and
accounts for transaction costs. Let's configure one:

```c
// Configure multi-asset portfolio parameters
WU_PortfolioParams params = {
    .direction = WU_DIRECTION_BOTH,  // Allow both long and short positions
    .initial_cash = 100000.0,
    .execution_policy = {
        .policy = WU_EXECUTION_POLICY_FIXED_SLIPPAGE,
        .execution_mean = 0.0005,   // 0.05% slippage
        .execution_stddev = 0.0,
        .tx_cost_type = WU_TRANSACTION_COST_PROPORTIONAL,
        .tx_cost_value = 0.001,      // 0.1% transaction cost
        .stop_loss_pct = NAN,        // No stop loss
        .take_profit_pct = NAN       // No take profit
    },
    .borrow_params = {
        .rate = 0.05,                // 5% annual rate for borrowing
        .limit = 100000.0            // Can borrow up to $100k worth
    },
    .position_sizing = {
        .size_type = WU_POSITION_SIZE_PCT,
        .size_value = 0.45           // Use 45% of cash per asset
    }
};

// Create multi-asset portfolio
WU_BasicPortfolio portfolio = wu_basic_portfolio_new(
    params, 
    wu_symbol_list("SPY", "QQQ"));

if (!portfolio) {
    fprintf(stderr, "Error: Could not create multi-asset portfolio\n");
    return 1;
}
```

We start with $100,000 in virtual cash. The portfolio parameters control all aspects of trading behavior.

### Execution Policy

The `execution_policy` groups all trade execution settings. The `policy` field determines when and how orders execute. IMMEDIATE execution happens instantly at the signal price. NEXT_CLOSE is more realistic—it stores orders as pending and executes them at the next bar's close price, since you typically can't act on a signal until after you see it. FIXED_SLIPPAGE executes immediately but applies a fixed percentage slippage to the price. RANDOM_SLIPPAGE introduces variability by drawing slippage from a distribution.

Slippage parameters control execution price adjustments. The `execution_mean` field sets the average slippage. For FIXED_SLIPPAGE with `execution_mean = 0.0005`, every trade gets exactly 0.05% slippage. For RANDOM_SLIPPAGE, the `execution_stddev` field adds variability—slippage is sampled as `mean ± stddev × random`, modeling how market impact varies with conditions.

Transaction costs come in two flavors. FIXED costs charge a flat fee per trade regardless of size (useful for modeling per-trade commissions). PROPORTIONAL costs charge a percentage of the trade value (useful for percentage-based fees). The `tx_cost_value` field specifies either the dollar amount or percentage. On a $10,000 trade with 0.1% proportional costs and 0.05% slippage, total cost reaches $15.

Risk management controls automatic position exits. The `stop_loss_pct` closes positions that lose beyond a threshold, while `take_profit_pct` locks in gains above a threshold. Setting these to NAN disables automatic exits—the default behavior. For mean-reversion strategies like pairs trading, we use NAN because the strategy expects temporary losses to reverse. For momentum strategies, you might set `stop_loss_pct = 0.10` to limit losses to 10% and `take_profit_pct = 0.20` to capture 20% gains.

### Borrow Parameters

Short selling requires borrowing assets. The `borrow_params.rate` sets the annual interest rate charged on borrowed assets. At 5% annually, shorting $50,000 worth of stock for one day costs about $6.85 (calculated as 50000 × 0.05 ÷ 365). Interest compounds continuously as positions are held. The `borrow_params.limit` caps the maximum dollar value of short positions, preventing excessive leverage. In our example, the $100,000 limit means we can't have more than $100,000 worth of short positions open simultaneously.

### Position Sizing

Position sizing determines how much to trade when signals arrive. WU_POSITION_SIZE_ABS uses absolute quantities—setting `size_value = 100.0` means always trading exactly 100 shares. Simple, but it doesn't scale with your capital or adapt to price changes.

WU_POSITION_SIZE_PCT uses a percentage of available cash. Our configuration sets `size_value = 0.45`, meaning each trade consumes 45% of available cash. With two assets, that's 90% total exposure, leaving 10% as a buffer. As your capital grows or shrinks, position sizes scale proportionally.

WU_POSITION_SIZE_PCT_EQUAL splits capital equally across all assets. If you have three assets and set `size_value = 0.95`, each asset gets approximately 31.67% of total portfolio value (95% divided by 3). This automatically maintains balanced exposure as positions move.

WU_POSITION_SIZE_STRATEGY_GUIDED gives control to the strategy. Instead of fixed percentages, the strategy sets each `signal.quantity` to the desired portfolio allocation. This enables sophisticated approaches where allocation varies based on signal strength, volatility, or other factors.

### Direction

The `direction` parameter gates what trades the portfolio accepts. WU_DIRECTION_LONG restricts the portfolio to buying and holding long positions only. WU_DIRECTION_SHORT allows only short selling. WU_DIRECTION_BOTH permits bidirectional trading—going long one asset while shorting another. Pairs trading requires BOTH since the strategy alternates between long and short positions in each asset.

### Symbol Mapping

The `wu_symbol_list("SPY", "QQQ")` macro creates a null-terminated array mapping integer indices to symbol names. Index 0 corresponds to "SPY", index 1 to "QQQ". Internally, all operations use these integer indices for speed. The symbols only matter for display—when printing results, "SPY" is more meaningful than "asset 0". This keeps the fast path fast while maintaining readable output.

---

## Step 6: Creating the Runner

The runner coordinates the backtest components:

```c
WU_Runner runner = wu_runner_new(
    WU_PORTFOLIO(portfolio),
    WU_STRATEGY(pairs_strat),
    wu_reader_list(WU_READER(spy_csv), WU_READER(qqq_csv))
);

if (!runner) {
    fprintf(stderr, "Error: Could not create runner\n");
    wu_portfolio_delete(WU_PORTFOLIO(portfolio));
    wu_strategy_delete(WU_STRATEGY(pairs_strat));
    wu_reader_delete(WU_READER(spy_csv));
    wu_reader_delete(WU_READER(qqq_csv));
    return 1;
}
```

When you call `wu_runner_new()`, the runner validates the setup. It checks that the number of readers matches what the strategy expects, and verifies data type compatibility. If anything doesn't align, it returns NULL.

The macros `WU_PORTFOLIO()`, `WU_STRATEGY()`, and `WU_READER()` cast concrete types to their base interface types. This is C's approach to polymorphism—the runner works with interfaces, not implementations.

The `wu_reader_list()` macro creates a NULL-terminated array. The runner takes ownership of all components and manages their lifecycle through `wu_runner_free()`.

The runner's job is orchestrating the backtest loop:

```c
while (true) {
    // Read from all readers
    void* inputs[num_inputs];
    for (int i = 0; i < num_inputs; i++) {
        inputs[i] = wu_reader_next(readers[i]);
        if (!inputs[i]) goto done;  // EOF on any reader
    }
    
    // Generate signals
    WU_Signal* signals = wu_strategy_update(strategy, inputs);
    
    // Execute trades
    wu_portfolio_update(portfolio, signals);
    
    // (Optional) Print verbose output
}
```

Notice how it reads from all readers before calling the strategy. This synchronization is crucial for multi-asset strategies. The pairs trading strategy needs to see both SPY and QQQ data for the same timestamp to calculate the spread correctly. The runner handles this automatically.

If type conversion is needed, the runner does that too. Maybe your strategy expects Single values (just a price) but your reader produces Candles (full OHLCV bars). The runner will extract the closing price and convert the Candle to a Single before passing it to the strategy. This flexibility means you can mix and match components without writing conversion code yourself.

---

## Step 7: Running the Backtest

With all our components configured and wired together, it's time to actually run the simulation. This is where theory meets execution:

```c
printf("=== Pairs Trading Backtest: SPY vs QQQ ===\n");
printf("Strategy: Mean Reversion (20-period window, 2.0 std threshold)\n");
printf("Initial Capital: $%.2f\n", params.initial_cash);
printf("Position Sizing: %.0f%% cash per asset\n", params.position_sizing.size_value * 100);

// Run backtest
wu_runner_exec(runner, verbose);
```

That single function call, `wu_runner_exec()`, kicks off the entire simulation. Behind the scenes, a lot happens:

The runner starts by reading one candle from each CSV file. It passes both candles to the strategy, which calculates the spread between SPY and QQQ closing prices. The strategy updates its internal indicators—the simple moving average tracks the spread's mean, the standard deviation measures its volatility. Based on where the current spread sits relative to its historical mean and standard deviation, the strategy decides what to do.

If the spread is extremely low (below mean minus 2 standard deviations), the strategy generates two signals: buy SPY, sell QQQ. The portfolio receives these signals and springs into action. It calculates position sizes using our 45% rule, checks that we have enough cash, executes both trades, and deducts transaction costs and slippage. The cash balance decreases by the net cost of both trades, and the positions are recorded.

On the next iteration, the runner reads another pair of candles—the next time step in our historical data. The strategy sees that we already have open positions. Maybe the spread has widened back toward the mean, but not enough to trigger an exit yet. The strategy generates hold signals for both assets, and the portfolio does nothing. This continues, bar after bar, the spread widening and narrowing according to market dynamics.

Eventually, the spread crosses back through its mean. The strategy recognizes this as the exit condition and generates close signals. The portfolio sells our SPY position and covers our QQQ short, calculating the profit or loss on the trade. All the fees get tallied up—entry costs, exit costs, slippage both ways—and we're back to flat positions with updated cash.

If you passed the `-v` flag, you get to watch this drama unfold in real-time:

```
Iteration 0 | Signals: 2 | Value: 100000.00 | P&L: 0.00
Iteration 100 | Signals: 2 | Value: 96570.30 | P&L: -3429.70
Iteration 200 | Signals: 2 | Value: 101458.51 | P&L: 1458.51
Iteration 300 | Signals: 2 | Value: 106875.01 | P&L: 6875.01
...
Iteration 5000 | Signals: 2 | Value: 269392.49 | P&L: 169392.49
Backtest completed after 5028 iterations
```

Each line shows an iteration milestone with the number of signals generated (always 2 for pairs), current portfolio value, and cumulative P&L. You can see the strategy's performance evolve over time—starting slightly negative, then steadily climbing as mean-reversion opportunities are captured and profitable positions closed. This verbose output is invaluable when you're debugging a strategy or trying to understand why it behaved unexpectedly on certain dates.

The backtest continues until one of the CSV files runs out of data. Since we need both assets for pairs trading, the runner stops as soon as either file ends, ensuring we never try to trade on incomplete information.

---

## Step 8: Analyzing Results

The backtest has finished running, and now we want to know: did we make money? How many trades did we take? What was the win rate?

```json
{
  "portfolio": {
    "initial_cash": 100000.00,
    "current_cash": 91232.89,
    "portfolio_value": 203736.87,
    "pnl": 103736.87,
    "pnl_pct": 103.74,
    "tx_fees": 46902.85,
    "borrow_interest": 9279.91
  },
  "trades": {
    "total": 339,
    "winning": 191,
    "losing": 148,
    "win_rate": 56.34,
    "max_win": 25274.93,
    "max_loss": -18127.48,
    "stop_loss_exits": 0,
    "take_profit_exits": 0
  },
  "positions": [
    { "symbol": "SPY", "quantity": 244.6679, "value": 162963.49, "last_price": 666.06 },
    { "symbol": "QQQ", "quantity": -84.4850, "value": -50459.52, "last_price": 597.26 }
  ]
}
```

The stats module provides `wu_portfolio_stats_to_keyvalue()` for compact logging and `wu_portfolio_stats_to_json(stats, pretty)` for structured output.

---

## Complete Code

Here's the full `pairs_trading.c` file:

```c
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "wu.h"

static void print_stats(WU_BasicPortfolio portfolio) {
    WU_PortfolioStats stats = portfolio->stats;
    char* kv = wu_portfolio_stats_to_keyvalue(stats);
    if (kv) {
        printf("%s\n\n", kv);
        free(kv);
    }
    char* json = wu_portfolio_stats_to_json(stats, true);
    if (json) {
        printf("=== JSON Format ===\n%s\n", json);
        free(json);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <spy.csv> <qqq.csv> [-v]\n", argv[0]);
        fprintf(stderr, "  -v: verbose output (shows trading activity)\n");
        return 1;
    }
    bool verbose = (argc > 3 && strcmp(argv[3], "-v") == 0);
    FILE* spy_file = fopen(argv[1], "r");
    FILE* qqq_file = fopen(argv[2], "r");
    if (!spy_file || !qqq_file) {
        fprintf(stderr, "Error: Could not open CSV files\n");
        if (spy_file) fclose(spy_file);
        if (qqq_file) fclose(qqq_file);
        return 1;
    }
    WU_Reader spy_reader = (WU_Reader)wu_csv_reader_new(spy_file,
            WU_DATA_TYPE_CANDLE, WU_TIME_UNIT_SECONDS, true);
    WU_Reader qqq_reader = (WU_Reader)wu_csv_reader_new(qqq_file,
            WU_DATA_TYPE_CANDLE, WU_TIME_UNIT_SECONDS, true);
    if (!spy_reader || !qqq_reader) {
        fprintf(stderr, "Error: Could not create CSV readers\n");
        fclose(spy_file);
        fclose(qqq_file);
        return 1;
    }
    
    // Create pairs trading strategy
    // Parameters:
    //   window = 20 (lookback for spread statistics)
    //   threshold = 2.0 (entry/exit at 2 standard deviations)
    //   ratio = 1.0 (1:1 hedge ratio for simplicity)
    WU_Strategy strategy = (WU_Strategy)wu_pairs_trading_strat_new(20, 2.0, 1.0);
    
    WU_PortfolioParams params = {
        .direction = WU_DIRECTION_BOTH,
        .initial_cash = 100000.0,
        .execution_policy = {
            .policy = WU_EXECUTION_POLICY_FIXED_SLIPPAGE,
            .execution_mean = 0.0005,
            .execution_stddev = 0.0,
            .tx_cost_type = WU_TRANSACTION_COST_PROPORTIONAL,
            .tx_cost_value = 0.001,
            .stop_loss_pct = NAN,
            .take_profit_pct = NAN
        },
        .borrow_params = {
            .rate = 0.05,
            .limit = 100000.0
        },
        .position_sizing = {
            .size_type = WU_POSITION_SIZE_PCT,
            .size_value = 0.45
        }
    };
    
    WU_BasicPortfolio portfolio = wu_basic_portfolio_new(
        params, 
        wu_symbol_list("SPY", "QQQ"));
    
    if (!portfolio) {
        fprintf(stderr, "Error: Could not create multi-asset portfolio\n");
        return 1;
    }
    
    printf("=== Pairs Trading Backtest: SPY vs QQQ ===\n");
    printf("Strategy: Mean Reversion (20-period window, 2.0 std threshold)\n");
    printf("Initial Capital: $%.2f\n", params.initial_cash);
    printf("Position Sizing: %.0f%% cash per asset\n", params.position_sizing.size_value * 100);
    
    WU_Runner runner = wu_runner_new(
        WU_PORTFOLIO(portfolio),
        strategy,
        wu_reader_list(WU_READER(spy_reader), WU_READER(qqq_reader))
    );
    
    if (!runner) {
        fprintf(stderr, "Error: Could not create runner\n");
        wu_portfolio_delete((WU_Portfolio)portfolio);
        wu_strategy_delete(strategy);
        wu_reader_delete(spy_reader);
        wu_reader_delete(qqq_reader);
        return 1;
    }
    
    // Run backtest using the unified runner
    wu_runner_exec(runner, verbose);
    
    // Print final statistics (before freeing runner which owns the portfolio)
    print_stats(portfolio);
    
    // Cleanup (runner frees portfolio, strategy, and readers)
    wu_runner_free(runner);
    
    return 0;
}
```

---

## Building and Running

### Building the Example

```bash
# From the wu directory
cd examples/backtest

# Build all examples (including pairs_trading)
make

# Or build just this example
gcc -I../../include \
    -o pairs_trading pairs_trading.c \
    -L../../lib -lwu -Wl,-rpath,$(pwd)/../../lib
```

### Running the Example

```bash
# Basic run (silent mode)
./pairs_trading ../../tests/data/spy.csv ../../tests/data/qqq.csv

# Verbose mode (shows each trade)
./pairs_trading ../../tests/data/spy.csv ../../tests/data/qqq.csv -v
```

The strategy more than doubled the initial capital over the test period, with a 61.6% win rate across 250 trades. The remaining SPY position shows we're still long at the end of the backtest. Transaction costs totaled over $42,000—a significant factor that real-world backtests must account for.

These results illustrate the library's capabilities, but require careful interpretation given the limitations discussed below.

---

## Critical Limitations and Warnings

### This is not Evidence of Profitability

The 163% return is **in-sample testing**—running the strategy on historical data. This presents several issues:

**Parameter Selection**: The strategy parameters (20-period window, 2.0 standard deviation threshold, 1.0 hedge ratio) were chosen for this dataset. In real trading, optimal parameters aren't known in advance.

**Overfitting Risk**: Testing many parameter combinations increases the chance of finding one that fits historical noise rather than genuine patterns.

**Data Snooping**: Each strategy variation tested consumes information from the dataset. Multiple tests can produce seemingly profitable results by chance.

Strategies with high backtest returns may underperform in live trading due to the gap between historical simulation and real market conditions.

### Missing Risk Metrics

Profit and loss shows returns but not risk. Wu currently lacks risk metrics commonly used in professional trading:

**Sharpe Ratio**: Risk-adjusted returns, calculated as `(average_return - risk_free_rate) / standard_deviation_of_returns`. A 163% return with 200% volatility differs significantly from 20% with 5% volatility.

**Maximum Drawdown**: The worst peak-to-trough decline. A 50% drawdown requires 100% gains to recover. This metric affects psychological tolerance and position sizing decisions.

**Volatility**: How much the equity curve fluctuates. Higher volatility requires smaller positions to manage risk.

**Calmar Ratio**: Annual return divided by maximum drawdown. Shows return per unit of worst-case risk.

Without these metrics, you lack full visibility into strategy risk characteristics.

### Unrealistic Execution Model

Wu's execution model is simplified for clarity. This simplification has limitations:

**Infinite Liquidity Assumption**: Wu assumes you can instantly trade any quantity at the current price. Real markets have limited liquidity. Fixed slippage percentages don't capture nonlinear market impact.

**Market Orders Only**: Wu executes trades immediately at the signal price. Real trading involves trade-offs between execution speed and price certainty.

**No Partial Fills**: Orders might only partially fill in real markets. Wu assumes all-or-nothing execution.

**Simplified Shorting Model**: Wu models borrowing costs via a fixed annual rate. Real shorting involves variable borrow rates, availability constraints, and regulatory restrictions.

**No Slippage During Volatility**: Wu's 0.05% slippage is constant. Real slippage explodes during market stress—precisely when you most need to exit positions. That March 2020 crash? Good luck executing at any reasonable price.

### Pairs Trading Specific Weaknesses

Our implementation demonstrates the architecture, but it's not a serious pairs trading strategy:

**No Cointegration Validation**: Professional pairs traders test for cointegration using Augmented Dickey-Fuller tests, Johansen tests, or similar statistical methods. They verify that the spread is mean-reverting, not just correlated. We assume SPY and QQQ will revert because they look correlated, but correlation ≠ cointegration. The relationship could be spurious or degrading over time.

**Fixed Hedge Ratio**: We use `ratio = 1.0` for simplicity. Real implementations estimate the hedge ratio using rolling regression or Kalman filters, updating it as the relationship between assets evolves. A fixed ratio becomes stale quickly.

**Naive Spread Definition**: We calculate `spread = SPY - (ratio × QQQ)` in price space. More sophisticated approaches work in log-price space to capture multiplicative relationships, or use more complex models like Error Correction Models or state-space models.

**No Regime Detection**: Markets have regimes—trending, mean-reverting, high-volatility, low-volatility. Our strategy trades blindly through all of them. A production strategy would detect regime changes and adapt or shut down.

**No Out-of-Sample Testing**: We should develop parameters on one time period (say, 2006-2015), then validate on a separate period (2016-2021). This guards against overfitting. Wu doesn't enforce this workflow, making it easy to fool yourself.

### What About Real Trading?

Trading this strategy with real money requires addressing several practical challenges. Position limits matter—deploying $45,000 per asset might move the market against you. Your broker won't give unlimited margin for the short leg, and margin maintenance requirements fluctuate with volatility. Shorting costs vary daily; hard-to-borrow stocks become expensive or unavailable. Regulatory constraints like pattern day trader rules impose trading frequency limits. Execution latency means prices move between signal generation and order arrival. Markets gap overnight, bypassing stop losses. Black swan events like March 2020 or September 2008 break correlations exactly when you need them.

### Wu's Purpose and Your Responsibility

Wu is a teaching tool and an architecture exploration platform. It demonstrates how to structure a backtesting system using clean abstractions and composable components. The code is meant to be read, understood, and modified.

Use Wu to learn how backtesting systems work, experiment with strategy ideas, and understand the interplay between data, signals, and execution. Use it to build intuition about trading mechanics.

**But**: Treat backtest results with extreme skepticism. Add proper validation workflows. Calculate risk metrics. Test out-of-sample. Understand that profitable backtests are a starting point for investigation, not proof of profitability.

And most importantly: never trade real money based solely on a backtest, especially one from an experimental learning library. If you decide to move toward real trading, engage with the existing research literature, implement proper risk management, and start with small position sizes while you validate your assumptions against reality.

Wu gives you the building blocks. What you build with them, and whether it actually works in live markets, is entirely your responsibility.

---

## Next Steps

### Experimenting with Parameters

Now that you've built a working pairs trading system, the real fun begins: experimentation. Try tweaking the strategy parameters and watch how the results change.

Want more trading opportunities? Lower the window to 10 periods and the threshold to 1.5 standard deviations. The strategy becomes more aggressive, jumping on smaller deviations. You'll see more trades, but be prepared for more whipsaws when the spread doesn't revert as expected.

Prefer a more conservative approach? Bump the window up to 30 periods and the threshold to 2.5. Now you're only trading on truly extreme deviations. Fewer trades, but each one is backed by stronger statistical evidence.

The hedge ratio is another knob worth turning. Our simple 1:1 ratio works fine, but it's not optimal. In reality, SPY and QQQ don't move in perfect lockstep. You can calculate a better ratio using linear regression:

```
ratio = covariance(SPY, QQQ) / variance(QQQ)
```

This ratio minimizes the variance of the spread, giving you a more stable mean-reversion signal. Load up Python or R, crunch the numbers on your historical data, and plug in the calculated ratio. You'll often see improved performance.

### Trying Different Asset Pairs

Pairs trading isn't limited to SPY and QQQ. The strategy works on any two assets that tend to move together. Try some classics:

Energy stocks often pair well—run the backtest on Exxon (XOM) versus Chevron (CVX). Big tech companies can work too—Microsoft (MSFT) versus Google (GOOGL). Consumer staples offer another playground—Coca-Cola (KO) versus PepsiCo (PEP).

The key is finding pairs with genuine economic relationships. Two random stocks that happened to correlate in the past won't cut it. You want pairs where there's a fundamental reason they should move together—same industry, similar business models, shared customer bases.

### Creating Your Own Strategies

Ready to move beyond the built-in strategies? Building a custom strategy means implementing the `WU_Strategy` interface. Here's a skeleton to get you started:

```c
typedef struct MyCustomStrat_ {
    struct WU_Strategy_ base;
    // Your custom state goes here
    WU_SMA sma;
    double threshold;
    // Add more indicators, parameters, whatever you need
} MyCustomStrat;

WU_Signal* my_custom_strat_update(WU_Strategy strategy, const void* inputs[]) {
    MyCustomStrat self = (MyCustomStrat)strategy;
    WU_Candle* candle = (WU_Candle*)inputs[0];
    
    // Your logic here
    wu_indicator_update(self->sma, candle->close);
    double sma_value = wu_indicator_get(self->sma);
    
    // Generate signal based on your rules
    if (candle->close > sma_value + self->threshold) {
        self->base.signal_buffer[0] = wu_signal_init(
            candle->timestamp, WU_SIDE_BUY, candle->close, 0.0
        );
    } else {
        self->base.signal_buffer[0] = wu_signal_init(
            candle->timestamp, WU_SIDE_HOLD, candle->close, 0.0
        );
    }
    
    return self->base.signal_buffer;
}
```

The pattern is always the same: maintain your state in a struct, implement an update function that processes new data and generates signals, and populate the signal buffer. Want to build a momentum strategy? Track rate-of-change indicators. Mean reversion? Use Bollinger Bands. Machine learning? Feed the price data through your model and convert predictions to signals.

### How the Pairs Trading Strategy is Implemented

Let's peek under the hood and see how the pairs trading strategy actually works. Understanding the implementation will help you build your own strategies and appreciate the design patterns Wu uses.

The strategy lives in `src/strategies/pairs_trading.c`. Here's the complete implementation with detailed explanation:

```c
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include "wu.h"

#define NUM_INPUTS 2
#define NUM_OUTPUTS 2

static const WU_DataType input_types[] = {
    WU_DATA_TYPE_SINGLE_VALUE,
    WU_DATA_TYPE_SINGLE_VALUE
};

static WU_Signal signal_buffer[NUM_OUTPUTS];
```

The strategy declares its requirements using static constants. It needs
two inputs of `WU_DATA_TYPE_SINGLE_VALUE` and produces two signals. The
static signal buffer is shared across all calls.

Now the update function that runs on each new data point:

```c
static WU_Signal* pairs_trading_strat_update(struct WU_Strategy_* strat_,
                                             const void* inputs[]) {
    WU_PairsTradingStrat strat = (WU_PairsTradingStrat)strat_;
    
    // Validate input count
    assert(strat->base.num_inputs == NUM_INPUTS);
    
    // Cast inputs with proper types
    const WU_Single* asset_a = (const WU_Single*)inputs[0];
    const WU_Single* asset_b = (const WU_Single*)inputs[1];
    
    // Validate input types
    assert(asset_a->data_type == WU_DATA_TYPE_SINGLE_VALUE);
    assert(asset_b->data_type == WU_DATA_TYPE_SINGLE_VALUE);
    
    // Initialize signal buffer with HOLD signals for both assets
    strat->base.signal_buffer[0] = wu_signal_init(asset_a->timestamp, WU_SIDE_HOLD, 
                                                     asset_a->value, 1.0);
    strat->base.signal_buffer[1] = wu_signal_init(asset_b->timestamp, WU_SIDE_HOLD, 
                                                     asset_b->value, 1.0);
```

The function starts defensively with assertions. In production, you might use runtime checks instead, but during development, assertions catch bugs early. We cast the void pointers to the expected types—`WU_Single` in this case—and verify that we actually received what we expected.

The signal buffer gets initialized with HOLD signals as the default. If nothing interesting happens with the spread, we'll just return these hold signals and the portfolio will do nothing. This is the base case.

```c
    // Calculate the spread: spread = asset_a - (ratio * asset_b)
    double spread = asset_a->value - (strat->ratio * asset_b->value);
    
    // Update spread statistics
    double spread_mean = wu_indicator_update(strat->spread_ma, spread);
    double spread_stddev = wu_indicator_update(strat->spread_std, spread);
    
    // Wait for indicators to warm up
    if (isnan(spread_mean) || isnan(spread_stddev))
        return strat->base.signal_buffer;
```

Here's where the math happens. We calculate the spread between the two assets, adjusted by the hedge ratio. If the ratio is 1.0, it's just `SPY - QQQ`. If we determined through regression that the optimal ratio is 1.2, we'd calculate `SPY - (1.2 × QQQ)`.

Then we update our indicators. The simple moving average tracks the spread's mean over our window (20 periods by default). The standard deviation measures how much the spread typically bounces around that mean. These indicators return `NaN` initially because they need a full window of data before they can produce valid values. We check for this and return early if the indicators aren't ready yet.

```c
    // Calculate entry/exit thresholds
    double upper_band = spread_mean + strat->threshold * spread_stddev;
    double lower_band = spread_mean - strat->threshold * spread_stddev;
    
    if (spread < lower_band && strat->last_signal != WU_SIDE_BUY) {
        // Spread below lower band: Asset A undervalued
        // Signal to BUY Asset A and SELL Asset B
        strat->last_signal = WU_SIDE_BUY;
        strat->base.signal_buffer[0].side = WU_SIDE_BUY;
        strat->base.signal_buffer[1].side = WU_SIDE_SELL;
    } 
    else if (spread > upper_band && strat->last_signal != WU_SIDE_SELL) {
        // Spread above upper band: Asset A overvalued
        // Signal to SELL Asset A and BUY Asset B
        strat->last_signal = WU_SIDE_SELL;
        strat->base.signal_buffer[0].side = WU_SIDE_SELL;
        strat->base.signal_buffer[1].side = WU_SIDE_BUY;
    }
```

Now we apply the pairs trading logic. We calculate Bollinger-Band-style thresholds: mean plus or minus some multiple of standard deviation. With a threshold of 2.0, we're looking at 2-sigma events—roughly the most extreme 5% of historical spread values.

When the spread drops below the lower band, we interpret that as asset A being undervalued. We generate a buy signal for asset A (index 0) and a sell signal for asset B (index 1). We're going long-short, betting the spread will widen back toward the mean.

The `last_signal` check prevents duplicate entries. If we're already long from a previous signal, we don't want to enter again just because the spread is still below the lower band. We only act on state transitions.

```c
    else if (strat->last_signal != WU_SIDE_HOLD) {
        // Check if spread has mean-reverted (exit condition)
        bool spread_reverted = false;
        
        if (strat->last_signal == WU_SIDE_BUY && spread > spread_mean) {
            // We were long (bought at low spread), now spread returned to mean
            spread_reverted = true;
        }
        else if (strat->last_signal == WU_SIDE_SELL && spread < spread_mean) {
            // We were short (sold at high spread), now spread returned to mean
            spread_reverted = true;
        }
        
        if (spread_reverted) {
            // Mean reversion occurred - close position
            // Generate opposite signals to close both positions
            strat->base.signal_buffer[0].side = (strat->last_signal == WU_SIDE_BUY) ? 
                                                  WU_SIDE_SELL : WU_SIDE_BUY;
            strat->base.signal_buffer[1].side = (strat->last_signal == WU_SIDE_BUY) ? 
                                                  WU_SIDE_BUY : WU_SIDE_SELL;
            strat->last_signal = WU_SIDE_HOLD;
        }
    }
    
    return strat->base.signal_buffer;
}
```

The exit logic is where mean reversion pays off—or doesn't. If we have an open position (`last_signal != WU_SIDE_HOLD`), we watch for the spread to cross back through its mean. If we bought when the spread was low and it's now risen above the mean, that's our exit. If we sold when the spread was high and it's now fallen below the mean, that's also our exit.

When mean reversion occurs, we generate closing signals. If we were long asset A and short asset B, we now sell asset A and buy asset B to flatten both positions. The portfolio will calculate our profit or loss, deduct costs, and update the statistics.

Notice how the strategy maintains state across calls. The `last_signal` field remembers what we did last time, the indicators accumulate history internally, and all this state lives explicitly in the struct. No hidden globals, no mysterious side effects.

Here's the cleanup function:

```c
static void pairs_trading_strat_free(struct WU_Strategy_* strategy) {
    WU_PairsTradingStrat strat = (WU_PairsTradingStrat)strategy;
    wu_indicator_delete(strat->spread_ma);
    wu_indicator_delete(strat->spread_std);
    free(strat);
}
```

When the runner frees the strategy, we need to clean up our indicators first. They allocated memory internally for their rolling windows, and we're responsible for releasing that. After the indicators are deleted, we free the strategy struct itself.

Finally, the constructor that wires everything together:

```c
WU_PairsTradingStrat wu_pairs_trading_strat_new(int window, double threshold, 
                                                  double ratio) {
    WU_PairsTradingStrat strat = malloc(sizeof(struct WU_PairsTradingStrat_));
    
    // Set up base strategy interface
    strat->base.update = pairs_trading_strat_update;
    strat->base.delete = pairs_trading_strat_free;
    
    // Declare input requirements (point to static const)
    strat->base.input_types = input_types;
    strat->base.num_inputs = NUM_INPUTS;
    
    // Declare output count (symbols set by runner)
    strat->base.output_symbols = NULL;  // Will be set by runner
    strat->base.num_outputs = NUM_OUTPUTS;
    strat->base.signal_buffer = signal_buffer;
    
    // Initialize spread statistics indicators
    strat->spread_ma = wu_sma_new(window);
    strat->spread_std = wu_stdev_new(window, 1);  // dof=1 for sample std deviation
    
    // Initialize strategy parameters
    strat->threshold = threshold;
    strat->ratio = ratio;
    strat->last_signal = WU_SIDE_HOLD;
    
    return strat;
}
```

The constructor allocates the strategy struct and populates the base interface—the function pointers that make polymorphism work in C. It points to our static input type array so the runner knows what data we expect. It creates the indicators we need (SMA and standard deviation) with the specified window size. And it stores the parameters we'll use during signal generation.

Notice that `output_symbols` is NULL. The runner will fill this in based on the portfolio's asset list. This separation keeps the strategy generic—it doesn't need to know whether it's trading SPY/QQQ, XOM/CVX, or any other pair. The strategy just deals with "asset 0" and "asset 1", and the portfolio maps those indices to real symbols.

This implementation pattern—struct with function pointers, explicit state, static metadata—is how all Wu strategies work. Once you understand this pattern, you can implement any strategy you can imagine.

### Advanced Topics Worth Exploring

**Dynamic Position Sizing**: Use the `WU_POSITION_SIZE_STRATEGY_GUIDED` mode to let your strategy control capital allocation on the fly. Maybe you want to increase exposure when volatility is low and reduce it when markets get choppy. In your strategy's update function, set each signal's quantity to the desired portfolio percentage:

```c
// Allocate more to strong signals, less to weak ones
signals[0].quantity = 0.6;  // 60% to asset 0
signals[1].quantity = 0.4;  // 40% to asset 1
```

**Custom Data Sources**: The `WU_Reader` interface isn't limited to CSV files. Implement your own reader that pulls from PostgreSQL, queries a REST API, or generates synthetic data for Monte Carlo simulations:

```c
typedef struct MyReader_ {
    struct WU_Reader_ base;
    // Your data source connections, state, whatever
} MyReader;

void* my_reader_next(WU_Reader reader) {
    // Fetch the next data point from your source
    // Return a pointer to WU_Candle, WU_Trade, or WU_Single
}
```

**Multi-Asset Portfolios**: Our pairs trading example uses two assets, but Wu handles as many as you want. Create a four-asset portfolio and implement a strategy that rotates between them based on momentum rankings:

```c
WU_AssetSymbol symbols[] = {"SPY", "QQQ", "TLT", "GLD"};
WU_BasicPortfolio portfolio = wu_basic_portfolio_new(params, symbols, 4);
```

Your strategy needs to return four signals, one per asset. You could implement sector rotation, risk parity, or other multi-asset approaches.

**Python Integration**: Wu includes SWIG-generated bindings for Python development:

```python
import wu

portfolio = wu.BasicPortfolio(...)
strategy = wu.CrossOverStrat(10, 30, 0.0)
runner = wu.Runner(portfolio, strategy, [reader])
runner.exec(verbose=False)
```

You can prototype strategies in Python, then port performance-critical pieces to C as needed.

---

## Summary

You've built a working pairs trading backtester. You've learned Wu's
architecture—how Readers supply data, Strategies generate signals,
Portfolios execute trades, and Runners orchestrate everything. The
components compose with flexibility.

Pairs trading itself is a beautiful strategy: when two correlated assets drift apart, bet on convergence. Calculate the spread, track its statistics, trade the extremes, exit at the mean. Simple in concept, nuanced in execution.

Multi-asset portfolios add another dimension—shared cash pools, independent positions per asset, index-based tracking for performance. The position sizing system gives you four different ways to allocate capital, from simple absolute quantities to sophisticated strategy-guided allocations.

The runner pattern handles validation, synchronization, and type
conversion. The metrics system tracks P&L, win rates, transaction costs,
and per-asset holdings.

Wu provides low-level building blocks that compose into complete systems.
Swap strategies, customize portfolios, implement data sources as needed.
Explicit state management and strong typing maintain debuggability and
flexibility.

Happy backtesting!
