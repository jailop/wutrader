#ifndef WU_PORTFOLIO_H
#define WU_PORTFOLIO_H

#include "types.h"
#include "data.h"

/**
 * Base for a portfolio, which defines the minimal interface for
 * updating the portfolio with a new signal, calculating the current
 * value of the portfolio, and calculating the profit and loss (PnL), as
 * well as a method to free the portfolio's resources. The delete method
 * should be called by the runner taking ownership of the portfolio.  It
 * is expected that specific portfolio implementations will extend this
 * base structure and implement the defined methods.
 */
typedef struct Portfolio_ {
    void (*update)(struct Portfolio_* portfolio, const Signal signal);
    double (*value)(const struct Portfolio_* portfolio);
    double (*pnl)(const struct Portfolio_* portfolio);
    void (*delete)(struct Portfolio_* portfolio);
}* Portfolio;

#define portfolio_update(portfolio, signal) do { \
    ((Portfolio)(portfolio))->update((Portfolio)(portfolio), (signal)); \
} while(0)

#define portfolio_value(portfolio) (((Portfolio)(portfolio))->value((Portfolio)(portfolio)))

#define portfolio_pnl(portfolio) (((Portfolio)(portfolio))->pnl((Portfolio)(portfolio)))

#define portfolio_delete(portfolio) do { \
    if ((portfolio)->delete) \
        (portfolio)->delete((Portfolio)(portfolio)); \
} while(0)

/**
 * Position represents an open position in the portfolio.
 */
typedef struct Position_ {
    int64_t timestamp;
    double quantity;
    double price;
    bool active;
}* Position;

/**
 * PositionVector is a data structure that holds multiple positions.
 */
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
    void (*delete)(struct PositionVector* vec);
} PositionVector;

#define position_add(vec, pos) ((vec)->add((vec), (pos)))
#define position_remove(vec, index) ((vec)->remove((vec), (index)))
#define position_clear(vec) ((vec)->clear((vec)))
#define position_get(vec, index, found) ((vec)->get((vec), (index), (found)))
#define position_total_quantity(vec) ((vec)->total_quantity((vec)))
#define position_vector_delete(vec) do { \
    if ((vec)->delete) \
        (vec)->delete((PositionVector*)(vec)); \
} while(0)

PositionVector* position_vector_new(void);

/**
 * PortfolioStats tracks trading statistics.
 */
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
    void (*record_trade)(struct PortfolioStats_* stats, double pnl, CloseReason reason);
    void (*reset)(struct PortfolioStats_* stats);
    void (*delete)(struct PortfolioStats_* stats);
}* PortfolioStats;

PortfolioStats portfolio_stats_new(void);

#define portfolio_stats_record_trade(stats, pnl, reason) do { \
    if ((stats)->record_trade) \
        (stats)->record_trade((stats), (pnl), (reason)); \
} while(0)

#define portfolio_stats_reset(stats) do { \
    if ((stats)->reset) \
        (stats)->reset((stats)); \
} while(0)

#define portfolio_stats_delete(stats) do { \
    if ((stats) && (stats)->delete) \
        (stats)->delete((stats)); \
} while(0)

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
    PositionVector* positions;
    double last_price;
    double accum_expenses;
    PortfolioStats stats;
} SingleAssetPortfolioTrack;

typedef struct SingleAssetPortfolio_ {
    struct Portfolio_ base;
    SingleAssetPortfolioParams params;
    SingleAssetPortfolioTrack track;
}* SingleAssetPortfolio;

SingleAssetPortfolio single_asset_portfolio_new(SingleAssetPortfolioParams params);

#endif // WU_PORTFOLIO_H
