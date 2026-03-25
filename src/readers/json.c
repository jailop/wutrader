#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "wu.h"

static void trim_line(char* line) {
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == '\n')) {
        line[--len] = '\0';
    }
}

static bool read_line(WU_JsonReader reader) {
    if (!reader->file) {
        reader->last_error = WU_JSON_ERROR_PARSE;
        return false;
    }
    
    if (!fgets(reader->line_buffer, WU_JSON_MAX_LINE_SIZE, reader->file)) {
        reader->last_error = WU_JSON_ERROR_EOF;
        return false;
    }
    
    trim_line(reader->line_buffer);
    return true;
}

static bool json_has_field(const char* json, const char* key) {
    char search[256];
    snprintf(search, sizeof(search), "\"%s\":", key);
    return strstr(json, search) != NULL;
}

static double json_get_number_value(const char* json, const char* key) {
    char search[256];
    snprintf(search, sizeof(search), "\"%s\":", key);
    
    const char* pos = strstr(json, search);
    if (!pos) return NAN;
    
    pos += strlen(search);
    while (*pos && isspace(*pos)) pos++;
    
    char* endptr;
    double value = strtod(pos, &endptr);
    
    if (pos == endptr) return NAN;
    
    return value;
}

static long json_get_long_value(const char* json, const char* key) {
    char search[256];
    snprintf(search, sizeof(search), "\"%s\":", key);
    
    const char* pos = strstr(json, search);
    if (!pos) return 0;
    
    pos += strlen(search);
    while (*pos && isspace(*pos)) pos++;
    
    char* endptr;
    long value = strtol(pos, &endptr, 10);
    return value;
}

static void* read_candle_json(WU_JsonReader reader) {
    if (!read_line(reader)) {
        return NULL;
    }
    
    bool has_timestamp = json_has_field(reader->line_buffer, "timestamp");
    bool has_open = json_has_field(reader->line_buffer, "open");
    bool has_high = json_has_field(reader->line_buffer, "high");
    bool has_low = json_has_field(reader->line_buffer, "low");
    bool has_close = json_has_field(reader->line_buffer, "close");
    bool has_volume = json_has_field(reader->line_buffer, "volume");
    
    int field_count = has_timestamp + has_open + has_high + has_low + has_close + has_volume;
    
    if (field_count == 0) {
        reader->last_error = WU_JSON_ERROR_PARSE;
        return NULL;
    }
    
    if (!has_timestamp || !has_open || !has_high || !has_low || !has_close || !has_volume) {
        reader->last_error = WU_JSON_ERROR_MISSING_FIELD;
        return NULL;
    }
    
    reader->data.candle.timestamp.mark = json_get_long_value(reader->line_buffer, "timestamp");
    reader->data.candle.open = json_get_number_value(reader->line_buffer, "open");
    reader->data.candle.high = json_get_number_value(reader->line_buffer, "high");
    reader->data.candle.low = json_get_number_value(reader->line_buffer, "low");
    reader->data.candle.close = json_get_number_value(reader->line_buffer, "close");
    reader->data.candle.volume = json_get_number_value(reader->line_buffer, "volume");
    
    if (isnan(reader->data.candle.open) || isnan(reader->data.candle.high) || 
        isnan(reader->data.candle.low) || isnan(reader->data.candle.close) || 
        isnan(reader->data.candle.volume)) {
        reader->last_error = WU_JSON_ERROR_MISSING_FIELD;
        return NULL;
    }
    
    reader->data.candle.data_type = WU_DATA_TYPE_CANDLE;
    reader->last_error = WU_JSON_OK;
    return &reader->data.candle;
}

static void* read_trade_json(WU_JsonReader reader) {
    if (!read_line(reader)) {
        return NULL;
    }
    
    bool has_timestamp = json_has_field(reader->line_buffer, "timestamp");
    bool has_price = json_has_field(reader->line_buffer, "price");
    bool has_volume = json_has_field(reader->line_buffer, "volume");
    bool has_side = json_has_field(reader->line_buffer, "side");
    
    int field_count = has_timestamp + has_price + has_volume + has_side;
    
    if (field_count == 0) {
        reader->last_error = WU_JSON_ERROR_PARSE;
        return NULL;
    }
    
    if (!has_timestamp || !has_price || !has_volume || !has_side) {
        reader->last_error = WU_JSON_ERROR_MISSING_FIELD;
        return NULL;
    }
    
    reader->data.trade.timestamp.mark = json_get_long_value(reader->line_buffer, "timestamp");
    reader->data.trade.price = json_get_number_value(reader->line_buffer, "price");
    reader->data.trade.volume = json_get_number_value(reader->line_buffer, "volume");
    
    long side_value = json_get_long_value(reader->line_buffer, "side");
    reader->data.trade.side = (WU_Side)side_value;
    
    if (isnan(reader->data.trade.price) || isnan(reader->data.trade.volume)) {
        reader->last_error = WU_JSON_ERROR_MISSING_FIELD;
        return NULL;
    }
    
    reader->data.trade.data_type = WU_DATA_TYPE_TRADE;
    reader->last_error = WU_JSON_OK;
    return &reader->data.trade;
}

static void* read_single_value_json(WU_JsonReader reader) {
    if (!read_line(reader)) {
        return NULL;
    }
    
    bool has_timestamp = json_has_field(reader->line_buffer, "timestamp");
    bool has_value = json_has_field(reader->line_buffer, "value");
    
    int field_count = has_timestamp + has_value;
    
    if (field_count == 0) {
        reader->last_error = WU_JSON_ERROR_PARSE;
        return NULL;
    }
    
    if (!has_timestamp || !has_value) {
        reader->last_error = WU_JSON_ERROR_MISSING_FIELD;
        return NULL;
    }
    
    reader->data.single_value.timestamp.mark = json_get_long_value(reader->line_buffer, "timestamp");
    reader->data.single_value.value = json_get_number_value(reader->line_buffer, "value");
    
    if (isnan(reader->data.single_value.value)) {
        reader->last_error = WU_JSON_ERROR_MISSING_FIELD;
        return NULL;
    }
    
    reader->data.single_value.data_type = WU_DATA_TYPE_SINGLE_VALUE;
    reader->last_error = WU_JSON_OK;
    return &reader->data.single_value;
}

static void wu_json_reader_free(WU_JsonReader reader) {
    if (!reader) return;
    free(reader);
}

WU_JsonReader wu_json_reader_new(FILE* file, WU_DataType data_type,
        WU_TimeUnit time_units) {
    if (!file) return NULL;
    WU_JsonReader reader = (WU_JsonReader)malloc(sizeof(struct WU_JsonReader_));
    if (!reader) return NULL;
    
    reader->file = file;
    reader->data_type = data_type;
    reader->last_error = WU_JSON_OK;
    
    switch (data_type) {
        case WU_DATA_TYPE_CANDLE:
            reader->data.candle.timestamp.units = time_units;
            reader->base.next = (void* (*)(struct WU_Reader_*))read_candle_json;
            break;
        case WU_DATA_TYPE_TRADE:
            reader->data.trade.timestamp.units = time_units;
            reader->base.next = (void* (*)(struct WU_Reader_*))read_trade_json;
            break;
        case WU_DATA_TYPE_SINGLE_VALUE:
            reader->data.single_value.timestamp.units = time_units;
            reader->base.next = (void* (*)(struct WU_Reader_*))read_single_value_json;
            break;
        default:
            free(reader);
            return NULL;
    }
    reader->base.delete = (void (*)(struct WU_Reader_*))wu_json_reader_free;
    
    return reader;
}
