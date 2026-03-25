#ifndef _STATS_H_
#define _STATS_H_

#include <time.h>
#include "indicators.h"

/**
 * PnL Statistics Result - contains mean and standard deviation of PnL values.
 */
typedef struct {
    double mean;           // Mean PnL per trade
    double stddev;         // Standard deviation of PnL
} WU_PnLStatsResult;

/**
 * PnL Statistics - combined wrapper using separated indicators. Exists in stats
 * as a convenient composite that can be used by higher-level code (e.g. portfolios).
 */
typedef struct WU_PnLStats_ {
    WU_PnLStatsResult (*update)(struct WU_PnLStats_* self, double pnl);
    void (*delete)(struct WU_PnLStats_* self);
    WU_PnLStatsResult value;
    /* internal Welford state */
    double pnl_mean;       // Mean of PnL
    double pnl_m2;         // Sum of squared deviations
    int64_t count;         // Number of trades
}* WU_PnLStats;

WU_PnLStats wu_pnl_stats_new(void);

/* Composite ratio indicators (moved from indicators.h so combined types live in stats.h)
 * These combine lower-level indicators and time tracking to compute annualized metrics.
 */

typedef struct WU_SharpeRatio_ {
    double (*update)(struct WU_SharpeRatio_* self, WU_PerformanceUpdate perf);
    void (*delete)(struct WU_SharpeRatio_* self);
    double value;           // Current Sharpe ratio
    WU_PnLStats return_stats; // Online mean/stddev of returns (Welford)
    double risk_free_rate;  // Annual risk-free rate
    double initial_value;   // Initial portfolio value used for return calc
    double prev_value;      // Previous portfolio value for return calc
    int64_t count;          // Number of returns observed
    int64_t start_time;     // First timestamp observed
    int64_t end_time;       // Last timestamp observed
    WU_TimeUnit time_unit;  // Time unit for period calculations
}* WU_SharpeRatio;

WU_SharpeRatio wu_sharpe_ratio_new(double initial_value, double risk_free_rate);

typedef struct WU_SortinoRatio_ {
    double (*update)(struct WU_SortinoRatio_* self, WU_PerformanceUpdate perf);
    void (*delete)(struct WU_SortinoRatio_* self);
    double value;           // Current Sortino ratio
    WU_PnLStats return_stats; // Online mean/stddev of returns (Welford)
    WU_Downside downside;   // Downside deviation tracker (negative returns)
    double risk_free_rate;  // Annual risk-free rate
    double prev_value;      // Previous portfolio value for return calc
    int64_t count;          // Number of returns observed
    int64_t start_time;     // First timestamp observed
    int64_t end_time;       // Last timestamp observed
    WU_TimeUnit time_unit;  // Time unit for period calculations
}* WU_SortinoRatio;

WU_SortinoRatio wu_sortino_ratio_new(double initial_value, double risk_free_rate);

typedef struct WU_CalmarRatio_ {
    double (*update)(struct WU_CalmarRatio_* self, WU_PerformanceUpdate perf);
    void (*delete)(struct WU_CalmarRatio_* self);
    double value;           // Current Calmar ratio
    WU_MaxDrawdown max_drawdown;
    double initial_value;   // Initial portfolio value
    double prev_value;      // Previous portfolio value for return calc
    int64_t count;          // Number of returns observed
    int64_t start_time;     // First timestamp observed
    int64_t end_time;       // Last timestamp observed
    WU_TimeUnit time_unit;  // Time unit for period calculations
}* WU_CalmarRatio;

WU_CalmarRatio wu_calmar_ratio_new(double initial_value);

/**
 * WU_PortfolioStats tracks portfolio state, positions, and trading statistics.
 * This is the central source of knowledge about portfolio performance and holdings.
 * 
 * Performance metrics (MaxDrawdown, Sharpe, Sortino, Calmar) are maintained as
 * indicators that update sequentially with each portfolio value change.
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
    double accum_pnl;       // Accumulated PnL from all trades
    
    // Position tracking (dynamic arrays)
    char** symbols;
    double* quantities;
    double* values;
    double* last_prices;
    int num_assets;
    int capacity;
    
    // Performance indicators (updated sequentially)
    WU_MaxDrawdown max_drawdown;
    WU_SharpeRatio sharpe_ratio;
    WU_SortinoRatio sortino_ratio;
    WU_CalmarRatio calmar_ratio;
    WU_PnLStats pnl_stats;
}* WU_PortfolioStats;

/**
 * Creates a new WU_PortfolioStats instance.
 * 
 * @param initial_cash Initial cash amount
 * @param risk_free_rate Annual risk-free rate for Sharpe/Sortino calculations (e.g., 0.03 for 3%)
 */
WU_PortfolioStats wu_portfolio_stats_new(double initial_cash, double risk_free_rate);

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
