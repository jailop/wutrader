#ifndef _STATS_H_
#define _STATS_H_

#include <time.h>

/**
 * WU_PortfolioStats tracks portfolio state, positions, and trading statistics.
 * This is the central source of knowledge about portfolio performance and holdings.
 */
typedef struct WU_PortfolioStats_ {
    void (*update)(struct WU_PortfolioStats_* stats, double cash,
            double portfolio_value, WU_TimeStamp timestamp);
    void (*record_trade)(struct WU_PortfolioStats_* stats, double pnl,
            WU_CloseReason reason);
    void (*update_position)(struct WU_PortfolioStats_* stats, int asset_index,
            const char* symbol, double quantity, double value, double last_price);
    char* (*to_keyvalue)(struct WU_PortfolioStats_* stats);
    char* (*to_json)(struct WU_PortfolioStats_* stats, bool pretty);
    void (*reset)(struct WU_PortfolioStats_* stats);
    void (*delete)(struct WU_PortfolioStats_* stats);
    
    // Portfolio state
    double initial_cash;
    double current_cash;
    double portfolio_value;
    WU_TimeStamp last_update;
    
    // Cost tracking
    double accum_tx_fees;
    double accum_borrow_interest;
    
    // Trading statistics
    int64_t total_trades;
    int64_t winning_trades;
    int64_t losing_trades;
    int64_t stop_loss_exits;
    int64_t take_profit_exits;
    double total_profit;
    double total_loss;
    double max_win;
    double max_loss;
    
    // Position tracking (dynamic arrays)
    char** symbols;
    double* quantities;
    double* values;
    double* last_prices;
    int num_assets;
    int capacity;
}* WU_PortfolioStats;

/**
 * Creates a new WU_PortfolioStats instance.
 */
WU_PortfolioStats wu_portfolio_stats_new(double initial_cash);

/**
 * Updates portfolio state in stats.
 */
#define wu_portfolio_stats_update(stats, cash, value, timestamp) do { \
    if ((stats)->update) \
        (stats)->update((stats), (cash), (value), (timestamp)); \
} while(0)

/**
 * Records a trade in the stats.
 */
#define wu_portfolio_stats_record_trade(stats, pnl, reason) do { \
    if ((stats)->record_trade) \
        (stats)->record_trade((stats), (pnl), (reason)); \
} while(0)

/**
 * Updates position info for a specific asset.
 */
#define wu_portfolio_stats_update_position(stats, idx, sym, qty, val, price) do { \
    if ((stats)->update_position) \
        (stats)->update_position((stats), (idx), (sym), (qty), (val), (price)); \
} while(0)

/**
 * Converts stats to key=value format string.
 * Caller must free the returned string.
 */
#define wu_portfolio_stats_to_keyvalue(stats) \
    ((stats)->to_keyvalue ? (stats)->to_keyvalue((stats)) : NULL)

/**
 * Converts stats to JSON format string.
 * Caller must free the returned string.
 */
#define wu_portfolio_stats_to_json(stats, pretty) \
    ((stats)->to_json ? (stats)->to_json((stats), (pretty)) : NULL)

#define wu_portfolio_stats_reset(stats) do { \
    if ((stats)->reset) \
        (stats)->reset((stats)); \
} while(0)

#define wu_portfolio_stats_delete(stats) do { \
    if ((stats) && (stats)->delete) \
        (stats)->delete((stats)); \
} while(0)

#endif // _STATS_H_
