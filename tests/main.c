#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>

extern void test_sma_returns_nan_during_warmup(void);
extern void test_sma_calculates_correct_sma(void);
extern void test_sma_sliding_window_updates_correctly(void);
extern void test_ema_returns_nan_during_warmup(void);
extern void test_ema_first_value_is_sma_of_warmup_period(void);
extern void test_ema_applies_exponential_smoothing_after_warmup(void);
extern void test_csv_reader_reads_candles(void);
extern void test_csv_reader_reads_trades(void);
extern void test_csv_reader_reads_single_values(void);
extern void test_csv_reader_returns_null_at_eof(void);
extern void test_csv_reader_handles_headers(void);
extern void test_json_reader_reads_candles(void);
extern void test_json_reader_reads_trades(void);
extern void test_json_reader_reads_single_values(void);
extern void test_json_reader_returns_null_at_eof(void);
extern void test_json_reader_handles_invalid_json(void);
extern void test_json_reader_handles_missing_fields(void);
extern void test_crossover_initialization(void);
extern void test_crossover_holds_during_warmup(void);
extern void test_crossover_generates_buy_signal(void);
extern void test_crossover_generates_sell_signal(void);
extern void test_crossover_no_repeat_signals(void);
extern void test_crossover_with_real_data(void);
extern void test_crossover_threshold_prevents_noise(void);
extern void test_mvar_returns_nan_during_warmup(void);
extern void test_mvar_calculates_population_variance(void);
extern void test_mvar_calculates_sample_variance(void);
extern void test_mvar_sliding_window_updates_correctly(void);
extern void test_mstdev_returns_nan_during_warmup(void);
extern void test_mstdev_calculates_population_mstdev(void);
extern void test_mstdev_calculates_sample_mstdev(void);
extern void test_mstdev_sliding_window_updates_correctly(void);
extern void test_rsi_returns_nan_during_warmup(void);
extern void test_rsi_calculates_correct_value(void);
extern void test_rsi_handles_all_gains(void);
extern void test_rsi_handles_all_losses(void);
extern void test_rsi_range_is_valid(void);
extern void test_macd_returns_nan_during_warmup(void);
extern void test_macd_signal_warmup(void);
extern void test_macd_calculates_correct_values(void);
extern void test_macd_histogram_consistency(void);
extern void test_macd_uptrend_produces_positive_macd(void);
extern void test_macd_downtrend_produces_negative_macd(void);
extern void test_pairs_trading_initialization(void);
extern void test_pairs_trading_holds_during_warmup(void);
extern void test_pairs_trading_detects_low_spread(void);
extern void test_pairs_trading_detects_high_spread(void);
extern void test_pairs_trading_signal_vector_structure(void);
extern void test_pairs_trading_with_custom_ratio(void);
extern void test_pairs_trading_input_validation(void);
extern void test_basic_portfolio_initialization(void);
extern void test_basic_portfolio_single_buy_signal(void);
extern void test_basic_portfolio_multiple_buy_signals(void);
extern void test_basic_portfolio_sell_before_buy(void);
extern void test_basic_portfolio_asset_value(void);
extern void test_basic_portfolio_total_value(void);
extern void test_basic_portfolio_equal_distribution_sizing(void);
extern void test_basic_portfolio_strategy_guided_sizing(void);
extern void test_basic_portfolio_invalid_asset_index(void);
extern void test_runner_single_input_creation(void);
extern void test_runner_multi_input_creation(void);
extern void test_runner_rejects_mismatched_reader_count(void);
extern void test_runner_rejects_null_arguments(void);
extern void test_runner_rejects_invalid_reader_count(void);
extern void test_runner_convenience_function(void);

/* Global statitcs */
extern void test_mean(void);
extern void test_var_dof1(void);
extern void test_var_dof0(void);
extern void test_stdev_dof1(void);

/* Stats tests */
extern void test_mean_basic(void);
extern void test_downside_basic(void);
extern void test_pnlstats_basic(void);
extern void test_sharpe_basic(void);

int main(void) {
    CU_pSuite sma_suite = NULL;
    CU_pSuite ema_suite = NULL;
    CU_pSuite csv_suite = NULL;
    CU_pSuite json_suite = NULL;
    CU_pSuite crossover_suite = NULL;
    CU_pSuite mvar_suite = NULL;
    CU_pSuite mstdev_suite = NULL;
    CU_pSuite rsi_suite = NULL;
    CU_pSuite macd_suite = NULL;
    CU_pSuite pairs_trading_suite = NULL;
    CU_pSuite basic_portfolio_suite = NULL;
    CU_pSuite runner_suite = NULL;

    if (CU_initialize_registry() != CUE_SUCCESS) {
        return CU_get_error();
    }

    sma_suite = CU_add_suite("SMA_Suite", NULL, NULL);
    if (sma_suite == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (CU_add_test(sma_suite, "test_returns_nan_during_warmup",
                    test_sma_returns_nan_during_warmup) == NULL ||
        CU_add_test(sma_suite, "test_calculates_correct_moving_average",
                    test_sma_calculates_correct_sma) == NULL ||
        CU_add_test(sma_suite, "test_sliding_window_updates_correctly",
                    test_sma_sliding_window_updates_correctly) == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    ema_suite = CU_add_suite("EMA_Suite", NULL, NULL);
    if (ema_suite == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (CU_add_test(ema_suite, "test_returns_nan_during_warmup",
                    test_ema_returns_nan_during_warmup) == NULL ||
        CU_add_test(ema_suite, "test_first_value_is_sma_of_warmup_period",
                    test_ema_first_value_is_sma_of_warmup_period) == NULL ||
        CU_add_test(ema_suite, "test_applies_exponential_smoothing_after_warmup",
                    test_ema_applies_exponential_smoothing_after_warmup) == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }


    csv_suite = CU_add_suite("CSV_Suite", NULL, NULL);
    if (csv_suite == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (CU_add_test(csv_suite, "test_reads_candles",
                    test_csv_reader_reads_candles) == NULL ||
        CU_add_test(csv_suite, "test_reads_trades",
                    test_csv_reader_reads_trades) == NULL ||
        CU_add_test(csv_suite, "test_reads_single_values",
                    test_csv_reader_reads_single_values) == NULL ||
        CU_add_test(csv_suite, "test_returns_null_at_eof",
                    test_csv_reader_returns_null_at_eof) == NULL ||
        CU_add_test(csv_suite, "test_handles_headers",
                    test_csv_reader_handles_headers) == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }


    json_suite = CU_add_suite("JSON_Suite", NULL, NULL);
    if (json_suite == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (CU_add_test(json_suite, "test_reads_candles",
                    test_json_reader_reads_candles) == NULL ||
        CU_add_test(json_suite, "test_reads_trades",
                    test_json_reader_reads_trades) == NULL ||
        CU_add_test(json_suite, "test_reads_single_values",
                    test_json_reader_reads_single_values) == NULL ||
        CU_add_test(json_suite, "test_returns_null_at_eof",
                    test_json_reader_returns_null_at_eof) == NULL ||
        CU_add_test(json_suite, "test_handles_invalid_json",
                    test_json_reader_handles_invalid_json) == NULL ||
        CU_add_test(json_suite, "test_handles_missing_fields",
                    test_json_reader_handles_missing_fields) == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }


    crossover_suite = CU_add_suite("CrossOver_Suite", NULL, NULL);
    if (crossover_suite == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (CU_add_test(crossover_suite, "test_initialization",
                    test_crossover_initialization) == NULL ||
        CU_add_test(crossover_suite, "test_holds_during_warmup",
                    test_crossover_holds_during_warmup) == NULL ||
        CU_add_test(crossover_suite, "test_generates_buy_signal",
                    test_crossover_generates_buy_signal) == NULL ||
        CU_add_test(crossover_suite, "test_generates_sell_signal",
                    test_crossover_generates_sell_signal) == NULL ||
        CU_add_test(crossover_suite, "test_no_repeat_signals",
                    test_crossover_no_repeat_signals) == NULL ||
        // CU_add_test(crossover_suite, "test_with_real_data",  // TEMPORARILY DISABLED
        //             test_crossover_with_real_data) == NULL ||
        CU_add_test(crossover_suite, "test_threshold_prevents_noise",
                    test_crossover_threshold_prevents_noise) == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    mvar_suite = CU_add_suite("MVar_Suite", NULL, NULL);
    if (mvar_suite == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (CU_add_test(mvar_suite, "test_returns_nan_during_warmup",
                    test_mvar_returns_nan_during_warmup) == NULL ||
        CU_add_test(mvar_suite, "test_calculates_population_variance",
                    test_mvar_calculates_population_variance) == NULL ||
        CU_add_test(mvar_suite, "test_calculates_sample_variance",
                    test_mvar_calculates_sample_variance) == NULL ||
        CU_add_test(mvar_suite, "test_sliding_window_updates_correctly",
                    test_mvar_sliding_window_updates_correctly) == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    mstdev_suite = CU_add_suite("Stdev_Suite", NULL, NULL);
    if (mstdev_suite == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (CU_add_test(mstdev_suite, "test_returns_nan_during_warmup",
                    test_mstdev_returns_nan_during_warmup) == NULL ||
        CU_add_test(mstdev_suite, "test_calculates_population_mstdev",
                    test_mstdev_calculates_population_mstdev) == NULL ||
        CU_add_test(mstdev_suite, "test_calculates_sample_mstdev",
                    test_mstdev_calculates_sample_mstdev) == NULL ||
        CU_add_test(mstdev_suite, "test_sliding_window_updates_correctly",
                    test_mstdev_sliding_window_updates_correctly) == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    rsi_suite = CU_add_suite("RSI_Suite", NULL, NULL);
    if (rsi_suite == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (CU_add_test(rsi_suite, "test_returns_nan_during_warmup",
                    test_rsi_returns_nan_during_warmup) == NULL ||
        CU_add_test(rsi_suite, "test_calculates_correct_value",
                    test_rsi_calculates_correct_value) == NULL ||
        CU_add_test(rsi_suite, "test_handles_all_gains",
                    test_rsi_handles_all_gains) == NULL ||
        CU_add_test(rsi_suite, "test_handles_all_losses",
                    test_rsi_handles_all_losses) == NULL ||
        CU_add_test(rsi_suite, "test_range_is_valid",
                    test_rsi_range_is_valid) == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    macd_suite = CU_add_suite("MACD_Suite", NULL, NULL);
    if (macd_suite == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (CU_add_test(macd_suite, "test_returns_nan_during_warmup",
                    test_macd_returns_nan_during_warmup) == NULL ||
        CU_add_test(macd_suite, "test_signal_warmup",
                    test_macd_signal_warmup) == NULL ||
        CU_add_test(macd_suite, "test_calculates_correct_values",
                    test_macd_calculates_correct_values) == NULL ||
        CU_add_test(macd_suite, "test_histogram_consistency",
                    test_macd_histogram_consistency) == NULL ||
        CU_add_test(macd_suite, "test_uptrend_produces_positive_macd",
                    test_macd_uptrend_produces_positive_macd) == NULL ||
        CU_add_test(macd_suite, "test_downtrend_produces_negative_macd",
                    test_macd_downtrend_produces_negative_macd) == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    pairs_trading_suite = CU_add_suite("PairsTrading_Suite", NULL, NULL);
    if (pairs_trading_suite == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (CU_add_test(pairs_trading_suite, "test_initialization",
                    test_pairs_trading_initialization) == NULL ||
        CU_add_test(pairs_trading_suite, "test_holds_during_warmup",
                    test_pairs_trading_holds_during_warmup) == NULL ||
        CU_add_test(pairs_trading_suite, "test_detects_low_spread",
                    test_pairs_trading_detects_low_spread) == NULL ||
        CU_add_test(pairs_trading_suite, "test_detects_high_spread",
                    test_pairs_trading_detects_high_spread) == NULL ||
        CU_add_test(pairs_trading_suite, "test_signal_vector_structure",
                    test_pairs_trading_signal_vector_structure) == NULL ||
        CU_add_test(pairs_trading_suite, "test_with_custom_ratio",
                    test_pairs_trading_with_custom_ratio) == NULL ||
        CU_add_test(pairs_trading_suite, "test_input_validation",
                    test_pairs_trading_input_validation) == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    basic_portfolio_suite = CU_add_suite("BasicPortfolio_Suite", NULL, NULL);
    if (basic_portfolio_suite == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (CU_add_test(basic_portfolio_suite, "test_initialization",
                    test_basic_portfolio_initialization) == NULL ||
        CU_add_test(basic_portfolio_suite, "test_single_buy_signal",
                    test_basic_portfolio_single_buy_signal) == NULL ||
        CU_add_test(basic_portfolio_suite, "test_multiple_buy_signals",
                    test_basic_portfolio_multiple_buy_signals) == NULL ||
        CU_add_test(basic_portfolio_suite, "test_sell_before_buy",
                    test_basic_portfolio_sell_before_buy) == NULL ||
        CU_add_test(basic_portfolio_suite, "test_asset_value",
                    test_basic_portfolio_asset_value) == NULL ||
        CU_add_test(basic_portfolio_suite, "test_total_value",
                    test_basic_portfolio_total_value) == NULL ||
        CU_add_test(basic_portfolio_suite, "test_equal_distribution_sizing",
                    test_basic_portfolio_equal_distribution_sizing) == NULL ||
        CU_add_test(basic_portfolio_suite, "test_strategy_guided_sizing",
                    test_basic_portfolio_strategy_guided_sizing) == NULL ||
        CU_add_test(basic_portfolio_suite, "test_invalid_asset_index",
                    test_basic_portfolio_invalid_asset_index) == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Stats suite (mean, downside, pnl, sharpe) */
    CU_pSuite stats_suite = CU_add_suite("Stats_Suite", NULL, NULL);
    if (stats_suite == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (CU_add_test(stats_suite, "test_mean_basic", test_mean_basic) == NULL ||
        CU_add_test(stats_suite, "test_downside_basic", test_downside_basic) == NULL ||
        CU_add_test(stats_suite, "test_pnlstats_basic", test_pnlstats_basic) == NULL ||
        CU_add_test(stats_suite, "test_sharpe_basic", test_sharpe_basic) == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }


    runner_suite = CU_add_suite("Runner_Suite", NULL, NULL);
    if (runner_suite == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (CU_add_test(runner_suite, "test_single_input_creation",
                    test_runner_single_input_creation) == NULL ||
        CU_add_test(runner_suite, "test_multi_input_creation",
                    test_runner_multi_input_creation) == NULL ||
        CU_add_test(runner_suite, "test_rejects_mismatched_reader_count",
                    test_runner_rejects_mismatched_reader_count) == NULL ||
        CU_add_test(runner_suite, "test_rejects_null_arguments",
                    test_runner_rejects_null_arguments) == NULL ||
        CU_add_test(runner_suite, "test_rejects_invalid_reader_count",
                    test_runner_rejects_invalid_reader_count) == NULL ||
        CU_add_test(runner_suite, "test_convenience_function",
                    test_runner_convenience_function) == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_pSuite global_stats_suite = CU_add_suite("Global statistics", NULL, NULL);
    if (!global_stats_suite) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    if (!CU_add_test(global_stats_suite, "mean", test_mean) ||
            !CU_add_test(global_stats_suite, "var_dof1", test_var_dof1) ||
            !CU_add_test(global_stats_suite, "var_dof0", test_var_dof0) ||
            !CU_add_test(global_stats_suite, "stdev_dof1", test_stdev_dof1)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
