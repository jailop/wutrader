#ifndef WU_DATA_H
#define WU_DATA_H

#include "types.h"

/**
 * Candle represents an aggregated data point to represent how prices
 * moved within a specific time period. It includes the timestamp of the
 * start of the period, the open price, the high price, the low price,
 * the close price, and the volume traded during that period. Instead of
 * directly using the Candle structure, the candle_init function should
 * be used to create a new Candle instance, ensuring all fields are
 * properly initialized.
 */
typedef struct Candle_ {
    int64_t timestamp;
    double open;
    double high;
    double low;
    double close;
    double volume;
    DataType data_type;
} Candle;

/**
 * Trade represents a single trade in the market, including the
 * timestamp, price, volume, and side (buy or sell) of the trade. This
 * is a tick-level data point that represents a single transaction
 * in the market. As with Candle, the trade_init function should be used
 * to create a new Trade instance.
 */
typedef struct {
    int64_t timestamp;
    double price;
    double volume;
    Side side;
    DataType data_type;
} Trade;

/**
 * SingleValue represents a single value with a timestamp, which can
 * be a price, an indicator value, or any other scalar value associated
 * with a specific time. This is a generic data type that can be used to
 * represent any single-valued time series data. The single_value_init
 * function should be used to create a new SingleValue instance.
 */
typedef struct {
    int64_t timestamp;
    double value;
    DataType data_type;
} SingleValue;

Candle candle_init(int64_t timestamp, double open, double high, double low,
                   double close, double volume);

Trade trade_init(int64_t timestamp, double price, double volume, Side side);

SingleValue single_value_init(int64_t timestamp, double value);

Signal signal_init(int64_t timestamp, Side side, double price, double quantity);

#define candle_to_single_value(candle) \
    single_value_init((candle)->timestamp, (candle)->close)

#define trade_to_single_value(trade) \
    single_value_init((trade)->timestamp, (trade)->price)

#endif // WU_DATA_H
