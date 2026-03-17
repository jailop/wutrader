#ifndef WU_PORTFOLIO_H
#define WU_PORTFOLIO_H

#include "types.h"
#include "data.h"
#include "positions.h"
#include "stats.h"

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

/**
 * The update macro allows you to update the portfolio with new signals.
 */
#define wu_portfolio_update(portfolio, signals) do { \
    ((WU_Portfolio)(portfolio))->update((WU_Portfolio)(portfolio), \
        (signals)); \
} while(0)

/**
 * Calculates the current value of the portfolio. This includes the
 * value of all positions plus any remaining cash.
 */
#define wu_portfolio_value(portfolio) (((WU_Portfolio)(portfolio))->value( \
            (WU_Portfolio)(portfolio)))

/**
 * Calculates the profit and loss (PnL) of the portfolio. This is typically
 * calculated as the difference between the current value of the portfolio and
 * the initial cash invested, minus any transaction costs or fees.
 */
#define wu_portfolio_pnl(portfolio) (((WU_Portfolio)(portfolio))->pnl( \
            (WU_Portfolio)(portfolio)))

/**
 * Frees the resources associated with the portfolio. Usually this macro
 * is called by the runner when it takes ownership of the portfolio.
 */
#define wu_portfolio_delete(portfolio) do { \
    if ((portfolio)->delete) \
        (portfolio)->delete((WU_Portfolio)(portfolio)); \
} while(0)

typedef enum {
    WU_DIRECTION_LONG,
    WU_DIRECTION_SHORT,
    WU_DIRECTION_BOTH
} WU_Direction;

typedef enum {
    WU_EXECUTION_POLICY_IMMEDIATE,
    WU_EXECUTION_POLICY_NEXT_CLOSE,
    WU_EXECUTION_POLICY_FIXED_SLIPPAGE,
    WU_EXECUTION_POLICY_RANDOM_SLIPPAGE
} WU_ExecutionPolicyValue;

typedef enum {
    WU_TRANSACTION_COST_FIXED,
    WU_TRANSACTION_COST_PROPORTIONAL
} WU_TransactionCostType;

typedef struct {
    WU_ExecutionPolicyValue policy;
    double execution_mean;
    double execution_stddev;
    WU_TransactionCostType tx_cost_type;
    double tx_cost_value;
    double stop_loss_pct;
    double take_profit_pct;
} WU_ExecutionPolicy;

typedef struct {
    double rate;
    double limit;
} WU_BorrowParams;

/**
 * The parameters to define a portfolio.
 */
typedef struct WU_PortfolioParams {
    WU_Direction direction;
    double initial_cash;
    WU_ExecutionPolicy execution_policy;
    WU_BorrowParams borrow_params;
    WU_PositionSizingParams position_sizing;
} WU_PortfolioParams;

/**
 * A basic portfolio implementation that supports multiple assets. It
 * tracks cash, positions for each asset, and calculates portfolio value
 * and PnL based on the current market prices and the positions held. It
 * also keeps track of accumulated transaction costs and trading
 * statistics. The portfolio is initialized with a set of parameters and
 * an array of asset symbols, and it dynamically manages the positions
 * for each asset.
 */

typedef struct WU_BasicPortfolio_ {
    struct WU_Portfolio_ base;
    double cash;
    int num_assets;
    WU_TimeStamp last_update_time;
    WU_PortfolioParams params;
    WU_PositionVector** positions;
    WU_PortfolioStats stats;
    WU_Signal* pending_orders;
}* WU_BasicPortfolio;

/**
 * Returns default portfolio parameters with reasonable values.
 */
WU_PortfolioParams wu_portfolio_params_default(void);

/**
 * Creates a new basic portfolio instance with the specified parameters
 * and asset symbols. The array symbols should be created using the
 * `wu_symbol_list` macro, which ensures it is null-terminated.
 */
WU_BasicPortfolio wu_basic_portfolio_new(
    WU_PortfolioParams params,
    const char* symbols[]
);

/**
 * Get current value of a specific asset's positions
 */
double wu_basic_portfolio_asset_value(
    WU_BasicPortfolio portfolio,
    int asset_index
);

/**
 * Get total quantity held for a specific asset
 */
double wu_basic_portfolio_asset_quantity(
    WU_BasicPortfolio portfolio,
    int asset_index
);

/**
 * Casting macro for the base portfolio type.
 */
#define WU_PORTFOLIO(portfolio) ((WU_Portfolio)(portfolio))

#endif // WU_PORTFOLIO_H
