/**
 * This file implements a basic portfolio management system that
 * supports multiple assets with a shared cash pool. The portfolio can
 * process buy and sell signals, manage positions for each asset, and
 * calculate the total value and profit/loss (PnL) of the portfolio. It
 * also includes features such as transaction costs, slippage,
 * stop-loss, and take-profit mechanisms.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "wu.h"

/**
 * Returns default portfolio parameters with reasonable values.
 */
WU_PortfolioParams wu_portfolio_params_default(void) {
    return (WU_PortfolioParams) {
        .direction = WU_DIRECTION_LONG,
        .initial_cash = 100000.0,
        .tx_cost_pct = 0.001,
        .stop_loss_pct = 0.0,
        .take_profit_pct = 0.0,
        .slippage_pct = 0.0,
        .borrow_rate = 0.0,
        .borrow_limit = 0.0,
        .position_sizing = {
            .size_type = WU_POSITION_SIZE_PCT,
            .size_value = 0.5
        }
    };
}

/**
 * Helper function to calculate the time difference in years between
 * two timestamps, accounting for different time units.
 */
static double calculate_years_held(WU_TimeStamp open_time, 
        WU_TimeStamp close_time) {
    if (open_time.units != close_time.units) return 0.0;
    int64_t diff = close_time.mark - open_time.mark;
    double seconds;
    switch (open_time.units) {
        case WU_TIME_UNIT_SECONDS:
            seconds = (double)diff;
            break;
        case WU_TIME_UNIT_MILLIS:
            seconds = (double)diff / 1000.0;
            break;
        case WU_TIME_UNIT_MICROS:
            seconds = (double)diff / 1000000.0;
            break;
        case WU_TIME_UNIT_NANOS:
            seconds = (double)diff / 1000000000.0;
            break;
        default:
            seconds = 0.0;
    }
    return seconds / (365.0 * 86400.0);
}

/**
 * At this moment, the slippage is calculated in a deterministic way
 * based on a fixed percentage defined in the portfolio parameters.
 * If the signal is a buy, the slippage increases the price, and if it's
 * a sell, the slippage decreases the price. In the future, this can be
 * enhanced to support more realistic slippage models.
 */
static inline
double slippage_price(double price, double slippage_pct, bool is_buy) {
    return price * (1.0 + (is_buy ? slippage_pct : -slippage_pct));
}

/**
 * Helper function to calculate the total value of all positions
 * in a position vector at a given price.
 */
static double calculate_positions_value(WU_PositionVector* vec, double price) {
    double value = 0.0;
    for (int i = 0; i < vec->capacity; i++) {
        bool found = false;
        struct WU_Position_ pos = wu_position_get(vec, i, &found);
        if (found) {
            value += pos.quantity * price;
        }
    }
    return value;
}

/**
 * To calculate the total value of the portfolio, an iterative process
 * is used to sum the value of each position across all assets, based on
 * the last known price for each asset. The cash balance is then added
 * to this total to get the overall portfolio value. This function has a
 * complexity of O(N*M) where N is the number of assets and M is the
 * average number of positions per asset. Although the performance can
 * be an issue, it provides an accurate valuation of the portfolio at a
 * given point in time. In the future, it can consider caching the total
 * value and only recalculating when there are changes to positions or
 * prices, to improve performance. Additionally, it can be optimized by
 * maintaining a running total of the portfolio value that gets updated
 * when positions modified.
 */
static double calculate_portfolio_value(const struct WU_Portfolio_ *portfolio)
{
    const WU_BasicPortfolio p = (const WU_BasicPortfolio) portfolio;
    double position_value = 0.0;
    for (int asset_idx = 0; asset_idx < p->num_assets; asset_idx++) {
        WU_PositionVector* vec = p->positions[asset_idx];
        position_value += calculate_positions_value(vec, vec->last_price);
    }
    return p->cash + position_value;
}

/**
 * This function calculates the position size for a given signal based
 * on the portfolio's position sizing parameters. It supports multiple
 * sizing strategies, including absolute size, percentage of cash, equal
 * percentage of portfolio, and strategy-guided sizing. The function
 * takes into account the current price of the asset and the available
 * cash in the portfolio to determine the appropriate quantity to buy or
 * sell. It ensures that the calculated position size does not exceed
 * the available cash for buy signals and can be adjusted based on the
 * signal's quantity for strategy-guided sizing.
 */
static double calculate_position_size(WU_BasicPortfolio portfolio,
        WU_Signal signal) {
    struct WU_PositionSizingParams* ps = &portfolio->params.position_sizing;
    double price = signal.price;
    switch (ps->size_type) {
        case WU_POSITION_SIZE_ABS:
            return ps->size_value;
        case WU_POSITION_SIZE_PCT: {
            double portfolio_value = calculate_portfolio_value(
                    (WU_Portfolio) portfolio);
            // If signal.quantity is between 0 and 1, treat as target allocation %
            // Otherwise use size_value as % of available cash
            if (signal.quantity > 0.0 && signal.quantity < 1.0) {
                double target_proportion = signal.quantity;
                return portfolio_value * target_proportion * ps->size_value / price;
            } else {
                return (portfolio->cash * ps->size_value) / price;
            }
        }
        case WU_POSITION_SIZE_PCT_EQUAL: {
            double portfolio_value = calculate_portfolio_value(
                    (WU_Portfolio) portfolio);
            double alloc_per_asset = portfolio_value / portfolio->num_assets;
            double target_value = alloc_per_asset * ps->size_value;
            return target_value / price;
        }
        case WU_POSITION_SIZE_STRATEGY_GUIDED: {
            double portfolio_value = calculate_portfolio_value(
                    (WU_Portfolio) portfolio);
            double target_proportion = signal.quantity;
            if (target_proportion < 0.0) target_proportion = 0.0;
            if (target_proportion > 1.0) target_proportion = 1.0;
            return portfolio_value * target_proportion * ps->size_value
                / price;
        }
        default: // unreachable
            return 0.0;
    }
}

/**
 * An auxiliary struct to hold the result of setting a buy position and 
 * calculating the total cost.
 */
struct BuyPositionResult {
    struct WU_Position_ position;
    double total_cost;
    double tx_cost;
};

/**
 * This function computes the total cost of a trade by calculating the
 * cost of the quantity at the given price and then applying the
 * transaction cost percentage to determine the additional cost incurred
 * from the trade.
 */
static double compute_total_cost(double quantity, double price,
        double tx_cost_pct) {
    double cost = quantity * price;
    double tx_cost = cost * tx_cost_pct;
    return cost + tx_cost;
}

/**
 * This function calculates the position size for a buy signal, applies
 * slippage to determine the effective price, and computes the total cost
 * of the purchase including transaction costs. It also checks if the
 * total cost exceeds the available cash in the portfolio, and if so,
 * it adjusts the quantity to fit within the cash constraints. The result
 * includes the position details (timestamp, quantity, price) and the
 * total cost and transaction cost for the trade.
 */
static struct BuyPositionResult set_buy_position(WU_BasicPortfolio portfolio,
        WU_Signal signal) {
    double quantity = calculate_position_size(portfolio, signal);
    double slippage = slippage_price(signal.price,
            portfolio->params.slippage_pct, true);
    double total_cost = compute_total_cost(quantity, slippage,
            portfolio->params.tx_cost_pct);
    if (total_cost > portfolio->cash) {
        quantity = portfolio->cash / (slippage
                * (1.0 + portfolio->params.tx_cost_pct));
        total_cost = compute_total_cost(quantity, slippage,
                portfolio->params.tx_cost_pct);
    }
    return (struct BuyPositionResult) {
        .position = {
            .timestamp = signal.timestamp,
            .quantity = quantity,
            .price = slippage,
            .active = true
        },
        .total_cost = total_cost,
        .tx_cost = total_cost - (quantity * slippage)
    };
}

/**
 * This function calculates the total cost basis for all active
 * positions in a given position vector. It iterates through each
 * position, checks if it is active, and if so, it multiplies the
 * quantity by the price to get the cost for that position, and sums it
 * up to get the total cost basis. This total cost basis is used when
 * closing positions to determine the profit and loss (PnL) from the
 * trade, as it represents the total amount invested in those positions.
 */
static double calculate_total_cost_basis(WU_PositionVector* vec) {
    double total_cost = 0.0;
    for (int i = 0; i < vec->capacity; i++) {
        bool found = false;
        struct WU_Position_ pos = wu_position_get(vec, i, &found);
        if (found) {
            total_cost += pos.quantity * pos.price;
        }
    }
    return total_cost;
}

/**
 * This function executes a buy signal. For portfolios without positions or
 * with long positions, it creates new long positions. When there are short
 * positions (negative quantity), it closes them (buy to cover). It updates
 * the portfolio's cash balance and tracks transaction fees in stats.
 */
static void execute_buy(WU_BasicPortfolio portfolio, WU_Signal signal,
        int asset_index) {
    if (asset_index < 0 || asset_index >= portfolio->num_assets) return;
    if (portfolio->params.direction == WU_DIRECTION_SHORT) return;
    
    WU_PositionVector* vec = portfolio->positions[asset_index];
    double total_quantity = wu_position_total_quantity(vec);
    
    if (total_quantity < 0) {
        // Closing short positions (buying to cover)
        double abs_quantity = -total_quantity;
        double total_cost = calculate_total_cost_basis(vec);
        double abs_cost = -total_cost;
        double buy_price = slippage_price(signal.price,
                portfolio->params.slippage_pct, true);
        double cost_to_close = abs_quantity * buy_price;
        double tx_cost = cost_to_close * portfolio->params.tx_cost_pct;
        double pnl = abs_cost - cost_to_close - tx_cost;
        portfolio->cash -= cost_to_close + tx_cost;
        portfolio->stats->accum_tx_fees += tx_cost;
        wu_portfolio_stats_record_trade(portfolio->stats, pnl, WU_CLOSE_REASON_SIGNAL);
        wu_position_clear(vec);
    }
    
    // Check borrow limit
    if (portfolio->cash < -portfolio->params.borrow_limit) return;
    
    struct BuyPositionResult result = set_buy_position(portfolio, signal);
    if (result.position.quantity <= 0) return;
    wu_position_add(portfolio->positions[asset_index], &result.position);
    portfolio->cash -= result.total_cost;
    portfolio->stats->accum_tx_fees += result.tx_cost;
}

/**
 * This function handles the logic for closing a position and updating
 * the portfolio's cash balance and trading statistics. It calculates
 * the proceeds from the sale, the transaction cost based on the defined
 * transaction cost percentage, and the profit and loss (PnL) from the
 * trade. The portfolio's cash balance is updated by adding the proceeds
 * and subtracting the transaction cost, and fees are tracked in stats.
 */
static void close_and_update_portfolio(WU_BasicPortfolio portfolio,
        double quantity, double cost_basis, double sell_price,
        WU_CloseReason reason) {
    double proceeds = quantity * sell_price;
    double tx_cost = proceeds * portfolio->params.tx_cost_pct;
    double pnl = proceeds - cost_basis - tx_cost;
    portfolio->cash += proceeds - tx_cost;
    portfolio->stats->accum_tx_fees += tx_cost;
    wu_portfolio_stats_record_trade(portfolio->stats, pnl, reason);
}

/**
 * This function executes a sell signal. For long positions, it closes
 * existing positions. For short and both-direction portfolios, it can
 * initiate a short sale even without existing positions (borrowing assets).
 */
static void execute_sell(WU_BasicPortfolio portfolio, WU_Signal signal,
        int asset_index) {
    if (asset_index < 0 || asset_index >= portfolio->num_assets) return;
    
    WU_PositionVector* vec = portfolio->positions[asset_index];
    double total_quantity = wu_position_total_quantity(vec);
    bool can_short = (portfolio->params.direction == WU_DIRECTION_SHORT ||
                      portfolio->params.direction == WU_DIRECTION_BOTH);
    
    if (total_quantity > 0) {
        // Closing long positions
        double total_cost = calculate_total_cost_basis(vec);
        double sell_price = slippage_price(signal.price,
                portfolio->params.slippage_pct, false);
        close_and_update_portfolio(portfolio, total_quantity, total_cost,
                sell_price, WU_CLOSE_REASON_SIGNAL);
        wu_position_clear(vec);
    } else if (total_quantity < 0) {
        // Closing short positions (buying to cover)
        double abs_quantity = -total_quantity;
        double total_cost = calculate_total_cost_basis(vec);
        double abs_cost = -total_cost;
        double buy_price = slippage_price(signal.price,
                portfolio->params.slippage_pct, true);
        double cost_to_close = abs_quantity * buy_price;
        double tx_cost = cost_to_close * portfolio->params.tx_cost_pct;
        double pnl = abs_cost - cost_to_close - tx_cost;
        portfolio->cash -= cost_to_close + tx_cost;
        portfolio->stats->accum_tx_fees += tx_cost;
        wu_portfolio_stats_record_trade(portfolio->stats, pnl, WU_CLOSE_REASON_SIGNAL);
        wu_position_clear(vec);
    }
    
    if (can_short && total_quantity == 0) {
        // Calculate total current short exposure
        double total_short_value = 0.0;
        for (int i = 0; i < portfolio->num_assets; i++) {
            double qty = wu_position_total_quantity(portfolio->positions[i]);
            if (qty < 0) {
                total_short_value += (-qty) * portfolio->positions[i]->last_price;
            }
        }
        
        // Check borrow limit
        double quantity = calculate_position_size(portfolio, signal);
        double slippage = slippage_price(signal.price,
                portfolio->params.slippage_pct, false);
        double new_short_value = quantity * slippage;
        
        if (total_short_value + new_short_value > portfolio->params.borrow_limit) return;
        
        double proceeds = new_short_value;
        double tx_cost = proceeds * portfolio->params.tx_cost_pct;
        
        struct WU_Position_ short_pos = {
            .timestamp = signal.timestamp,
            .quantity = -quantity,
            .price = slippage,
            .active = true
        };
        wu_position_add(vec, &short_pos);
        portfolio->cash += proceeds - tx_cost;
        portfolio->stats->accum_tx_fees += tx_cost;
    }
}

/**
 * This function checks if a position should be closed based on the
 * current price and the portfolio's stop-loss and take-profit
 * thresholds. For short positions (negative quantity), the PnL logic
 * is inverted.
 */
static bool close_position(WU_BasicPortfolio portfolio,
        struct WU_Position_ pos, double current_price) {
    bool is_short = (pos.quantity < 0);
    double pnl_pct = is_short ? 
        (pos.price - current_price) / pos.price :
        (current_price - pos.price) / pos.price;
    bool should_close = false;
    WU_CloseReason reason = WU_CLOSE_REASON_SIGNAL;
    if (portfolio->params.stop_loss_pct > 0 && 
        pnl_pct <= -portfolio->params.stop_loss_pct) {
        should_close = true;
        reason = WU_CLOSE_REASON_STOP_LOSS;
    }
    if (portfolio->params.take_profit_pct > 0 && 
        pnl_pct >= portfolio->params.take_profit_pct) {
        should_close = true;
        reason = WU_CLOSE_REASON_TAKE_PROFIT;
    }
    if (!should_close) return false;
    
    double abs_quantity = is_short ? -pos.quantity : pos.quantity;
    double price = is_short ? 
        slippage_price(current_price, portfolio->params.slippage_pct, true) :
        slippage_price(current_price, portfolio->params.slippage_pct, false);
    double cost_basis = abs_quantity * pos.price;
    close_and_update_portfolio(portfolio, abs_quantity, cost_basis,
            price, reason);
    return true;
}

/**
 * This function iterates through all active positions for a given asset
 * and checks if any of them can be closed based on the current price
 * and the portfolio's stop-loss and take-profit thresholds. If a
 * position is closed, then it is removed from the position vector.
 */
static void check_and_close_positions(WU_BasicPortfolio portfolio, 
        int asset_index, double current_price) {
    assert(asset_index >= 0 && asset_index < portfolio->num_assets);
    WU_PositionVector* vec = portfolio->positions[asset_index];
    if (vec->count == 0) return;
    for (int i = 0; i < vec->capacity; i++) {
        bool found = false;
        struct WU_Position_ pos = wu_position_get(vec, i, &found);
        if (!found) continue;
        if (close_position(portfolio, pos, current_price))
            wu_position_remove(vec, i);
    }
}

/**
 * The profit and loss (PnL) of the portfolio is calculated by taking
 * the current total value of the portfolio (cash + value of all
 * positions) and subtracting the initial cash invested. This provides a
 * measure of the overall performance of the portfolio since inception.
 */
static double calculate_portfolio_pnl(const struct WU_Portfolio_ *portfolio) {
    WU_BasicPortfolio p = (WU_BasicPortfolio) portfolio;
    double current_value = calculate_portfolio_value(portfolio);
    return current_value - p->params.initial_cash;
}

/**
 * Helper function to calculate total value of all short positions across
 * all assets in the portfolio.
 */
static double calculate_total_short_value(WU_BasicPortfolio p) {
    double total_short_value = 0.0;
    for (int i = 0; i < p->num_assets; i++) {
        double qty = wu_position_total_quantity(p->positions[i]);
        if (qty < 0) {
            total_short_value += (-qty) * p->positions[i]->last_price;
        }
    }
    return total_short_value;
}

/**
 * Helper function to update last prices, apply borrow interest on short
 * positions, check exits, and update stats with current portfolio state.
 */
static void update_prices_and_check_exits(WU_BasicPortfolio p,
        const WU_Signal* signals, int count) {
    // Get current timestamp from first valid signal
    bool has_valid_signal = false;
    WU_TimeStamp current_time;
    for (int i = 0; i < count && !has_valid_signal; i++) {
        if (wu_signal_validate(&signals[i])) {
            current_time = signals[i].timestamp;
            has_valid_signal = true;
        }
    }
    
    if (!has_valid_signal) return;
    
    // Apply borrow interest if we have short positions
    double total_short_value = calculate_total_short_value(p);
    if (total_short_value > 0.0 && p->params.borrow_rate > 0.0 && 
        p->last_update_time.mark != 0) {
        double years_elapsed = calculate_years_held(p->last_update_time, current_time);
        double borrow_interest = total_short_value * p->params.borrow_rate * years_elapsed;
        p->cash -= borrow_interest;
        p->stats->accum_borrow_interest += borrow_interest;
    }
    
    p->last_update_time = current_time;
    
    // Update prices and check exits
    for (int i = 0; i < count; i++) {
        WU_Signal signal = signals[i];
        if (!wu_signal_validate(&signal)) continue;
        p->positions[i]->last_price = signal.price;
        check_and_close_positions(p, i, signal.price);
    }
    
    // Update stats with current portfolio state
    double pf_value = wu_portfolio_value((WU_Portfolio)p);
    wu_portfolio_stats_update(p->stats, p->cash, pf_value, current_time);
    
    // Update position info in stats
    for (int i = 0; i < p->num_assets; i++) {
        double qty = wu_position_total_quantity(p->positions[i]);
        double value = wu_basic_portfolio_asset_value(p, i);
        wu_portfolio_stats_update_position(p->stats, i, 
            p->positions[i]->symbol, qty, value, p->positions[i]->last_price);
    }
}

/**
 * Helper function to process all sell signals. It iterates through the
 * signals, validates them, and if a signal is a sell signal, it calls
 * the execute_sell function to close the corresponding positions. This
 * function should be called first to ensure that all sell signals are
 * handled before processing buy signals, which is important for proper
 * cash management and to avoid situations where buy signals are ignored
 * due to insufficient cash after processing sell signals.
 */
static void process_sell_signals(WU_BasicPortfolio p,
        const WU_Signal* signals, int count) {
    for (int i = 0; i < count; i++) {
        WU_Signal signal = signals[i];
        if (!wu_signal_validate(&signal)) continue;
        if (signal.side == WU_SIDE_SELL) execute_sell(p, signal, i);
    }
}

/**
 * Helper function to count valid buy signals.
 */
static int count_buy_signals(const WU_Signal* signals, int count) {
    int buy_count = 0;
    for (int i = 0; i < count; i++) {
        if (wu_signal_validate(&signals[i]) 
                && signals[i].side == WU_SIDE_BUY) {
            buy_count++;
        }
    }
    return buy_count;
}

/**
 * Helper function to check if signals use strategy-guided allocation.
 */
static bool uses_strategy_guided_allocation(WU_BasicPortfolio p,
        const WU_Signal* signals, int count) {
    if (p->params.position_sizing.size_type == WU_POSITION_SIZE_PCT) {
        for (int i = 0; i < count; i++) {
            if (wu_signal_validate(&signals[i]) && signals[i].side == WU_SIDE_BUY) {
                if (signals[i].quantity > 0.0 && signals[i].quantity < 1.0) {
                    return true;
                }
            }
        }
    }
    return (p->params.position_sizing.size_type == WU_POSITION_SIZE_STRATEGY_GUIDED ||
            p->params.position_sizing.size_type == WU_POSITION_SIZE_PCT_EQUAL);
}

/**
 * Helper function to process all buy signals. If cash splitting is
 * enabled, it divides the available cash equally among the buy signals
 * to ensure that the total allocated cash does not exceed the
 * portfolio's cash balance. Each buy signal is processed sequentially,
 * and the cash balance is updated after each purchase to reflect the
 * remaining cash for subsequent buys. If cash splitting is not enabled,
 * each buy signal is processed with the full available cash, which may
 * lead to some buy signals being ignored if the cash runs out. This
 * function ensures that the portfolio's cash management is consistent
 * with the defined position sizing strategy and prevents
 * over-allocation of funds when multiple buy signals are present.
 */
static void process_buy_signals(WU_BasicPortfolio p,
        const WU_Signal* signals, int count, int buy_count) {
    bool use_cash_splitting = !uses_strategy_guided_allocation(p, signals, count);
    double cash_per_signal = use_cash_splitting
            ? (p->cash / buy_count) : 0.0;
    for (int i = 0; i < count; i++) {
        WU_Signal signal = signals[i];
        if (!wu_signal_validate(&signal)) continue;
        if (signal.side != WU_SIDE_BUY) continue;
        if (use_cash_splitting) {
            double original_cash = p->cash;
            p->cash = cash_per_signal;
            execute_buy(p, signal, i);
            double cash_used = cash_per_signal - p->cash;
            p->cash = original_cash - cash_used;
        } else {
            execute_buy(p, signal, i);
        }
    }
}

/**
 * The update_portfolio function is the core of the portfolio management
 * logic. It processes incoming signals to update the portfolio's state.
 * The function first updates the last known prices for each asset and
 * checks for any positions that need to be closed due to stop-loss or
 * take-profit conditions. It then processes all sell signals to close
 * positions as needed, and finally processes buy signals to open new
 * positions.
 */
static void update_portfolio(WU_Portfolio portfolio,
        const WU_Signal* signals) {
    WU_BasicPortfolio p = (WU_BasicPortfolio) portfolio;
    int count = p->num_assets;
    update_prices_and_check_exits(p, signals, count);
    process_sell_signals(p, signals, count);
    int buy_count = count_buy_signals(signals, count);
    if (buy_count > 0) {
        process_buy_signals(p, signals, count, buy_count);
    }
}

/**
 * This function is responsible for freeing all memory associated with the
 * portfolio. It iterates through each asset's position vector and deletes
 * it, then frees the array of position vectors, deletes the portfolio
 * statistics, and finally frees the portfolio structure itself. This ensures
 * that all resources are properly released when the portfolio is no longer
 * needed, preventing memory leaks.
 */
static void wu_basic_portfolio_free(struct WU_Portfolio_* portfolio) {
    WU_BasicPortfolio p = (WU_BasicPortfolio) portfolio;
    if (!p) return;
    for (int i = 0; i < p->num_assets; i++) {
        wu_position_vector_delete(p->positions[i]);
    }
    free(p->positions);
    wu_portfolio_stats_delete(p->stats);
    free(portfolio);
}

/**
 * Helper function to initialize position vectors for all assets.
 * Allocates memory and creates position vectors. Returns NULL on failure.
 */
static WU_PositionVector** initialize_positions(const char* symbols[],
        int num_assets) {
    WU_PositionVector** positions = malloc(num_assets
            * sizeof(WU_PositionVector*));
    if (!positions) return NULL;
    for (int i = 0; i < num_assets; i++) {
        positions[i] = wu_position_vector_new(symbols[i]);
        if (!positions[i]) {
            for (int j = 0; j < i; j++) {
                wu_position_vector_delete(positions[j]);
            }
            free(positions);
            return NULL;
        }
    }
    return positions;
}

/**
 * This constructor function creates a new basic portfolio instance. It
 * takes in the portfolio parameters and an array of asset symbols, and
 * initializes the portfolio's internal state accordingly. Memory is
 * dynamically allocated for the portfolio structure and the position
 * vectors for each asset. The function also sets up the function pointers
 * for the portfolio operations (update, value, pnl, delete) and initializes
 * the cash balance and trading statistics. If any memory allocation fails,
 * it ensures that all previously allocated resources are freed to prevent
 * memory leaks.
 */
WU_BasicPortfolio wu_basic_portfolio_new(WU_PortfolioParams params,
        const char* symbols[]) {
    WU_BasicPortfolio portfolio = malloc(sizeof(struct WU_BasicPortfolio_));
    if (!portfolio) return NULL;
    int num_assets;
    for (num_assets = 0; symbols[num_assets] != NULL; num_assets++);
    portfolio->positions = initialize_positions(symbols, num_assets);
    if (!portfolio->positions) {
        free(portfolio);
        return NULL;
    }
    portfolio->base = (struct WU_Portfolio_){
        .update = update_portfolio,
        .value = calculate_portfolio_value,
        .pnl = calculate_portfolio_pnl,
        .delete = wu_basic_portfolio_free
    };
    portfolio->params = params;
    portfolio->cash = params.initial_cash;
    portfolio->num_assets = num_assets;
    portfolio->last_update_time = (WU_TimeStamp){.mark = 0, .units = WU_TIME_UNIT_SECONDS};
    portfolio->stats = wu_portfolio_stats_new(params.initial_cash);
    
    // Initialize stats with asset symbols
    for (int i = 0; i < num_assets; i++) {
        wu_portfolio_stats_update_position(portfolio->stats, i,
            portfolio->positions[i]->symbol, 0.0, 0.0, 0.0);
    }
    
    return portfolio;
}

/**
 * The value of a specific asset's positions is calculated by summing
 * the value of each active position. This function does bound checking.
 */
double wu_basic_portfolio_asset_value(WU_BasicPortfolio portfolio,
        int asset_index) {
    if (!portfolio || asset_index < 0 
            || asset_index >= portfolio->num_assets) {
        return 0.0;
    }
    WU_PositionVector* vec = portfolio->positions[asset_index];
    return calculate_positions_value(vec, vec->last_price);
}

/**
 * Computes the total quantity held for a specific asset by summing the
 * quantities of all active positions in the corresponding position
 * vector. It does bound checking.
 */
double wu_basic_portfolio_asset_quantity(WU_BasicPortfolio portfolio,
        int asset_index) {
    if (!portfolio || asset_index < 0 
            || asset_index >= portfolio->num_assets) {
        return 0.0;
    }
    return wu_position_total_quantity(portfolio->positions[asset_index]);
}
