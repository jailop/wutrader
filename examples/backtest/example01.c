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

    /*
     * This strategy uses a simple moving average crossover. When the 
     * short-term moving average crosses above the long-term moving
     * average, it generates a buy signal. When the short-term moving
     * average crosses below the long-term moving average, it generates
     * a sell signal. A threshold parameters can be used to add a buffer
     * to the crossover signals to avoid false signals in choppy
     * markets.
     */
    CrossOverStrat strategy = cross_over_strat_new(
            10,  // short window
            30,  // long window
            0.0  // no threshold
        );

    /**
     * The CSV reader can read single time series, trades and candle
     * data.
     */
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
    printf("Total Fees:        %.2f\n", sap->track.accum_expenses);
    printf("Total Trades:      %ld\n", stats->total_trades);
    printf("Winning Trades:    %ld\n", stats->winning_trades);
    printf("Losing Trades:     %ld\n", stats->losing_trades);
    if (stats->total_trades > 0) {
        printf("Win Rate:          %.2f%%\n", 
               (stats->winning_trades * 100.0) / stats->total_trades);
    }
    printf("Stop Loss Exits:   %ld\n", stats->stop_loss_exits);
    printf("Take Profit Exits: %ld\n", stats->take_profit_exits);
    printf("Total Profit:      %.2f\n", stats->total_profit);
    printf("Total Loss:        %.2f\n", stats->total_loss);
    printf("Max Win:           %.2f\n", stats->max_win);
    printf("Max Loss:          %.2f\n", stats->max_loss);
    if (stats->winning_trades > 0) {
        printf("Avg Win:           $%.2f\n", 
               stats->total_profit / stats->winning_trades);
    }
    if (stats->losing_trades > 0) {
        printf("Avg Loss:          $%.2f\n", 
               stats->total_loss / stats->losing_trades);
    }
    
cleanup:
    if (runner) basic_runner_free(runner);
    if (file) fclose(file);
    return ret;
}
