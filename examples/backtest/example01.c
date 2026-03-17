/**
 * Example on using the wu library to backtest a simple crossover
 * strategy on a single asset portfolio. The strategy buys when the
 * short-term moving average crosses above the long-term moving average,
 * and sells when the short-term moving average crosses below the
 * long-term moving average. The example reads historical price data
 * from a CSV file that it is expected to have the following columns:
 * timestamp, open, high, low, close, volume. The example also prints
 * out various performance metrics at the end of the backtest.
 *
 * Usage:
 *
 *   ./example01 path/to/data.csv [-v]
 *
 * The optional -v flag enables verbose output during the backtest,
 * which will print out details of each trade and portfolio value
 * changes.
 */

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
   
    /* 
     * This portfolio works with a single asset. It has fixed parameters
     * for initial cash, transaction costs, stop loss and take profit
     * levels, slippage, and position sizing.
     */
    WU_PortfolioParams params = {
        .direction = WU_DIRECTION_LONG,
        .initial_cash = 100000.0,
        .execution_policy = {
            .policy = WU_EXECUTION_POLICY_FIXED_SLIPPAGE,
            .execution_mean = 0.0005,
            .execution_stddev = 0.0,
            .tx_cost_type = WU_TRANSACTION_COST_PROPORTIONAL,
            .tx_cost_value = 0.001,
            .stop_loss_pct = 0.10,
            .take_profit_pct = 0.20
        },
        .borrow_params = {
            .rate = 0.0,
            .limit = 0.0
        },
        .position_sizing = {
            .size_type = WU_POSITION_SIZE_PCT,
            .size_value = 1.0
        }
    };
    WU_BasicPortfolio portfolio = wu_basic_portfolio_new(
            params,
            wu_symbol_list("ASSET"));

    /*
     * This strategy uses a simple moving average crossover. When the 
     * short-term moving average crosses above the long-term moving
     * average, it generates a buy signal. When the short-term moving
     * average crosses below the long-term moving average, it generates
     * a sell signal. A threshold parameters can be used to add a buffer
     * to the crossover signals to avoid false signals in choppy
     * markets.
     */
    WU_CrossOverStrat strategy = wu_crossover_strat_new(
            10,  // short window
            30,  // long window
            0.0  // no threshold
        );

    /**
     * The CSV reader can read single time series, trades and candle
     * data.
     */
    WU_CsvReader reader = wu_csv_reader_new(
            file,                   // file pointer
            WU_DATA_TYPE_CANDLE,
            WU_TIME_UNIT_SECONDS,
            true                    // has header
        );
    
    if (!portfolio || !strategy || !reader) {
        fprintf(stderr, "Error: Failed to initialize components\n");
        ret = 1;
        goto cleanup;
    }
   
    /*
     * The runner will take the portfolio, strategy, and reader, and
     * execute the backtest by feeding the data from the reader into the
     * strategy, which will generate buy/sell signals that the portfolio
     * will execute. The runner will also handle the timing and
     * sequencing of the backtest, and can optionally print verbose
     * output if the -v flag is provided.
     *
     * The runner works with generic types for the portfolio, strategy,
     * and reader, so it can be easily swapped out with different
     * implementations without changing the overall structure of the
     * backtest.
     */
    WU_Runner runner = wu_runner_new(
        WU_PORTFOLIO(portfolio),
        WU_STRATEGY(strategy),
        wu_reader_list(WU_READER(reader))
    );
    
    if (!runner) {
        fprintf(stderr, "Error: Failed to create runner\n");
        ret = 1;
        goto cleanup;
    }
    
    wu_runner_exec(runner, argc > 2 && strcmp(argv[2], "-v") == 0);
    
    // Print stats in key-value and JSON formats
    WU_PortfolioStats stats = portfolio->stats;
    char* kv = wu_portfolio_stats_to_keyvalue(stats);
    if (kv) {
        printf("\n%s\n", kv);
        free(kv);
    }
    
    char* json = wu_portfolio_stats_to_json(stats, true);
    if (json) {
        printf("\n%s\n", json);
        free(json);
    }
    
cleanup:
    if (runner) wu_runner_free(runner);
    if (file) fclose(file);
    return ret;
}
