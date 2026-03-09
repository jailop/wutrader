#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "wu.h"

static void trim_line(char* line) {
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == '\n')) {
        line[--len] = '\0';
    }
}

static bool read_line(WU_Csv_Reader reader) {
    if (!reader->file) {
        reader->last_error = WU_CSV_ERROR_PARSE;
        return false;
    }
    
    if (!fgets(reader->line_buffer, WU_CSV_MAX_LINE_SIZE, reader->file)) {
        reader->last_error = WU_CSV_ERROR_EOF;
        return false;
    }
    
    trim_line(reader->line_buffer);
    return true;
}

static void* read_candle(WU_Csv_Reader reader) {
    if (!read_line(reader)) {
        return NULL;
    }
    
    if (sscanf(reader->line_buffer, "%ld,%lf,%lf,%lf,%lf,%lf",
               &(reader->data.candle.timestamp),
               &(reader->data.candle.open),
               &(reader->data.candle.high),
               &(reader->data.candle.low),
               &(reader->data.candle.close),
               &(reader->data.candle.volume)) != 6) {
        reader->last_error = WU_CSV_ERROR_PARSE;
        return NULL;
    }
    
    reader->data.candle.data_type = WU_DATA_TYPE_CANDLE;
    reader->last_error = WU_CSV_OK;
    return &reader->data.candle;
}

static void* read_trade(WU_Csv_Reader reader) {
    if (!read_line(reader)) {
        return NULL;
    }
    
    int side_value;
    if (sscanf(reader->line_buffer, "%ld,%lf,%lf,%d",
               &(reader->data.trade.timestamp),
               &(reader->data.trade.price),
               &(reader->data.trade.volume),
               &side_value) != 4) {
        reader->last_error = WU_CSV_ERROR_PARSE;
        return NULL;
    }
    
    reader->data.trade.side = (WU_Side)side_value;
    reader->data.trade.data_type = WU_DATA_TYPE_TRADE;
    reader->last_error = WU_CSV_OK;
    return &reader->data.trade;
}

static void* read_single_value(WU_Csv_Reader reader) {
    if (!read_line(reader)) {
        return NULL;
    }
    
    if (sscanf(reader->line_buffer, "%ld,%lf",
                &(reader->data.single_value.timestamp),
                &(reader->data.single_value.value)) != 2) {
        reader->last_error = WU_CSV_ERROR_PARSE;
        return NULL;
    }
    
    reader->data.single_value.data_type = WU_DATA_TYPE_SINGLE_VALUE;
    reader->last_error = WU_CSV_OK;
    return &reader->data.single_value;
}

static void wu_csv_reader_free(WU_Csv_Reader reader) {
    if (!reader) return;
    free(reader);
}

WU_Csv_Reader wu_csv_reader_new(FILE* file, WU_DataType data_type, bool has_headers) {
    if (!file) return NULL;
    
    WU_Csv_Reader reader = (WU_Csv_Reader)malloc(sizeof(struct WU_Csv_Reader_));
    if (!reader) return NULL;
    
    reader->file = file;
    reader->has_headers = has_headers;
    reader->data_type = data_type;
    reader->last_error = WU_CSV_OK;
    
    switch (data_type) {
        case WU_DATA_TYPE_CANDLE:
            reader->base.next = (void* (*)(struct WU_Reader_*))read_candle;
            break;
        case WU_DATA_TYPE_TRADE:
            reader->base.next = (void* (*)(struct WU_Reader_*))read_trade;
            break;
        case WU_DATA_TYPE_SINGLE_VALUE:
            reader->base.next = (void* (*)(struct WU_Reader_*))read_single_value;
            break;
        default:
            free(reader);
            return NULL;
    }
    reader->base.delete = (void (*)(struct WU_Reader_*))wu_csv_reader_free;    
    if (has_headers) {
        if (!fgets(reader->line_buffer, WU_CSV_MAX_LINE_SIZE, reader->file)) {
            free(reader);
            return NULL;
        }
    }
    
    return reader;
}


