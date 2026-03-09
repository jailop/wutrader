#ifndef WU_DATA_H
#define WU_DATA_H

#include "types.h"

/**
 * WU_Candle represents an aggregated data point to represent how prices
 * moved within a specific time period. It includes the timestamp of the
 * start of the period, the open price, the high price, the low price,
 * the close price, and the volume traded during that period. Instead of
 * directly using the WU_Candle structure, the wu_candle_init function should
 * be used to create a new WU_Candle instance, ensuring all fields are
 * properly initialized.
 */
typedef struct WU_Candle_ {
    int64_t timestamp;
    double open;
    double high;
    double low;
    double close;
    double volume;
    WU_DataType data_type;
} WU_Candle;

/**
 * WU_Trade represents a single trade in the market, including the
 * timestamp, price, volume, and side (buy or sell) of the trade. This
 * is a tick-level data point that represents a single transaction
 * in the market. As with WU_Candle, the wu_trade_init function should be used
 * to create a new WU_Trade instance.
 */
typedef struct {
    int64_t timestamp;
    double price;
    double volume;
    WU_Side side;
    WU_DataType data_type;
} WU_Trade;

/**
 * WU_Single represents a single value with a timestamp, which can
 * be a price, an indicator value, or any other scalar value associated
 * with a specific time. This is a generic data type that can be used to
 * represent any single-valued time series data. The wu_single_init
 * function should be used to create a new WU_Single instance.
 */
typedef struct {
    int64_t timestamp;
    double value;
    WU_DataType data_type;
} WU_Single;

WU_Candle wu_candle_init(int64_t timestamp, double open, double high, double low,
                   double close, double volume);

WU_Trade wu_trade_init(int64_t timestamp, double price, double volume, WU_Side side);

WU_Single wu_single_init(int64_t timestamp, double value);

WU_Signal wu_signal_init(int64_t timestamp, WU_Side side, double price, double quantity);

#define wu_candle_to_single_value(candle) \
    wu_single_init((candle)->timestamp, (candle)->close)

#define wu_trade_to_single_value(trade) \
    wu_single_init((trade)->timestamp, (trade)->price)

#endif // WU_DATA_H
