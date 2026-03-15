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
