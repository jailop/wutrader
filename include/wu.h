#ifndef TZU_H
#define TZU_H

#include <stdint.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

typedef enum {
    SIDE_BUY = 0,
    SIDE_SELL = 1,
    SIDE_HOLD = 2
} Side;

typedef enum {
    DATA_TYPE_CANDLE = 0,
    DATA_TYPE_TRADE = 1,
    DATA_TYPE_SINGLE_VALUE = 2
} DataType;

typedef struct {
    int64_t timestamp;
    Side side;
    double price;
    double quantity;
} Signal;

typedef struct Portfolio_ {
    void (*update)(struct Portfolio_* portfolio, Signal signal);
    double (*value)(struct Portfolio_* portfolio);
    double (*pnl)(struct Portfolio_* portfolio);
}* Portfolio;

typedef struct Strategy_ {
    Signal (*update)(struct Strategy_* strategy, void* data);
}* Strategy;

#define strategy_update(strategy, data) ((strategy)->update((strategy), (data)))

typedef struct Reader_ {
    void* (*next)(struct Reader_* reader);
}* Reader;

typedef struct {
    int64_t timestamp;
    double open;
    double high;
    double low;
    double close;
    double volume;
    DataType data_type;
} Candle;

Candle candle_init(int64_t timestamp, double open, double high, double low, double close, double volume);

typedef struct {
    int64_t timestamp;
    double price;
    double volume;
    Side side;
    DataType data_type;
} Trade;

Trade trade_init(int64_t timestamp, double price, double volume, Side side);

typedef struct {
    int64_t timestamp;
    double value;
    DataType data_type;
} SingleValue;

SingleValue single_value_init(int64_t timestamp, double value);

#define candle_to_single_value(candle) \
    single_value_init((candle)->timestamp, (candle)->close)

typedef enum {
    NSEC = 0,
    USEC = 1,
    MSEC = 2,
    SEC = 3,
    MIN = 4,
    HOUR = 5,
    DAY = 6,
    WEEK = 7,
    MONTH = 8,
    YEAR = 9
} TimeUnit;

typedef struct Indicator_ {
    double value;
    void (*update)(struct Indicator_* ind, double new_value);
}* Indicator;

#define indicator_update(ind, value) do { \
    if ((ind)->base.update) \
        (ind)->base.update((Indicator)(ind), (value)); \
} while(0)

#define indicator_value(ind) ((ind)->base.value)

typedef struct MovingAverage_ {
    struct Indicator_ base;
    double* prev_values;
    int window_size;
    int pos;
    int len;
    double sum;
}* MovingAverage;

MovingAverage moving_average_new(int window_size);
void moving_average_free(MovingAverage ma);

typedef struct ExponentialMovingAverage_ {
    struct Indicator_ base;
    double prev_value;
    double alpha;
    int len;
    int period;
}* ExponentialMovingAverage;

ExponentialMovingAverage exponential_moving_average_new(int period, double smoothing);
void exponential_moving_average_free(ExponentialMovingAverage ema);

typedef struct CrossOverStrat_ {
    struct Strategy_ base;
    MovingAverage short_ma;
    MovingAverage long_ma;
    double threshold;
    Side last_signal;
}* CrossOverStrat;

CrossOverStrat cross_over_strat_new(int short_window, int long_window, double threshold);
void cross_over_strat_free(CrossOverStrat strat);

#define CSV_MAX_LINE_SIZE 2048

typedef enum {
    CSV_OK = 0,
    CSV_ERROR_FILE = 1,
    CSV_ERROR_PARSE = 2,
    CSV_ERROR_EOF = 3
} CsvError;

#define reader_next(reader) (((Reader)(reader))->next((Reader)(reader)))
#define reader_last_error(reader) ((reader)->last_error)

typedef struct CsvReader_ {
    struct Reader_ base;
    FILE* file;
    char line_buffer[CSV_MAX_LINE_SIZE];
    bool has_headers;
    DataType data_type;
    CsvError last_error;
    union {
        Candle candle;
        Trade trade;
        SingleValue single_value;
    } data;
}* CsvReader;

CsvReader csv_reader_new(FILE *file, DataType data_type, bool has_headers);
void csv_reader_free(CsvReader reader);

typedef struct Position_ {
    int64_t timestamp;
    double quantity;
    double price;
}* Position;

#define position_add(vec, pos) ((vec)->add((vec), (pos)))
#define position_remove(vec, index) ((vec)->remove((vec), (index)))
#define position_clear(vec) ((vec)->clear((vec)))
#define position_get(vec, index, found) ((vec)->get((vec), (index), (found)))
#define position_total_quantity(vec) ((vec)->total_quantity((vec)))

typedef struct PositionVector {
    struct Position_* positions;
    bool* active;
    int count;
    int capacity;
    void (*add)(struct PositionVector* vec, Position pos);
    void (*remove)(struct PositionVector* vec, int index);
    void (*clear)(struct PositionVector* vec);
    struct Position_ (*get)(struct PositionVector* vec, int index, bool* found);
    double (*total_quantity)(struct PositionVector* vec);
} PositionVector;

PositionVector* position_vector_new(void);
void position_vector_free(PositionVector* vec);

typedef enum {
    CLOSE_REASON_SIGNAL = 0,
    CLOSE_REASON_STOP_LOSS = 1,
    CLOSE_REASON_TAKE_PROFIT = 2
} CloseReason;

typedef struct PortfolioStats_ {
    int64_t total_trades;
    int64_t winning_trades;
    int64_t losing_trades;
    int64_t stop_loss_exits;
    int64_t take_profit_exits;
    double total_profit;
    double total_loss;
    double max_win;
    double max_loss;
}* PortfolioStats;

PortfolioStats portfolio_stats_new(void);
void portfolio_stats_free(PortfolioStats stats);
void portfolio_stats_record_trade(PortfolioStats stats, double pnl, CloseReason reason);
void portfolio_stats_reset(PortfolioStats stats);


typedef enum PositionSizeType {
    POSITION_SIZE_ABS = 0,
    POSITION_SIZE_PCT = 1
} PositionSizeType;

typedef struct PositionSizingParams {
    PositionSizeType size_type;
    double size_value;
} PositionSizingParams;

typedef struct SingleAssetPortfolioParams {
    double initial_cash;
    double tx_cost_pct;
    double stop_loss_pct;
    double take_profit_pct;
    double slippage_pct;
    PositionSizingParams position_sizing;
} SingleAssetPortfolioParams;

typedef struct SingleAssetPortfolioTrack {
    double cash;
    struct PositionVector* positions;
    double last_price;
    double accum_expenses;
    PortfolioStats stats;
} SingleAssetPortfolioTrack;

#define portfolio_update(portfolio, signal) do { \
    ((Portfolio)(portfolio))->update((Portfolio)(portfolio), (signal)); \
} while(0)

#define portfolio_value(portfolio) (((Portfolio)(portfolio))->value((Portfolio)(portfolio)))

#define portfolio_pnl(portfolio) (((Portfolio)(portfolio))->pnl((Portfolio)(portfolio)))

typedef struct SingleAssetPortfolio_ {
    struct Portfolio_ base;
    SingleAssetPortfolioParams params;
    SingleAssetPortfolioTrack track;
}* SingleAssetPortfolio;

SingleAssetPortfolio single_asset_portfolio_new(SingleAssetPortfolioParams params);
void single_asset_portfolio_free(SingleAssetPortfolio portfolio);

#define runner_run(runner, verbose) ((runner)->run((runner), (verbose)))

typedef struct BasicRunner_ {
    Portfolio portfolio;
    Strategy strategy;
    Reader reader;
    void (*run)(struct BasicRunner_* runner, bool verbose);
}* BasicRunner;

BasicRunner basic_runner_new(Portfolio portfolio, Strategy strategy, Reader reader);
void basic_runner_free(BasicRunner runner);

#endif // TZU_H
