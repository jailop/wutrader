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
typedef struct WU_Portfolio_ {
    void (*update)(struct WU_Portfolio_* portfolio, const WU_Signal signal);
    double (*value)(const struct WU_Portfolio_* portfolio);
    double (*pnl)(const struct WU_Portfolio_* portfolio);
    void (*delete)(struct WU_Portfolio_* portfolio);
}* WU_Portfolio;

#define wu_portfolio_update(portfolio, signal) do { \
    ((WU_Portfolio)(portfolio))->update((WU_Portfolio)(portfolio), (signal)); \
} while(0)

#define wu_portfolio_value(portfolio) (((WU_Portfolio)(portfolio))->value((WU_Portfolio)(portfolio)))

#define wu_portfolio_pnl(portfolio) (((WU_Portfolio)(portfolio))->pnl((WU_Portfolio)(portfolio)))

#define wu_portfolio_delete(portfolio) do { \
    if ((portfolio)->delete) \
        (portfolio)->delete((WU_Portfolio)(portfolio)); \
} while(0)

/**
 * WU_Position represents an open position in the portfolio.
 */
typedef struct WU_Position_ {
    int64_t timestamp;
    double quantity;
    double price;
    bool active;
}* WU_Position;

/**
 * WU_PositionVector is a data structure that holds multiple positions.
 */
typedef struct WU_PositionVector {
    struct WU_Position_* positions;
    bool* active;
    int count;
    int capacity;
    void (*add)(struct WU_PositionVector* vec, WU_Position pos);
    void (*remove)(struct WU_PositionVector* vec, int index);
    void (*clear)(struct WU_PositionVector* vec);
    struct WU_Position_ (*get)(struct WU_PositionVector* vec, int index, bool* found);
    double (*total_quantity)(struct WU_PositionVector* vec);
    void (*delete)(struct WU_PositionVector* vec);
} WU_PositionVector;

#define wu_position_add(vec, pos) ((vec)->add((vec), (pos)))
#define wu_position_remove(vec, index) ((vec)->remove((vec), (index)))
#define wu_position_clear(vec) ((vec)->clear((vec)))
#define wu_position_get(vec, index, found) ((vec)->get((vec), (index), (found)))
#define wu_position_total_quantity(vec) ((vec)->total_quantity((vec)))
#define wu_position_vector_delete(vec) do { \
    if ((vec)->delete) \
        (vec)->delete((WU_PositionVector*)(vec)); \
} while(0)

WU_PositionVector* wu_position_vector_new(void);

/**
 * WU_PortfolioStats tracks trading statistics.
 */
typedef struct WU_PortfolioStats_ {
    int64_t total_trades;
    int64_t winning_trades;
    int64_t losing_trades;
    int64_t stop_loss_exits;
    int64_t take_profit_exits;
    double total_profit;
    double total_loss;
    double max_win;
    double max_loss;
    void (*record_trade)(struct WU_PortfolioStats_* stats, double pnl, WU_CloseReason reason);
    void (*reset)(struct WU_PortfolioStats_* stats);
    void (*delete)(struct WU_PortfolioStats_* stats);
}* WU_PortfolioStats;

WU_PortfolioStats wu_portfolio_stats_new(void);

#define wu_portfolio_stats_record_trade(stats, pnl, reason) do { \
    if ((stats)->record_trade) \
        (stats)->record_trade((stats), (pnl), (reason)); \
} while(0)

#define wu_portfolio_stats_reset(stats) do { \
    if ((stats)->reset) \
        (stats)->reset((stats)); \
} while(0)

#define wu_portfolio_stats_delete(stats) do { \
    if ((stats) && (stats)->delete) \
        (stats)->delete((stats)); \
} while(0)

typedef struct WU_PositionSizingParams {
    WU_PositionSizeType size_type;
    double size_value;
} WU_PositionSizingParams;

typedef struct WU_SingleAssetPortfolioParams {
    double initial_cash;
    double tx_cost_pct;
    double stop_loss_pct;
    double take_profit_pct;
    double slippage_pct;
    WU_PositionSizingParams wu_position_sizing;
} WU_SingleAssetPortfolioParams;

typedef struct WU_SingleAssetPortfolioTrack {
    double cash;
    WU_PositionVector* positions;
    double last_price;
    double accum_expenses;
    WU_PortfolioStats stats;
} WU_SingleAssetPortfolioTrack;

typedef struct WU_SingleAssetPortfolio_ {
    struct WU_Portfolio_ base;
    WU_SingleAssetPortfolioParams params;
    WU_SingleAssetPortfolioTrack track;
}* WU_SingleAssetPortfolio;

WU_SingleAssetPortfolio wu_singleasset_portfolio_new(WU_SingleAssetPortfolioParams params);

#endif // WU_PORTFOLIO_H
