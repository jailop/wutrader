# Wu - Low Level Backtesting Library for Trading

This is a personal project – a backtesting library for trading strategies written in C. I use it for learning, experimenting with different designs, and exploring ideas in algorithmic trading.

## Requirements

- C11 compatible compiler
- Make
- SWIG (for Python bindings)
- Doxygen (optional, for API documentation)

## Building

Build the library:

```bash
make
```

Run the tests:

```bash
make run_tests
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

## Example

Run a C example:

```bash
./examples/backtest/example01 ./tests/data/btcusd.csv -v
```

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wu.h"

int main(int argc, char** argv) {
    int ret = 0;
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <csv_file> [-v]\n", argv[0]);
        return 1;
    }
    const char* filename = argv[1];
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file %s\n", filename);
        return 1;
    }
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
    if (!portfolio || !strategy || !reader) {
        fprintf(stderr, "Error: Failed to initialize components\n");
        ret = 1;
        goto cleanup;
    }
    BasicRunner runner = basic_runner_new(
        (Portfolio)portfolio,
        (Strategy)strategy,
        (Reader)reader
    );
    if (!runner) {
        fprintf(stderr, "Error: Failed to create runner\n");
        ret = 1;
        goto cleanup;
    }
    runner_run(runner, argc > 2 && strcmp(argv[2], "-v") == 0);
    SingleAssetPortfolio sap = (SingleAssetPortfolio)portfolio;
    PortfolioStats stats = sap->track.stats;
    printf("Initial Cash:      %.2f\n", params.initial_cash);
    printf("Final Value:       %.2f\n", portfolio_value(portfolio));
    printf("P&L:               %.2f (%.2f%%)\n", 
           portfolio_pnl(portfolio),
           (portfolio_pnl(portfolio) / params.initial_cash) * 100.0);
cleanup:
    if (runner) basic_runner_free(runner);
    if (file) fclose(file);
    return ret;
}
```

This example demonstrates a complete backtesting workflow: reading historical candle data, calculating moving average indicators, generating crossover signals, simulating trade executions, and generating statistical reports.

## Design Approach

The library utilizes C's struct-and-function-pointer pattern to achieve polymorphism without virtual tables or runtime overhead. Each major abstraction – `Portfolio`, `Strategy`, `Reader`, and `Indicator` – is defined as a struct containing function pointers, allowing different implementations while maintaining a consistent interface.

Wu is not a framework; it's a toolkit. The core abstractions are designed to be composed freely. I can swap implementations, combine strategies, or bypass components entirely without fighting against framework constraints.

The `BasicRunner` demonstrates this composability. It’s just one way to wire together a portfolio, strategy, and data reader. I’m free to write other runners with custom logic, logging, or execution patterns.

This design allows me to implement custom portfolio types—such as multi-asset portfolios, portfolios with margin trading, or portfolios with exotic risk models—without modifying the library. The type system remains, while still providing the necessary structure for interoperability.

Trading data comes in many forms. Wu supports three fundamental data types:

- **Candles** (OHLCV): Standard bar data for time-series analysis.
- **Trades**: Tick-level trade data, including price, volume, and side.
- **Single Values**: Generic time-series data for arbitrary indicators or derived signals.

The `Reader` abstraction allows me to plug in any data source—CSV files, databases, real-time feeds, or synthetic generators. The library doesn’t care where the data comes from; it just needs something that implements the `next()` method.

Every stateful component maintains its state explicitly within its struct. When I create a `MovingAverage`, its circular buffer, position counter, and sum are all defined within the struct. This makes debugging straightforward and allows me to snapshot or serialize state at any point.

Using C is a matter of personal preference. In my experience, no other language has provided me with as much joy when writing code. With C, I have the feeling of being free to explore, experiment with different designs, and understand the details of how my code executes. I can write code that is both high-level in its abstractions and low-level in its performance characteristics.

That doesn’t mean that I’m a C purist. On the contrary, I know how to gain insight and productivity by using higher-level languages when appropriate. That is the reason this library includes bindings for Python. Besides that, I maintain a more user-oriented implementation of a similar library in C++ ([tzutrader](https://jailop.codeberg.page/tzutrader/docs/)). Often, I enjoy exploring ideas between these implementations, moving back and forth between them. Their core ideas are the same.

## Contributing

This is a personal project, so I’m not hopeful for contributions. However, if you have questions, suggestions, or want to share ideas, feel free to reach out to me. If you feel interested in learning more about this project, I’ll be happy to share more details about the design and implementation. Furthermore, if you want to contribute – by reviewing the code, opening issues, or even submitting pull requests – I’ll be very grateful.

My email is: >> jailop \AT/ protonmail \DOT/ com <<
