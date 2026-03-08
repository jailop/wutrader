/**
 * Implementation of data type initialization functions.
 * 
 * (C) 2026 Jaime Lopez
 */

#include "wu.h"

Candle inline candle_init(int64_t timestamp, double open, double high, double low, double close, double volume) {
    Candle c;
    c.timestamp = timestamp;
    c.open = open;
    c.high = high;
    c.low = low;
    c.close = close;
    c.volume = volume;
    c.data_type = DATA_TYPE_CANDLE;
    return c;
}

Trade inline trade_init(int64_t timestamp, double price, double volume, Side side) {
    Trade t;
    t.timestamp = timestamp;
    t.price = price;
    t.volume = volume;
    t.side = side;
    t.data_type = DATA_TYPE_TRADE;
    return t;
}

SingleValue inline single_value_init(int64_t timestamp, double value) {
    SingleValue sv;
    sv.timestamp = timestamp;
    sv.value = value;
    sv.data_type = DATA_TYPE_SINGLE_VALUE;
    return sv;
}
