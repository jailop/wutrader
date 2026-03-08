#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wu.h"

int main(int argc, char** argv) {
    int ret = 0;
    FILE* file = NULL;
    SingleAssetPortfolio portfolio = NULL;
    CrossOverStrat strategy = NULL;
    CsvReader reader = NULL;
    BasicRunner runner = NULL;
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <csv_file> [-v]\n", argv[0]);
        return 1;
    }
    
    const char* filename = argv[1];
    file = fopen(filename, "r");
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
    
    portfolio = single_asset_portfolio_new(params);
    strategy = cross_over_strat_new(10, 30, 0.0);
    reader = csv_reader_new(file, DATA_TYPE_CANDLE, true);
    
    if (!portfolio || !strategy || !reader) {
        fprintf(stderr, "Error: Failed to initialize components\n");
        ret = 1;
        goto cleanup;
    }
    
    runner = basic_runner_new(
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
    if (portfolio) single_asset_portfolio_free(portfolio);
    if (strategy) cross_over_strat_free(strategy);
    if (reader) csv_reader_free(reader);
    if (file) fclose(file);
    return ret;
}
