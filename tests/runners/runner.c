#include <CUnit/CUnit.h>
#include <string.h>
#include <stdio.h>
#include "wu.h"

void test_runner_single_input_creation(void) {
    // Create a simple portfolio
    WU_PortfolioParams params = {
        .initial_cash = 100000.0,
        .tx_cost_pct = 0.001,
        .stop_loss_pct = 0.0,
        .take_profit_pct = 0.0,
        .slippage_pct = 0.0,
        .position_sizing = {
            .size_type = WU_POSITION_SIZE_PCT,
            .size_value = 1.0
        }
    };
    const char* symbols[] = {"BTC", NULL};
    WU_BasicPortfolio portfolio = wu_basic_portfolio_new(params, (const char**)symbols);
    
    // Create a crossover strategy
    WU_Strategy strategy = (WU_Strategy)wu_crossover_strat_new(5, 10, 0.01);
    
    // Create a reader (using test data)
    FILE* file = fopen("tests/data/btcusd_price.csv", "r");
    CU_ASSERT_PTR_NOT_NULL(file);
    WU_Reader reader = (WU_Reader)wu_csv_reader_new(file, WU_DATA_TYPE_SINGLE_VALUE, WU_TIME_UNIT_SECONDS, false);
    
    // Create runner with single reader
    WU_Reader readers[] = {reader};
    WU_Runner runner = wu_runner_new(
        (WU_Portfolio)portfolio,
        strategy,
        readers
    );
    
    CU_ASSERT_PTR_NOT_NULL(runner);
    CU_ASSERT_EQUAL(runner->num_readers, 1);
    CU_ASSERT_PTR_NOT_NULL(runner->readers);
    CU_ASSERT_PTR_NOT_NULL(runner->portfolio);
    CU_ASSERT_PTR_NOT_NULL(runner->strategy);
    CU_ASSERT_PTR_NOT_NULL(runner->run);
    
    wu_runner_free(runner);
    fclose(file);
}

void test_runner_multi_input_creation(void) {
    // Create multi-asset portfolio
    WU_PortfolioParams params = {
        .initial_cash = 100000.0,
        .tx_cost_pct = 0.001,
        .stop_loss_pct = 0.0,
        .take_profit_pct = 0.0,
        .slippage_pct = 0.0,
        .position_sizing = {
            .size_type = WU_POSITION_SIZE_PCT,
            .size_value = 0.5
        }
    };
    
    const char* symbols[] = {"SPY", "QQQ", NULL};    
    WU_BasicPortfolio portfolio = wu_basic_portfolio_new(params, (const char**)symbols);
    
    // Create pairs trading strategy
    WU_Strategy strategy = (WU_Strategy)wu_pairs_trading_strat_new(20, 2.0, 1.0);
    
    // Create two readers
    FILE* file1 = fopen("tests/data/spy.csv", "r");
    FILE* file2 = fopen("tests/data/qqq.csv", "r");
    CU_ASSERT_PTR_NOT_NULL(file1);
    CU_ASSERT_PTR_NOT_NULL(file2);
    
    WU_Reader reader1 = (WU_Reader)wu_csv_reader_new(file1, WU_DATA_TYPE_CANDLE, WU_TIME_UNIT_SECONDS, true);
    WU_Reader reader2 = (WU_Reader)wu_csv_reader_new(file2, WU_DATA_TYPE_CANDLE, WU_TIME_UNIT_SECONDS, true);
    
    // Create runner with two readers
    WU_Reader readers[] = {reader1, reader2};
    WU_Runner runner = wu_runner_new(
        (WU_Portfolio)portfolio,
        strategy,
        readers
    );
    
    CU_ASSERT_PTR_NOT_NULL(runner);
    CU_ASSERT_EQUAL(runner->num_readers, 2);
    CU_ASSERT_PTR_NOT_NULL(runner->readers);
    
    wu_runner_free(runner);
    fclose(file1);
    fclose(file2);
}

void test_runner_rejects_mismatched_reader_count(void) {
    WU_PortfolioParams params = {
        .initial_cash = 100000.0,
        .tx_cost_pct = 0.001,
        .stop_loss_pct = 0.0,
        .take_profit_pct = 0.0,
        .slippage_pct = 0.0,
        .position_sizing = {
            .size_type = WU_POSITION_SIZE_PCT,
            .size_value = 0.5
        }
    };
    
    const char* symbols[] = {"SPY", "QQQ", NULL};    
    WU_BasicPortfolio portfolio = wu_basic_portfolio_new(params, (const char**)symbols);
    
    // Strategy expects 2 inputs
    WU_Strategy strategy = (WU_Strategy)wu_pairs_trading_strat_new(20, 2.0, 1.0);
    CU_ASSERT_EQUAL(strategy->num_inputs, 2);
    
    // Provide 2 readers but one is NULL - should fail validation
    FILE* file = fopen("tests/data/spy.csv", "r");
    CU_ASSERT_PTR_NOT_NULL(file);
    WU_Reader reader = (WU_Reader)wu_csv_reader_new(file, WU_DATA_TYPE_CANDLE, WU_TIME_UNIT_SECONDS, true);
    
    WU_Reader readers[] = {reader, NULL};  // Second reader is NULL
    WU_Runner runner = wu_runner_new(
        (WU_Portfolio)portfolio,
        strategy,
        readers
    );
    
    // Should fail validation and return NULL
    CU_ASSERT_PTR_NULL(runner);
    
    // Cleanup
    wu_reader_delete(reader);
    wu_strategy_delete(strategy);
    wu_portfolio_delete((WU_Portfolio)portfolio);
    fclose(file);
}

void test_runner_rejects_null_arguments(void) {
    WU_PortfolioParams params = {
        .initial_cash = 100000.0,
        .tx_cost_pct = 0.001,
        .stop_loss_pct = 0.0,
        .take_profit_pct = 0.0,
        .slippage_pct = 0.0,
        .position_sizing = {
            .size_type = WU_POSITION_SIZE_PCT,
            .size_value = 1.0
        }
    };
    const char* symbols[] = {"BTC", NULL};
    WU_BasicPortfolio portfolio = wu_basic_portfolio_new(params, (const char**)symbols);
    WU_Strategy strategy = (WU_Strategy)wu_crossover_strat_new(5, 10, 0.01);
    
    FILE* file = fopen("tests/data/btcusd_price.csv", "r");
    WU_Reader reader = (WU_Reader)wu_csv_reader_new(file, WU_DATA_TYPE_SINGLE_VALUE, WU_TIME_UNIT_SECONDS, false);
    WU_Reader readers[] = {reader};
    
    // Test NULL portfolio
    WU_Runner runner1 = wu_runner_new(NULL, strategy, readers);
    CU_ASSERT_PTR_NULL(runner1);
    
    // Test NULL strategy
    WU_Runner runner2 = wu_runner_new((WU_Portfolio)portfolio, NULL, readers);
    CU_ASSERT_PTR_NULL(runner2);
    
    // Test NULL readers array
    WU_Runner runner3 = wu_runner_new((WU_Portfolio)portfolio, strategy, NULL);
    CU_ASSERT_PTR_NULL(runner3);
    
    // Cleanup
    wu_reader_delete(reader);
    wu_strategy_delete(strategy);
    wu_portfolio_delete((WU_Portfolio)portfolio);
    fclose(file);
}

void test_runner_rejects_invalid_reader_count(void) {
    // This test validates that providing NULL readers within the array causes rejection
    WU_PortfolioParams params = {
        .initial_cash = 100000.0,
        .tx_cost_pct = 0.001,
        .stop_loss_pct = 0.0,
        .take_profit_pct = 0.0,
        .slippage_pct = 0.0,
        .position_sizing = {
            .size_type = WU_POSITION_SIZE_PCT,
            .size_value = 1.0
        }
    };
    const char* symbols[] = {"BTC", NULL};
    WU_BasicPortfolio portfolio = wu_basic_portfolio_new(params, (const char**)symbols);
    WU_Strategy strategy = (WU_Strategy)wu_crossover_strat_new(5, 10, 0.01);
    
    FILE* file = fopen("tests/data/btcusd_price.csv", "r");
    WU_Reader reader = (WU_Reader)wu_csv_reader_new(file, WU_DATA_TYPE_SINGLE_VALUE, WU_TIME_UNIT_SECONDS, false);
    
    // Test with NULL reader in array
    WU_Reader readers[] = {NULL};
    WU_Runner runner1 = wu_runner_new((WU_Portfolio)portfolio, strategy, readers);
    CU_ASSERT_PTR_NULL(runner1);
    
    // Cleanup
    wu_reader_delete(reader);
    wu_strategy_delete(strategy);
    wu_portfolio_delete((WU_Portfolio)portfolio);
    fclose(file);
}

void test_runner_convenience_function(void) {
    WU_PortfolioParams params = {
        .initial_cash = 100000.0,
        .tx_cost_pct = 0.001,
        .stop_loss_pct = 0.0,
        .take_profit_pct = 0.0,
        .slippage_pct = 0.0,
        .position_sizing = {
            .size_type = WU_POSITION_SIZE_PCT,
            .size_value = 1.0
        }
    };
    const char* symbols[] = {"BTC", NULL};
    WU_BasicPortfolio portfolio = wu_basic_portfolio_new(params, (const char**)symbols);
    WU_Strategy strategy = (WU_Strategy)wu_crossover_strat_new(5, 10, 0.01);
    
    FILE* file = fopen("tests/data/btcusd_price.csv", "r");
    WU_Reader reader = (WU_Reader)wu_csv_reader_new(file, WU_DATA_TYPE_SINGLE_VALUE, WU_TIME_UNIT_SECONDS, false);
    
    // Test convenience function for single reader
    WU_Runner runner = wu_runner_new_single(
        (WU_Portfolio)portfolio,
        strategy,
        reader
    );
    
    CU_ASSERT_PTR_NOT_NULL(runner);
    CU_ASSERT_EQUAL(runner->num_readers, 1);
    
    wu_runner_free(runner);
    fclose(file);
}
