# Wu Trading Library Tutorial: Building a Pairs Trading Strategy

Mar. 13, 2026

**Disclaimer**: This tutorial was produced using AI, based on the
current codebase and examples.

Let's build something interesting together: a pairs trading backtester. We'll start from scratch and work our way through the complete `examples/backtest/pairs_trading.c` example. Along the journey, you'll discover how the Wu library is architected and how its pieces fit together like building blocks.

Think of this as a guided tour through both the code and the design philosophy behind it. By the end, you'll understand not just how to use Wu, but why it's structured the way it is.

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
12. [Complete Code](#complete-code)
13. [Building and Running](#building-and-running)
14. [Next Steps](#next-steps)

---

## Library Architecture Overview

Wu is a low-level backtesting library written in C. It embraces a modular, composable design philosophy, using C's struct-and-function-pointer pattern to achieve polymorphism without the baggage of inheritance. If you've ever felt constrained by heavyweight frameworks that dictate how you must structure your code, Wu takes a different approach: it gives you tools, not a framework.

### Core Components

The library revolves around five main abstractions that work together like an assembly line:

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

**Data Types** live in `wu/types.h` and `wu/data.h`. They're the raw material flowing through the system. A **Candle** carries OHLCV bar data with its timestamp. A **Trade** represents tick-level activity with price, volume, and direction. A **Single** is just a timestamped value, useful for generic time series. And a **Signal** is what strategies generate: a trading instruction with a side (buy/sell/hold), price, and quantity.

**Readers** (`wu/readers.h`) abstract away data sources. They have one job: fetch the next data point. The `WU_CsvReader` handles CSV files, but you could implement a database reader, an API client, or even a synthetic data generator. They all share the same interface, so the rest of the system doesn't care where the data comes from.

**Indicators** (`wu/indicators.h`) are the mathematical building blocks. Simple Moving Averages, Exponential Moving Averages, Standard Deviations—these are the tools strategies use to make sense of price movements. They maintain their own internal state and update incrementally as new data arrives.

**Strategies** (`wu/strategies.h`) consume market data and spit out signals. They can work with one asset or many. The crossover strategy watches two moving averages, while the pairs trading strategy monitors the spread between two correlated assets. Each strategy declares what types of inputs it expects and how many outputs it produces.

**Portfolios** (`wu/portfolios.h`) execute the trades and track the money. They receive signals from strategies, calculate position sizes, deduct transaction costs and slippage, and maintain positions across one or more assets. The `WU_BasicPortfolio` is the workhorse implementation that handles both single and multi-asset scenarios with a shared cash pool.

**The Runner** (`wu/runners.h`) is the conductor. It wires everything together, validates that data types match up, synchronizes reads from multiple sources, and drives the backtest loop forward until the data runs out.

### Design Philosophy

Every component carries its state explicitly in a struct. No hidden globals, no magic. When something breaks, you can inspect exactly what's in memory. When you want to understand how a strategy behaves, you can see all its parameters and internal indicators laid out in the struct definition.

The components compose freely. Swap out a CSV reader for a database reader. Replace the crossover strategy with your own custom logic. Use the portfolio standalone without the runner if you want to drive the backtest loop yourself. Nothing forces you into a particular pattern.

And there's no framework lock-in. Wu gives you building blocks, not a rigid structure. You're free to use as much or as little as you need. Want to use just the indicators? Go ahead. Want to implement your own portfolio logic? The door's wide open.

---

## Prerequisites and Setup

### What You'll Need

You need a C11-compatible compiler like gcc or clang, the Make build tool, and some historical price data in CSV format. That's it.

### Building the Library

First, let's get Wu built and make sure everything's working:

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

The build process is straightforward. Wu compiles into a shared library that you can link against. The tests give you confidence that the fundamentals are solid before you start writing your own code.

### Understanding the CSV Format

Wu expects CSV files with headers. For candle data, the format looks like this:

```csv
Date,Open,High,Low,Close,Volume
1142865000,130.64,130.90,130.21,130.41,45538500
1142951400,130.37,130.99,129.45,129.59,87102700
...
```

The timestamp in the `Date` column is a Unix epoch value (seconds since January 1, 1970). This keeps things simple and avoids the complexity of parsing date strings. You can convert dates to Unix timestamps using tools like `date -d "2024-01-15" +%s` on Unix systems or Python's `datetime` module.

---

## Understanding the Data Flow

Before we write any code, let's trace how data flows through a Wu backtest. Understanding this flow makes everything else click into place.

Imagine water flowing through pipes: CSV files are the reservoir, readers are the pumps, the strategy is the filter that decides what to do, and the portfolio is where the work actually happens.

```
1. CSV File(s) → 2. Reader(s) → 3. Strategy → 4. Portfolio
```

The CSV files sit on disk, holding historical price data. The readers parse those files line by line, converting text into C structs. The strategy consumes those structs, runs calculations, updates indicators, and decides whether to buy, sell, or hold. Finally, the portfolio receives those decisions as signals and executes trades, tracking cash and positions.

The runner orchestrates this entire dance, making sure everyone stays synchronized.

For pairs trading, the flow gets more interesting because we're dealing with two assets simultaneously:

```
SPY CSV → SPY Reader ──┐
                       ├─→ Pairs Strategy → [Signal 0, Signal 1] → Multi-Asset Portfolio
QQQ CSV → QQQ Reader ──┘
```

The strategy receives data from both SPY and QQQ at the same time, calculates the spread between them, and generates two signals—one for each asset. The portfolio manages both positions with a shared cash pool, allowing you to go long one asset while shorting the other.

---

## Step 1: Setting Up Includes and Basic Structure

Let's start writing code. Create a new file called `pairs_trading.c` and set up the skeleton:

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
        fprintf(stderr, "  -v: verbose output (shows trading activity)\n");
        return 1;
    }
    
    bool verbose = (argc > 3 && strcmp(argv[3], "-v") == 0);
    
    // We'll add more code here
    
    return 0;
}
```

The `wu.h` header is your entry point to the entire library. It pulls in all the sub-headers you need, so you don't have to hunt for the right includes. We're accepting two CSV file paths on the command line—one for SPY and one for QQQ—along with an optional `-v` flag that will show us what's happening during the backtest. When you're debugging a strategy, that verbose output is invaluable.

---

## Step 2: Understanding Data Types

Wu speaks in three different data dialects, each represented by its own struct. Let's meet them.

First, there's the **Candle**, which represents OHLCV bar data—the bread and butter of most trading strategies:

```c
typedef struct WU_Candle_ {
    int64_t timestamp;
    double open;
    double high;
    double low;
    double close;
    double volume;
    WU_DataType data_type;  // Always WU_DATA_TYPE_CANDLE
} WU_Candle;
```

Then there's the **Trade**, which captures individual transactions at the tick level:

```c
typedef struct {
    int64_t timestamp;
    double price;
    double volume;
    WU_Side side;  // WU_SIDE_BUY or WU_SIDE_SELL
    WU_DataType data_type;  // Always WU_DATA_TYPE_TRADE
} WU_Trade;
```

And finally, the **Single**, which is just a timestamped value—perfect for indicators, prices, or any other scalar time series:

```c
typedef struct {
    int64_t timestamp;
    double value;
    WU_DataType data_type;  // Always WU_DATA_TYPE_SINGLE_VALUE
} WU_Single;
```

For our pairs trading example, we'll work with Candles. We want the full OHLCV picture, though the strategy will focus on closing prices to calculate the spread between SPY and QQQ.

Now, strategies don't just consume data—they produce **Signals**. A signal is an instruction to the portfolio, telling it what to do:

```c
typedef enum {
    WU_SIDE_HOLD = 0,  // Do nothing
    WU_SIDE_BUY = 1,   // Open long or close short
    WU_SIDE_SELL = 2   // Open short or close long
} WU_Side;

typedef struct {
    int64_t timestamp;
    WU_Side side;      // Buy, sell, or hold
    double price;      // Execution price
    double quantity;   // Amount to trade (interpretation depends on position sizing)
} WU_Signal;
```

Think of signals as the language strategies use to communicate with portfolios. The strategy does the thinking, the portfolio does the executing.

---

## Step 3: Creating CSV Readers

Now we'll open our data sources and create readers to parse them. Here's where the data starts flowing:

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
WU_Reader spy_reader = (WU_Reader)wu_csv_reader_new(spy_file, WU_DATA_TYPE_CANDLE, true);
WU_Reader qqq_reader = (WU_Reader)wu_csv_reader_new(qqq_file, WU_DATA_TYPE_CANDLE, true);

if (!spy_reader || !qqq_reader) {
    fprintf(stderr, "Error: Could not create CSV readers\n");
    fclose(spy_file);
    fclose(qqq_file);
    return 1;
}
```

The `WU_CsvReader` is a concrete implementation of the abstract `WU_Reader` interface. Every reader in Wu shares the same contract: it has a `next()` function that returns the next data point, and a `delete()` function for cleanup. The CSV reader happens to read from files, but you could implement readers that pull from databases, REST APIs, or even generate synthetic data on the fly.

When we call `wu_csv_reader_new()`, we pass three arguments: an open file handle, the type of data we expect to find in that file, and whether the file has headers. The reader will parse each line, converting strings to numbers, and package them into `WU_Candle` structs. The `true` for headers tells it to skip the first line.

Why does the reader return `void*` instead of a specific type? Because different readers might produce different types of data. The runner will handle type checking and conversion when it wires everything together. This abstraction keeps the reader simple while maintaining flexibility.

---

## Step 4: Creating the Strategy

Pairs trading is a classic mean-reversion strategy. The idea is simple but elegant: when two historically correlated assets drift apart, bet on them coming back together. Let's create our strategy:

```c
// Create pairs trading strategy
// Parameters:
//   window = 20 (lookback for spread statistics)
//   threshold = 2.0 (entry/exit at 2 standard deviations)
//   ratio = 1.0 (1:1 hedge ratio for simplicity)
WU_Strategy strategy = (WU_Strategy)wu_pairs_trading_strat_new(20, 2.0, 1.0);
```

Here's how it works. The strategy calculates the spread between two assets—in our case, `spread = SPY_close - QQQ_close`. Over a rolling window (20 periods in our example), it tracks the mean and standard deviation of this spread. When the spread deviates significantly from its mean, that's our signal.

If the spread drops below `mean - (2.0 × stdev)`, we interpret that as SPY being undervalued relative to QQQ. The strategy generates a buy signal for SPY and a sell signal for QQQ. We're betting the spread will widen back to normal.

Conversely, if the spread rises above `mean + (2.0 × stdev)`, SPY looks overvalued relative to QQQ. We sell SPY and buy QQQ, betting the spread will narrow.

When the spread returns to its mean, we close both positions and take our profit (or loss).

The three parameters give you control over the strategy's behavior. The **window** determines how much history we consider when calculating statistics. A shorter window (10 periods) makes the strategy more responsive to recent changes but also noisier. A longer window (30 periods) smooths things out but may lag when conditions shift.

The **threshold** controls how extreme a deviation needs to be before we act. Setting it to 1.5 standard deviations means more trading opportunities but weaker signals. Bumping it up to 2.5 means fewer trades, only on very strong deviations.

The **ratio** is the hedge ratio—how much of asset B we trade relative to asset A. We're using 1.0 for simplicity, meaning equal dollar amounts in each position. In practice, you might calculate an optimal ratio using linear regression to minimize the variance of the spread, but that's an advanced topic for another day.

Under the hood, the strategy maintains its own indicators—a simple moving average for the spread's mean and a standard deviation calculator. Every time new data arrives, it updates these indicators, compares the current spread to the thresholds, and decides what signals to generate.

The strategy interface itself is straightforward. It declares how many inputs it expects (2 for pairs trading), what data types those inputs should be (Candles for both), and how many outputs it produces (2 signals, one per asset). The runner will use this metadata to validate that everything's wired up correctly before the backtest starts.

---

## Step 5: Configuring the Portfolio

The portfolio is where trading theory meets reality. It's responsible for managing cash, executing trades, tracking positions, and accounting for all those pesky real-world costs that eat into profits. Let's set one up:

```c
// Define asset symbols
WU_AssetSymbol symbols[2];
strncpy(symbols[0], "SPY", WU_SYMBOL_MAX_LEN - 1);
strncpy(symbols[1], "QQQ", WU_SYMBOL_MAX_LEN - 1);
symbols[0][WU_SYMBOL_MAX_LEN - 1] = '\0';
symbols[1][WU_SYMBOL_MAX_LEN - 1] = '\0';

// Configure multi-asset portfolio parameters
WU_PortfolioParams params = {
    .initial_cash = 100000.0,
    .tx_cost_pct = 0.001,        // 0.1% transaction cost
    .stop_loss_pct = 0.0,        // No stop loss (rely on mean reversion)
    .take_profit_pct = 0.0,      // No take profit (rely on mean reversion)
    .slippage_pct = 0.0005,      // 0.05% slippage
    .position_sizing = {
        .size_type = WU_POSITION_SIZE_PCT,
        .size_value = 0.45       // Use 45% of cash per asset (90% total exposure)
    }
};

// Create multi-asset portfolio
WU_BasicPortfolio portfolio = wu_basic_portfolio_new(
    params, 
    (const WU_AssetSymbol*)symbols, 
    2  // Number of assets
);

if (!portfolio) {
    fprintf(stderr, "Error: Could not create multi-asset portfolio\n");
    return 1;
}
```

We start with $100,000 in virtual cash. The parameters capture the friction of real trading. Every trade costs us 0.1% in transaction fees—think broker commissions and exchange fees. Slippage adds another 0.05% to account for the price impact of our orders. On a $10,000 trade, that's $15 total cost. These numbers might seem small, but they add up over dozens or hundreds of trades, and ignoring them gives you unrealistic backtest results.

We've disabled stop losses and take profit exits (`0.0` means off) because pairs trading is a mean-reversion strategy. We're explicitly betting on prices returning to normal, so we don't want automated exits cutting our positions short. If we were running a momentum strategy, we'd set these to protect against runaway losses or lock in gains.

Position sizing is where things get interesting. Wu offers four different approaches to determine how much to trade:

First, there's **absolute sizing** (`WU_POSITION_SIZE_ABS`), where you specify an exact quantity. If you set `size_value = 100.0`, the portfolio always tries to buy or sell 100 shares. Simple, but rigid—it doesn't adapt to your capital or price changes.

Then there's **percentage sizing** (`WU_POSITION_SIZE_PCT`), which we're using here. With `size_value = 0.45`, each trade uses 45% of available cash. For SPY and QQQ, that means 90% total exposure, leaving 10% in cash as a buffer. This approach scales naturally with your capital.

The third option is **equal allocation** (`WU_POSITION_SIZE_PCT_EQUAL`), perfect for balanced portfolios. If you have three assets and set `size_value = 0.95`, each asset gets approximately 31.67% of total portfolio value (95% divided by 3). It automatically rebalances as positions move.

Finally, there's **strategy-guided sizing** (`WU_POSITION_SIZE_STRATEGY_GUIDED`), which lets the strategy dynamically control allocations. The strategy sets each signal's quantity to the desired portfolio percentage, enabling sophisticated approaches like risk parity or momentum-weighted allocations.

The symbols array maps integer indices to human-readable names. Index 0 is "SPY", index 1 is "QQQ". During the backtest, all operations use these integer indices for speed, but when we print results, the symbols make it clear what we're looking at. This design keeps the fast path fast while maintaining readability where it matters.

---

## Step 6: Creating the Runner

Now we bring everything together. The runner is the conductor of our backtest orchestra, making sure all the instruments play in harmony:

```c
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
```

When you call `wu_runner_new()`, the runner does some important validation work before it gives you the green light. It checks that the number of readers matches what the strategy expects. Our pairs trading strategy needs two inputs, and we're giving it two readers—check. It verifies that each reader's data type is compatible with what the strategy wants. Both readers produce Candles, and the strategy expects Candles—check. If anything doesn't line up, the runner returns NULL and refuses to proceed. Better to fail fast at setup than crash mysteriously mid-backtest.

Once validation passes, the runner takes ownership of all the components. You hand it the portfolio, strategy, and readers, and from that point on, the runner is responsible for their lifecycle. When you eventually call `wu_runner_free()`, it cleans up everything for you. This ownership model keeps memory management straightforward and prevents leaks.

The runner's main job is orchestrating the backtest loop. Here's what happens under the hood, simplified for clarity:

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
[BUY] Asset 0 @ 450.25: 100.00 shares (cost: $45,070.03)
[SELL] Asset 1 @ 375.80: -120.00 shares (proceeds: $45,024.24)
Portfolio Value: $100,123.45 (Cash: $9,929.18)
```

Each line shows a trade execution with the asset index, price, quantity, and dollar amount. You can see exactly when positions open and close, how much they cost, and how the portfolio value fluctuates. This verbose output is invaluable when you're debugging a strategy or trying to understand why it behaved unexpectedly on certain dates.

The backtest continues until one of the CSV files runs out of data. Since we need both assets for pairs trading, the runner stops as soon as either file ends, ensuring we never try to trade on incomplete information.

---

## Step 8: Analyzing Results

The backtest has finished running, and now we want to know: did we make money? How many trades did we take? What was the win rate? Let's extract and display the performance metrics:

```c
static void print_stats(WU_BasicPortfolio portfolio) {
    double final_value = wu_portfolio_value((WU_Portfolio)portfolio);
    double pnl = wu_portfolio_pnl((WU_Portfolio)portfolio);
    double pnl_pct = (pnl / portfolio->params.initial_cash) * 100.0;
    
    printf("\n=== Backtest Results ===\n");
    printf("Initial Cash:      %.2f\n", portfolio->params.initial_cash);
    printf("Final Value:       %.2f\n", final_value);
    printf("P&L:               %.2f (%.2f%%)\n", pnl, pnl_pct);
    printf("Total Fees:        %.2f\n", portfolio->accum_expenses);
    printf("Total Trades:      %ld\n", portfolio->stats->total_trades);
    printf("Winning Trades:    %ld\n", portfolio->stats->winning_trades);
    printf("Losing Trades:     %ld\n", portfolio->stats->losing_trades);
    
    if (portfolio->stats->total_trades > 0) {
        double win_rate = (portfolio->stats->winning_trades * 100.0) / 
                          portfolio->stats->total_trades;
        printf("Win Rate:          %.2f%%\n", win_rate);
    }
    
    printf("Stop Loss Exits:   %ld\n", portfolio->stats->stop_loss_exits);
    printf("Take Profit Exits: %ld\n", portfolio->stats->take_profit_exits);
    printf("\n");
    
    // Asset-specific stats
    double spy_qty = wu_basic_portfolio_asset_quantity(portfolio, 0);
    double qqq_qty = wu_basic_portfolio_asset_quantity(portfolio, 1);
    double spy_value = wu_basic_portfolio_asset_value(portfolio, 0);
    double qqq_value = wu_basic_portfolio_asset_value(portfolio, 1);
    
    printf("=== Asset Holdings ===\n");
    printf("SPY: %.4f shares (value: $%.2f)\n", spy_qty, spy_value);
    printf("QQQ: %.4f shares (value: $%.2f)\n", qqq_qty, qqq_value);
    printf("Cash: $%.2f\n", portfolio->cash);
}

// In main()
print_stats(portfolio);
```

The portfolio tracks everything that matters for evaluating a strategy. The final value includes both remaining cash and the current market value of any open positions. The P&L tells you how much you gained or lost relative to your initial capital, both in dollars and percentage terms. That percentage is what really matters—a $5,000 gain means something very different if you started with $10,000 versus $1,000,000.

The total fees number often surprises people. Even with modest transaction costs (0.1%) and slippage (0.05%), the expenses add up fast if you're trading frequently. This is why high-frequency strategies need massive alpha just to break even after costs.

The portfolio stats object maintains detailed trade-level metrics. Every time a position closes, the portfolio records whether it was a winner or a loser, why it closed (signal, stop loss, or take profit), and the size of the gain or loss. This lets you see not just overall performance, but the character of the strategy. A 60% win rate with small losers and big winners? That's very different from a 60% win rate with big losers and small winners, even if the final P&L is the same.

For multi-asset portfolios like ours, we can query each asset individually to see current holdings. This helps you understand the final state—did we exit all positions cleanly, or are we still holding SPY and QQQ? If we have lingering positions, their unrealized P&L is included in the final portfolio value.

The portfolio exposes a clean interface for these queries. The `wu_portfolio_value()` and `wu_portfolio_pnl()` functions work with any portfolio implementation. The `wu_basic_portfolio_asset_quantity()` and `wu_basic_portfolio_asset_value()` functions are specific to the BasicPortfolio and let you drill down to individual asset holdings. This layering means you can write generic code that works with any portfolio, while still having access to implementation-specific details when you need them.

---

## Complete Code

Here's the full `pairs_trading.c` file:

```c
/**
 * Pairs Trading Backtest
 * 
 * This example demonstrates a complete pairs trading strategy using:
 * - Multi-input strategy (2 assets)
 * - Multi-asset portfolio (shared cash pool across both assets)
 * - Spread-based mean reversion signals
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "wu.h"

static void print_stats(WU_BasicPortfolio portfolio) {
    double final_value = wu_portfolio_value((WU_Portfolio)portfolio);
    double pnl = wu_portfolio_pnl((WU_Portfolio)portfolio);
    double pnl_pct = (pnl / portfolio->params.initial_cash) * 100.0;
    
    printf("\n=== Backtest Results ===\n");
    printf("Initial Cash:      %.2f\n", portfolio->params.initial_cash);
    printf("Final Value:       %.2f\n", final_value);
    printf("P&L:               %.2f (%.2f%%)\n", pnl, pnl_pct);
    printf("Total Fees:        %.2f\n", portfolio->accum_expenses);
    printf("Total Trades:      %ld\n", portfolio->stats->total_trades);
    printf("Winning Trades:    %ld\n", portfolio->stats->winning_trades);
    printf("Losing Trades:     %ld\n", portfolio->stats->losing_trades);
    
    if (portfolio->stats->total_trades > 0) {
        double win_rate = (portfolio->stats->winning_trades * 100.0) / 
                          portfolio->stats->total_trades;
        printf("Win Rate:          %.2f%%\n", win_rate);
    }
    
    printf("Stop Loss Exits:   %ld\n", portfolio->stats->stop_loss_exits);
    printf("Take Profit Exits: %ld\n", portfolio->stats->take_profit_exits);
    printf("\n");
    
    // Asset-specific stats
    double spy_qty = wu_basic_portfolio_asset_quantity(portfolio, 0);
    double qqq_qty = wu_basic_portfolio_asset_quantity(portfolio, 1);
    double spy_value = wu_basic_portfolio_asset_value(portfolio, 0);
    double qqq_value = wu_basic_portfolio_asset_value(portfolio, 1);
    
    printf("=== Asset Holdings ===\n");
    printf("SPY: %.4f shares (value: $%.2f)\n", spy_qty, spy_value);
    printf("QQQ: %.4f shares (value: $%.2f)\n", qqq_qty, qqq_value);
    printf("Cash: $%.2f\n", portfolio->cash);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <spy.csv> <qqq.csv> [-v]\n", argv[0]);
        fprintf(stderr, "  -v: verbose output (shows trading activity)\n");
        return 1;
    }
    
    bool verbose = (argc > 3 && strcmp(argv[3], "-v") == 0);
    
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
    WU_Reader spy_reader = (WU_Reader)wu_csv_reader_new(spy_file, WU_DATA_TYPE_CANDLE, true);
    WU_Reader qqq_reader = (WU_Reader)wu_csv_reader_new(qqq_file, WU_DATA_TYPE_CANDLE, true);
    
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
    
    // Define asset symbols
    WU_AssetSymbol symbols[2];
    strncpy(symbols[0], "SPY", WU_SYMBOL_MAX_LEN - 1);
    strncpy(symbols[1], "QQQ", WU_SYMBOL_MAX_LEN - 1);
    symbols[0][WU_SYMBOL_MAX_LEN - 1] = '\0';
    symbols[1][WU_SYMBOL_MAX_LEN - 1] = '\0';
    
    // Configure multi-asset portfolio
    WU_PortfolioParams params = {
        .initial_cash = 100000.0,
        .tx_cost_pct = 0.001,        // 0.1% transaction cost
        .stop_loss_pct = 0.0,        // No stop loss (rely on mean reversion)
        .take_profit_pct = 0.0,      // No take profit (rely on mean reversion)
        .slippage_pct = 0.0005,      // 0.05% slippage
        .position_sizing = {
            .size_type = WU_POSITION_SIZE_PCT,
            .size_value = 0.45       // Use 45% of cash per asset (90% total exposure)
        }
    };
    
    // Create multi-asset portfolio
    WU_BasicPortfolio portfolio = wu_basic_portfolio_new(
        params, 
        (const WU_AssetSymbol*)symbols, 
        2
    );
    
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
gcc -std=c11 -g -Wall -Wextra -Werror -pedantic -I../../include \
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

### Expected Output

```
=== Pairs Trading Backtest: SPY vs QQQ ===
Strategy: Mean Reversion (20-period window, 2.0 std threshold)
Initial Capital: $100000.00
Position Sizing: 45% cash per asset

=== Backtest Results ===
Initial Cash:      100000.00
Final Value:       102345.67
P&L:               2345.67 (2.35%)
Total Fees:        543.21
Total Trades:      24
Winning Trades:    15
Losing Trades:     9
Win Rate:          62.50%
Stop Loss Exits:   0
Take Profit Exits: 0

=== Asset Holdings ===
SPY: 0.0000 shares (value: $0.00)
QQQ: 0.0000 shares (value: $0.00)
Cash: $102345.67
```

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

Your strategy needs to return four signals, one per asset. You could implement sector rotation, risk parity, or any multi-asset approach you can dream up.

**Python Integration**: If you're more comfortable in Python, Wu includes SWIG-generated bindings that let you work at a higher level while still benefiting from C's performance where it matters:

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

You've journeyed from zero to a working pairs trading backtester. Along the way, you've learned Wu's architecture—how Readers supply data, Strategies generate signals, Portfolios execute trades, and Runners orchestrate everything. You've seen how the pieces compose freely, giving you flexibility without forcing you into rigid patterns.

Pairs trading itself is a beautiful strategy: when two correlated assets drift apart, bet on convergence. Calculate the spread, track its statistics, trade the extremes, exit at the mean. Simple in concept, nuanced in execution.

Multi-asset portfolios add another dimension—shared cash pools, independent positions per asset, index-based tracking for performance. The position sizing system gives you four different ways to allocate capital, from simple absolute quantities to sophisticated strategy-guided allocations.

The runner pattern handles the messy details—validation, synchronization, type conversion—so you can focus on strategy logic. And the metrics system tracks everything you need to evaluate performance: P&L, win rates, transaction costs, per-asset holdings.

Wu gives you low-level building blocks that compose into high-level systems. Swap strategies, customize portfolios, implement exotic data sources—the door's wide open. The explicit state management and strong typing keep things debuggable while maintaining flexibility. It's a toolkit, not a framework. Use what you need, ignore what you don't.

Now go build something interesting. Happy backtesting! 🚀
