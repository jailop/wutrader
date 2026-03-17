#include <CUnit/CUnit.h>
#include <math.h>
#include "wu.h"

void test_json_reader_reads_candles(void) {
    FILE* file = fopen("tests/data/candles.jsonl", "r");
    CU_ASSERT_PTR_NOT_NULL(file);
    
    WU_JsonReader reader = wu_json_reader_new(file, WU_DATA_TYPE_CANDLE, WU_TIME_UNIT_SECONDS);
    CU_ASSERT_PTR_NOT_NULL(reader);
    
    WU_Candle* candle = (WU_Candle*)wu_reader_next(reader);
    CU_ASSERT_PTR_NOT_NULL(candle);
    CU_ASSERT_EQUAL(candle->timestamp.mark, 1419984000);
    CU_ASSERT_DOUBLE_EQUAL(candle->open, 320.43, 0.001);
    CU_ASSERT_DOUBLE_EQUAL(candle->high, 320.43, 0.001);
    CU_ASSERT_DOUBLE_EQUAL(candle->low, 314.0, 0.001);
    CU_ASSERT_DOUBLE_EQUAL(candle->close, 314.25, 0.001);
    CU_ASSERT_DOUBLE_EQUAL(candle->volume, 8036550.0, 0.001);
    
    candle = (WU_Candle*)wu_reader_next(reader);
    CU_ASSERT_PTR_NOT_NULL(candle);
    CU_ASSERT_EQUAL(candle->timestamp.mark, 1420070000);
    CU_ASSERT_DOUBLE_EQUAL(candle->close, 315.03, 0.001);
    
    wu_reader_delete((WU_Reader)reader);
    fclose(file);
}

void test_json_reader_reads_trades(void) {
    FILE* file = fopen("tests/data/trades.jsonl", "r");
    CU_ASSERT_PTR_NOT_NULL(file);
    
    WU_JsonReader reader = wu_json_reader_new(file, WU_DATA_TYPE_TRADE, WU_TIME_UNIT_SECONDS);
    CU_ASSERT_PTR_NOT_NULL(reader);
    
    WU_Trade* trade = (WU_Trade*)wu_reader_next(reader);
    CU_ASSERT_PTR_NOT_NULL(trade);
    CU_ASSERT_EQUAL(trade->timestamp.mark, 1419984000);
    CU_ASSERT_DOUBLE_EQUAL(trade->price, 320.43, 0.001);
    CU_ASSERT_DOUBLE_EQUAL(trade->volume, 8036550.0, 0.001);
    CU_ASSERT_EQUAL(trade->side, 2);
    
    trade = (WU_Trade*)wu_reader_next(reader);
    CU_ASSERT_PTR_NOT_NULL(trade);
    CU_ASSERT_EQUAL(trade->timestamp.mark, 1420070000);
    CU_ASSERT_DOUBLE_EQUAL(trade->price, 314.08, 0.001);
    
    wu_reader_delete((WU_Reader)reader);
    fclose(file);
}

void test_json_reader_reads_single_values(void) {
    FILE* file = fopen("tests/data/prices.jsonl", "r");
    CU_ASSERT_PTR_NOT_NULL(file);
    
    WU_JsonReader reader = wu_json_reader_new(file, WU_DATA_TYPE_SINGLE_VALUE, WU_TIME_UNIT_SECONDS);
    CU_ASSERT_PTR_NOT_NULL(reader);
    
    WU_Single* sv = (WU_Single*)wu_reader_next(reader);
    CU_ASSERT_PTR_NOT_NULL(sv);
    CU_ASSERT_EQUAL(sv->timestamp.mark, 1419984000);
    CU_ASSERT_DOUBLE_EQUAL(sv->value, 320.43, 0.001);
    
    sv = (WU_Single*)wu_reader_next(reader);
    CU_ASSERT_PTR_NOT_NULL(sv);
    CU_ASSERT_EQUAL(sv->timestamp.mark, 1420070000);
    CU_ASSERT_DOUBLE_EQUAL(sv->value, 314.08, 0.001);
    
    wu_reader_delete((WU_Reader)reader);
    fclose(file);
}

void test_json_reader_returns_null_at_eof(void) {
    FILE* file = fopen("tests/data/prices.jsonl", "r");
    CU_ASSERT_PTR_NOT_NULL(file);
    
    WU_JsonReader reader = wu_json_reader_new(file, WU_DATA_TYPE_SINGLE_VALUE, WU_TIME_UNIT_SECONDS);
    CU_ASSERT_PTR_NOT_NULL(reader);
    
    WU_Single* sv;
    int count = 0;
    while ((sv = (WU_Single*)wu_reader_next(reader)) != NULL) {
        count++;
    }
    
    CU_ASSERT_TRUE(count > 0);
    CU_ASSERT_EQUAL(wu_reader_last_error(reader), WU_JSON_ERROR_EOF);
    
    wu_reader_delete((WU_Reader)reader);
    fclose(file);
}

void test_json_reader_handles_invalid_json(void) {
    FILE* file = fopen("tests/data/invalid.jsonl", "r");
    CU_ASSERT_PTR_NOT_NULL(file);
    
    WU_JsonReader reader = wu_json_reader_new(file, WU_DATA_TYPE_CANDLE, WU_TIME_UNIT_SECONDS);
    CU_ASSERT_PTR_NOT_NULL(reader);
    
    WU_Candle* candle = (WU_Candle*)wu_reader_next(reader);
    CU_ASSERT_PTR_NULL(candle);
    CU_ASSERT_EQUAL(wu_reader_last_error(reader), WU_JSON_ERROR_PARSE);
    
    wu_reader_delete((WU_Reader)reader);
    fclose(file);
}

void test_json_reader_handles_missing_fields(void) {
    FILE* file = fopen("tests/data/incomplete.jsonl", "r");
    CU_ASSERT_PTR_NOT_NULL(file);
    
    WU_JsonReader reader = wu_json_reader_new(file, WU_DATA_TYPE_CANDLE, WU_TIME_UNIT_SECONDS);
    CU_ASSERT_PTR_NOT_NULL(reader);
    
    WU_Candle* candle = (WU_Candle*)wu_reader_next(reader);
    CU_ASSERT_PTR_NULL(candle);
    CU_ASSERT_EQUAL(wu_reader_last_error(reader), WU_JSON_ERROR_MISSING_FIELD);
    
    wu_reader_delete((WU_Reader)reader);
    fclose(file);
}
