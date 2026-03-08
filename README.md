# Wu - Low Level Backtesting Library for Trading

This is a personal project, a backtesting library for trading strategies
written in C. I use it to for learning, experimenting with different
designs, and exploring ideas in algorithmic trading. 

Build the library:

```bash
make
```

Run the example backtest:

```bash
./examples/backtest/example01 ./tests/data/btcusd.csv -v
```

Here is an example (the safeguards are ommitted for brevity):

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wu.h"

int main(int argc, char** argv) {
    int ret = 0;
    const char* filename = argv[1];
    FILE* file = fopen(filename, "r");
    SingleAssetPortfolioParams params = {
        .initial_cash = 100000.0,
        .tx_cost_pct = 0.001,
        .stop_loss_pct = 0.10,
        .take_profit_pct = 0.20,
        .slippage_pct = 0.0005,
        .position_sizing = {
            .size_type = POSITION_SIZE_PCT,
            .size_value = 1.0
        }
    };
    SingleAssetPortfolio portfolio = single_asset_portfolio_new(params);
    CrossOverStrat strategy = cross_over_strat_new(
            10,  // short window
            30,  // long window
            0.0  // no threshold
        );
    CsvReader reader = csv_reader_new(
            file,               // file pointer
            DATA_TYPE_CANDLE,   // data type
            true                // has header
        );
    BasicRunner runner = basic_runner_new(
        (Portfolio)portfolio,
        (Strategy)strategy,
        (Reader)reader
    );
    runner_run(runner, argc > 2 && strcmp(argv[2], "-v") == 0);
    SingleAssetPortfolio sap = (SingleAssetPortfolio)portfolio;
    PortfolioStats stats = sap->track.stats;
    printf("Initial Cash:      %.2f\n", params.initial_cash);
    printf("Final Value:       %.2f\n", portfolio_value(portfolio));
    printf("P&L:               %.2f (%.2f%%)\n", 
           portfolio_pnl(portfolio),
           (portfolio_pnl(portfolio) / params.initial_cash) * 100.0);
    if (runner) basic_runner_free(runner);
    if (portfolio) single_asset_portfolio_free(portfolio);
    if (strategy) cross_over_strat_free(strategy);
    if (reader) csv_reader_free(reader);
    if (file) fclose(file);
    return ret;
}

```

This example demonstrates a complete backtesting workflow: reading historical candle data, computing moving average indicators, generating crossover signals, executing trades with realistic costs, and reporting comprehensive statistics.

The library uses C's struct-and-function-pointer pattern to achieve polymorphism without virtual tables or runtime overhead. Each major abstraction (`Portfolio`, `Strategy`, `Reader`, `Indicator`) is defined as a struct containing function pointers, allowing different implementations while maintaining a consistent interface.

Wu is not a framework—it's a toolkit. The core abstractions (`Portfolio`, `Strategy`, `Reader`, `Indicator`) are designed to be composed freely. You can swap implementations, combine strategies, or bypass components entirely without fighting against framework constraints.

The `BasicRunner` demonstrates this composability. It's just one way to wire together a portfolio, strategy, and data reader. I'm free to write other runners with custom logic, logging, or execution patterns.

This design means I could implement custom portfolio types—multi-asset portfolios, portfolios with margin trading, portfolios with exotic risk models—without modifying the library. The type system stays while providing the structure needed for interoperability.

Trading data comes in many forms. Wu supports three fundamental data types:

- **Candles** (OHLCV): Standard bar data for time-series analysis
- **Trades**: Tick-level trade data with price, volume, and side
- **Single Values**: Generic time-series data for arbitrary indicators or derived signals

The `Reader` abstraction allows you to plug in any data source: CSV files, databases, real-time feeds, or synthetic generators. The library doesn't care where data comes from—it just needs something that implements the `next()` method.

Every stateful component in the maintains its state explicitly in its struct. When you create a `MovingAverage`, its circular buffer, position counter, and sum are right there in the struct. This makes debugging straightforward and allows me to snapshot or serialize state at any point.

Using C is a personal preference. In my life, no other language has provided me more joy
when writing code. With C, I have the feeling of being free to explore,
experiment with different designs, and understand every detail of how my
code executes, everythin at the same time. I can write code that is both high-level in its
abstractions and low-level in its performance characteristics.

That doesn't mean that I'm a C purist. On the contrary. I know how to
gain insight and productivity by using higher-level languages when
appropriate.  That is the reason this library include bindings for
Python. Besides that, I maintain a more user oriented implementation of
a similar library in C++
([tzutrader](https://jailop.codeberg.page/tzutrader/docs/)). Often, I
play exploring ideas between this implementations, moving back and forth
between them. Their core ideas are the same.

## Python Bindings

The library includes Python bindings that expose the same core
abstractions and functionality. The Python API is designed to be as
close as possible to the C API, while providing a more Pythonic
interface. The bindings are implemented using SWIG, which allows for
automatic memory management and seamless integration with Python's data
structures.

Here is a similar example in Python using the bindings:

```python
import wu

def main():
    filename = sys.argv[1]
    verbose = len(sys.argv) > 2 and sys.argv[2] == "-v"
    initial_cash = 100000.0
    portfolio = wu.create_single_asset_portfolio(
        initial_cash=initial_cash,
        tx_cost_pct=0.001,
        stop_loss_pct=0.10,
        take_profit_pct=0.20,
        slippage_pct=0.0005,
        size_type=wu.POSITION_SIZE_PCT,
        size_value=1.0
    )
    strategy = wu.cross_over_strat_new(10, 30, 0.0)
    reader = wu.csv_reader_open(filename, wu.DATA_TYPE_CANDLE, True)
    runner = wu.basic_runner_new(portfolio, strategy, reader)
    wu.runner_call_run(runner, verbose)
    pnl = wu.portfolio_call_pnl(portfolio)
    pnl_pct = (pnl / initial_cash) * 100.0
    print(f"Initial Cash:      {initial_cash:.2f}")
    print(f"Final Value:       {wu.portfolio_call_value(portfolio):.2f}")
    print(f"P&L:               {pnl:.2f} ({pnl_pct:.2f}%)")

if __name__ == "__main__":
    main()
```

You see, I love C, but I also love Python and many other languages.
Moreover, what I love the most is solving problems!


