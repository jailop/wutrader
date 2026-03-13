#ifndef WU_PORTFOLIO_H
#define WU_PORTFOLIO_H

#include "types.h"
#include "data.h"
#include "positions.h"

/**
 * Base for a portfolio, which defines the minimal interface for
 * updating the portfolio with signals, calculating the current
 * value of the portfolio, and calculating the profit and loss (PnL), as
 * well as a method to free the portfolio's resources. The delete method
 * should be called by the runner taking ownership of the portfolio.  It
 * is expected that specific portfolio implementations will extend this
 * base structure and implement the defined methods.
 */
typedef struct WU_Portfolio_ {
    void (*update)(struct WU_Portfolio_* portfolio, const WU_Signal* signals);
    double (*value)(const struct WU_Portfolio_* portfolio);
    double (*pnl)(const struct WU_Portfolio_* portfolio);
    void (*delete)(struct WU_Portfolio_* portfolio);
}* WU_Portfolio;

#define wu_portfolio_update(portfolio, signals) do { \
    ((WU_Portfolio)(portfolio))->update((WU_Portfolio)(portfolio), (signals)); \
} while(0)

#define wu_portfolio_value(portfolio) (((WU_Portfolio)(portfolio))->value( \
            (WU_Portfolio)(portfolio)))

#define wu_portfolio_pnl(portfolio) (((WU_Portfolio)(portfolio))->pnl( \
            (WU_Portfolio)(portfolio)))

#define wu_portfolio_delete(portfolio) do { \
    if ((portfolio)->delete) \
        (portfolio)->delete((WU_Portfolio)(portfolio)); \
} while(0)

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

/**
 * WU_PositionSizingParams - Position sizing configuration
 * 
 * Controls how much capital to allocate to each trade:
 * 
 * 1. WU_POSITION_SIZE_ABS: Absolute quantity
 *    - size_value = quantity to buy (e.g., 100 shares)
 *    Example: {.size_type = WU_POSITION_SIZE_ABS, .size_value = 100.0}
 * 
 * 2. WU_POSITION_SIZE_PCT: Percentage of available cash
 *    - size_value = percentage (0.0 to 1.0)
 *    - Uses available cash at time of signal
 *    Example: {.size_type = WU_POSITION_SIZE_PCT, .size_value = 0.5}  // Use 50% of cash
 * 
 * 3. WU_POSITION_SIZE_PCT_EQUAL: Equal distribution among all assets
 *    - size_value = allocation multiplier (0.0 to 1.0)
 *    - Divides total portfolio value equally by num_assets
 *    - Useful for multi-asset portfolios where each asset should maintain equal weight
 *    - Example with 3-asset portfolio: each asset targets 33.3% of portfolio value
 *    Example: {.size_type = WU_POSITION_SIZE_PCT_EQUAL, .size_value = 0.95}  // Use 95% of target allocation
 * 
 * 4. WU_POSITION_SIZE_STRATEGY_GUIDED: Strategy specifies target proportion
 *    - size_value = allocation multiplier (0.0 to 1.0)
 *    - Strategy specifies target proportion via signal.quantity (0.0 to 1.0)
 *    - signal.quantity = target proportion of total portfolio value for this asset
 *    - Allows dynamic, strategy-driven position sizing (e.g., risk parity, momentum-weighted)
 *    - Example: signal.quantity = 0.4 means this asset should be 40% of portfolio value
 *    Example: {.size_type = WU_POSITION_SIZE_STRATEGY_GUIDED, .size_value = 1.0}
 * 
 *    Strategy-guided example for 3-asset portfolio with $100k:
 *      Signal 0 (SPY): quantity=0.5  → allocate $50k (50% of portfolio)
 *      Signal 1 (QQQ): quantity=0.3  → allocate $30k (30% of portfolio)
 *      Signal 2 (TLT): quantity=0.2  → allocate $20k (20% of portfolio)
 * 
 * Note: WU_POSITION_SIZE_PCT_EQUAL is ideal for balanced portfolios.
 * WU_POSITION_SIZE_STRATEGY_GUIDED is ideal for sophisticated strategies that
 * need dynamic allocation based on market conditions, volatility, or other factors.
 */
typedef struct WU_PositionSizingParams {
    WU_PositionSizeType size_type;
    double size_value;
} WU_PositionSizingParams;

/**
 * WU_PortfolioParams - Portfolio configuration parameters
 * 
 * These parameters configure transaction costs, risk management, and position sizing
 * for portfolios (both single-asset and multi-asset).
 */
typedef struct WU_PortfolioParams {
    double initial_cash;
    double tx_cost_pct;
    double stop_loss_pct;
    double take_profit_pct;
    double slippage_pct;
    WU_PositionSizingParams position_sizing;
} WU_PortfolioParams;

/**
 * WU_BasicPortfolio - Unified portfolio for single-asset and multi-asset portfolios
 * 
 * This is the RECOMMENDED portfolio type for all new code. It manages positions
 * across one or more assets with a shared cash pool.
 * 
 * Key features:
 * - Unified interface (works for 1+ assets)
 * - Portfolio knows its own num_assets (no need to pass count to update())
 * - Index-based asset identification (no string lookups at runtime)
 * - Shared cash pool across all assets
 * - Same position sizing policy for all assets
 * - Heap-allocated positions array (dynamic size based on num_assets)
 * - Each PositionVector includes symbol and last_price
 * 
 * Single-asset example:
 *   WU_AssetSymbol symbols[] = {"SPY"};
 *   WU_BasicPortfolio portfolio = wu_basic_portfolio_new(params, symbols, 1);
 *   
 *   WU_Signal signals[1];
 *   signals[0] = wu_signal_init(ts, WU_SIDE_BUY, 450.0, 10.0);
 *   wu_portfolio_update(portfolio, signals);
 * 
 * Multi-asset example:
 *   WU_AssetSymbol symbols[] = {"SPY", "QQQ", "TLT"};
 *   WU_BasicPortfolio portfolio = wu_basic_portfolio_new(params, symbols, 3);
 *   
 *   WU_Signal signals[3];
 *   signals[0] = wu_signal_init(ts, WU_SIDE_BUY, 450.0, 10.0);  // SPY
 *   signals[1] = wu_signal_init(ts, WU_SIDE_HOLD, 375.0, 0.0);  // QQQ
 *   signals[2] = wu_signal_init(ts, WU_SIDE_SELL, 102.0, 5.0);  // TLT
 *   wu_portfolio_update(portfolio, signals);
 */

typedef struct WU_BasicPortfolio_ {
    struct WU_Portfolio_ base;
    WU_PortfolioParams params;
    double cash;
    WU_PositionVector** positions;
    int num_assets;
    double accum_expenses;
    WU_PortfolioStats stats;
}* WU_BasicPortfolio;

/**
 * Creates a basic portfolio (RECOMMENDED for all new code).
 * 
 * @param params Portfolio parameters (tx costs, slippage, position sizing)
 * @param symbols Array of asset symbols (e.g., {"SPY", "TLT"})
 * @param num_assets Number of assets to track
 * @return New portfolio instance
 * 
 * This unified constructor handles both single-asset (num_assets=1) and
 * multi-asset portfolios. The portfolio remembers num_assets internally,
 * so you don't need to pass it to update().
 * 
 * Memory is dynamically allocated based on num_assets (no fixed limits).
 */
WU_BasicPortfolio wu_basic_portfolio_new(
    WU_PortfolioParams params,
    const WU_AssetSymbol* symbols,
    int num_assets
);

/**
 * Get current value of a specific asset's positions
 * 
 * @param portfolio The portfolio
 * @param asset_index Index of the asset (0 to num_assets-1)
 * @return Current value of positions for that asset
 */
double wu_basic_portfolio_asset_value(
    WU_BasicPortfolio portfolio,
    int asset_index
);

/**
 * Get total quantity held for a specific asset
 * 
 * @param portfolio The portfolio
 * @param asset_index Index of the asset (0 to num_assets-1)
 * @return Total quantity held
 */
double wu_basic_portfolio_asset_quantity(
    WU_BasicPortfolio portfolio,
    int asset_index
);

#define WU_PORTFOLIO(portfolio) ((WU_Portfolio)(portfolio))

#endif // WU_PORTFOLIO_H
