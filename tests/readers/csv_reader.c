#include <CUnit/CUnit.h>
#include <math.h>
#include "wu.h"

void test_csv_reader_reads_candles(void) {
    FILE* file = fopen("tests/data/btcusd.csv", "r");
    CU_ASSERT_PTR_NOT_NULL(file);
    
    CsvReader reader = csv_reader_new(file, DATA_TYPE_CANDLE, true);
    CU_ASSERT_PTR_NOT_NULL(reader);
    
    Candle* candle = (Candle*)reader_next(reader);
    CU_ASSERT_PTR_NOT_NULL(candle);
    CU_ASSERT_EQUAL(candle->timestamp, 1419984000);
    CU_ASSERT_DOUBLE_EQUAL(candle->open, 320.43, 0.001);
    CU_ASSERT_DOUBLE_EQUAL(candle->high, 320.43, 0.001);
    CU_ASSERT_DOUBLE_EQUAL(candle->low, 314.0, 0.001);
    CU_ASSERT_DOUBLE_EQUAL(candle->close, 314.25, 0.001);
    CU_ASSERT_DOUBLE_EQUAL(candle->volume, 8036550.0, 0.001);
    
    candle = (Candle*)reader_next(reader);
    CU_ASSERT_PTR_NOT_NULL(candle);
    CU_ASSERT_EQUAL(candle->timestamp, 1420070000);
    CU_ASSERT_DOUBLE_EQUAL(candle->close, 315.03, 0.001);
    
    csv_reader_free(reader);
    fclose(file);
}

void test_csv_reader_reads_trades(void) {
    FILE* file = fopen("tests/data/btcusd_trade.csv", "r");
    CU_ASSERT_PTR_NOT_NULL(file);
    
    CsvReader reader = csv_reader_new(file, DATA_TYPE_TRADE, false);
    CU_ASSERT_PTR_NOT_NULL(reader);
    
    Trade* trade = (Trade*)reader_next(reader);
    CU_ASSERT_PTR_NOT_NULL(trade);
    CU_ASSERT_EQUAL(trade->timestamp, 1419984000);
    CU_ASSERT_DOUBLE_EQUAL(trade->price, 320.43, 0.001);
    CU_ASSERT_DOUBLE_EQUAL(trade->volume, 8036550.0, 0.001);
    CU_ASSERT_EQUAL(trade->side, 2);
    
    trade = (Trade*)reader_next(reader);
    CU_ASSERT_PTR_NOT_NULL(trade);
    CU_ASSERT_EQUAL(trade->timestamp, 1420070000);
    CU_ASSERT_DOUBLE_EQUAL(trade->price, 314.08, 0.001);
    
    csv_reader_free(reader);
    fclose(file);
}

void test_csv_reader_reads_single_values(void) {
    FILE* file = fopen("tests/data/btcusd_price.csv", "r");
    CU_ASSERT_PTR_NOT_NULL(file);
    
    CsvReader reader = csv_reader_new(file, DATA_TYPE_SINGLE_VALUE, false);
    CU_ASSERT_PTR_NOT_NULL(reader);
    
    SingleValue* sv = (SingleValue*)reader_next(reader);
    CU_ASSERT_PTR_NOT_NULL(sv);
    CU_ASSERT_EQUAL(sv->timestamp, 1419984000);
    CU_ASSERT_DOUBLE_EQUAL(sv->value, 320.43, 0.001);
    
    sv = (SingleValue*)reader_next(reader);
    CU_ASSERT_PTR_NOT_NULL(sv);
    CU_ASSERT_EQUAL(sv->timestamp, 1420070000);
    CU_ASSERT_DOUBLE_EQUAL(sv->value, 314.08, 0.001);
    
    csv_reader_free(reader);
    fclose(file);
}

void test_csv_reader_returns_null_at_eof(void) {
    FILE* file = fopen("tests/data/btcusd_price.csv", "r");
    CU_ASSERT_PTR_NOT_NULL(file);
    
    CsvReader reader = csv_reader_new(file, DATA_TYPE_SINGLE_VALUE, false);
    CU_ASSERT_PTR_NOT_NULL(reader);
    
    SingleValue* sv;
    int count = 0;
    while ((sv = (SingleValue*)reader_next(reader)) != NULL) {
        count++;
    }
    
    CU_ASSERT_TRUE(count > 0);
    CU_ASSERT_EQUAL(reader_last_error(reader), CSV_ERROR_EOF);
    
    csv_reader_free(reader);
    fclose(file);
}

void test_csv_reader_handles_headers(void) {
    FILE* file = fopen("tests/data/btcusd.csv", "r");
    CU_ASSERT_PTR_NOT_NULL(file);
    
    CsvReader reader = csv_reader_new(file, DATA_TYPE_CANDLE, true);
    CU_ASSERT_PTR_NOT_NULL(reader);
    
    Candle* candle = (Candle*)reader_next(reader);
    CU_ASSERT_PTR_NOT_NULL(candle);
    CU_ASSERT_EQUAL(candle->timestamp, 1419984000);
    
    csv_reader_free(reader);
    fclose(file);
}
