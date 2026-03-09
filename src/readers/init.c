/**
 * Implementation of data type initialization functions.
 * 
 * (C) 2026 Jaime Lopez
 */

#include "wu.h"

inline WU_Candle wu_candle_init(int64_t timestamp, double open, double high, double low, double close, double volume) {
    WU_Candle c;
    c.timestamp = timestamp;
    c.open = open;
    c.high = high;
    c.low = low;
    c.close = close;
    c.volume = volume;
    c.data_type = WU_DATA_TYPE_CANDLE;
    return c;
}

inline WU_Trade wu_trade_init(int64_t timestamp, double price, double volume, WU_Side side) {
    WU_Trade t;
    t.timestamp = timestamp;
    t.price = price;
    t.volume = volume;
    t.side = side;
    t.data_type = WU_DATA_TYPE_TRADE;
    return t;
}

inline WU_Single wu_single_init(int64_t timestamp, double value) {
    WU_Single sv;
    sv.timestamp = timestamp;
    sv.value = value;
    sv.data_type = WU_DATA_TYPE_SINGLE_VALUE;
    return sv;
}

inline WU_Signal wu_signal_init(int64_t timestamp, WU_Side side, double price, double quantity) {
    WU_Signal s;
    s.timestamp = timestamp;
    s.side = side;
    s.price = price;
    s.quantity = quantity;
    return s;
}
