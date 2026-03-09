/**
 * Implementation of data type initialization functions.
 * 
 * (C) 2026 Jaime Lopez
 */

#include "wu.h"

inline Candle candle_init(int64_t timestamp, double open, double high, double low, double close, double volume) {
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

inline Trade trade_init(int64_t timestamp, double price, double volume, Side side) {
    Trade t;
    t.timestamp = timestamp;
    t.price = price;
    t.volume = volume;
    t.side = side;
    t.data_type = DATA_TYPE_TRADE;
    return t;
}

inline SingleValue single_value_init(int64_t timestamp, double value) {
    SingleValue sv;
    sv.timestamp = timestamp;
    sv.value = value;
    sv.data_type = DATA_TYPE_SINGLE_VALUE;
    return sv;
}

inline Signal signal_init(int64_t timestamp, Side side, double price, double quantity) {
    Signal s;
    s.timestamp = timestamp;
    s.side = side;
    s.price = price;
    s.quantity = quantity;
    return s;
}
