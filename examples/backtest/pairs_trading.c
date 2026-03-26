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
    WU_PortfolioStats stats = portfolio->stats;
    
    printf("\n=== Backtest Results ===\n");
    
    // Key-value format
    char* kv = wu_portfolio_stats_to_keyvalue(stats);
    if (kv) {
        printf("%s\n\n", kv);
        free(kv);
    }
    
    // JSON format (pretty)
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
    
    // Create CSV readers for both assets
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
    
    WU_Strategy strategy = (WU_Strategy)wu_pairs_trading_strat_new(
            20,     // window
            2.0,    // threshold
            1.0     // hedge ratio 1:1
        );
    
    WU_PortfolioParams params = {
        .direction = WU_DIRECTION_BOTH,
        .initial_cash = 100000.0,
        .execution_policy = {
            .policy = WU_EXECUTION_POLICY_FIXED_SLIPPAGE,
            .execution_mean = 0.0005,
            .execution_stdev = 0.0,
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
    
    wu_runner_exec(runner, verbose);
    print_stats(portfolio);
    wu_runner_free(runner);
    
    return 0;
}
