#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>

extern void test_sma_returns_nan_during_warmup(void);
extern void test_sma_calculates_correct_moving_average(void);
extern void test_sma_sliding_window_updates_correctly(void);
extern void test_ema_returns_nan_during_warmup(void);
extern void test_ema_first_value_is_sma_of_warmup_period(void);
extern void test_ema_applies_exponential_smoothing_after_warmup(void);
extern void test_csv_reader_reads_candles(void);
extern void test_csv_reader_reads_trades(void);
extern void test_csv_reader_reads_single_values(void);
extern void test_csv_reader_returns_null_at_eof(void);
extern void test_csv_reader_handles_headers(void);
extern void test_crossover_initialization(void);
extern void test_crossover_holds_during_warmup(void);
extern void test_crossover_generates_buy_signal(void);
extern void test_crossover_generates_sell_signal(void);
extern void test_crossover_no_repeat_signals(void);
extern void test_crossover_with_real_data(void);
extern void test_crossover_threshold_prevents_noise(void);

int main(void) {
    CU_pSuite sma_suite = NULL;
    CU_pSuite ema_suite = NULL;
    CU_pSuite csv_suite = NULL;
    CU_pSuite crossover_suite = NULL;

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
                    test_sma_calculates_correct_moving_average) == NULL ||
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
        CU_add_test(crossover_suite, "test_with_real_data",
                    test_crossover_with_real_data) == NULL ||
        CU_add_test(crossover_suite, "test_threshold_prevents_noise",
                    test_crossover_threshold_prevents_noise) == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
