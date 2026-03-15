#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "wu.h"

static double calculate_portfolio_value(const struct WU_Portfolio_ *portfolio)
{
    const WU_BasicPortfolio p = (const WU_BasicPortfolio) portfolio;
    double position_value = 0.0;
    for (int asset_idx = 0; asset_idx < p->num_assets; asset_idx++) {
        WU_PositionVector* vec = p->positions[asset_idx];
        double last_price = vec->last_price;
        for (int i = 0; i < vec->capacity; i++) {
            bool found = false;
            struct WU_Position_ pos = wu_position_get(vec, i, &found);
            if (found) {
                position_value += pos.quantity * last_price;
            }
        }
    }
    return p->cash + position_value;
}


static double calculate_position_size(WU_BasicPortfolio portfolio,
        WU_Signal signal) {
    struct WU_PositionSizingParams* ps = &portfolio->params.position_sizing;
    double price = signal.price;
    switch (ps->size_type) {
        case WU_POSITION_SIZE_ABS:
            return ps->size_value;
        case WU_POSITION_SIZE_PCT:
            return (portfolio->cash * ps->size_value) / price;
        case WU_POSITION_SIZE_PCT_EQUAL: 
            double portfolio_value = calculate_portfolio_value(
                    (WU_Portfolio) portfolio);
            double alloc_per_asset = portfolio_value / portfolio->num_assets;
            double target_value = alloc_per_asset * ps->size_value;
            return target_value / price;
        case WU_POSITION_SIZE_STRATEGY_GUIDED:
            portfolio_value = calculate_portfolio_value(
                    (WU_Portfolio) portfolio);
            double target_proportion = signal.quantity;
            if (target_proportion < 0.0) target_proportion = 0.0;
            if (target_proportion > 1.0) target_proportion = 1.0;
            return portfolio_value * target_proportion * ps->size_value
                / price;
        default: // unreachable
            return 0.0;
    }
}

static void execute_buy(WU_BasicPortfolio portfolio, WU_Signal signal,
        int asset_index) {
    if (asset_index < 0 || asset_index >= portfolio->num_assets) return;
    if (portfolio->cash <= 0) return;
    double quantity = calculate_position_size(portfolio, signal);
    double slippage_price = signal.price * (1.0
            + portfolio->params.slippage_pct);
    double cost = quantity * slippage_price;
    double tx_cost = cost * portfolio->params.tx_cost_pct;
    double total_cost = cost + tx_cost;
    if (total_cost > portfolio->cash) {
        quantity = portfolio->cash / (slippage_price
                * (1.0 + portfolio->params.tx_cost_pct));
        cost = quantity * slippage_price;
        tx_cost = cost * portfolio->params.tx_cost_pct;
        total_cost = cost + tx_cost;
    }
    if (quantity <= 0) return;
    struct WU_Position_ pos = {
        .timestamp = signal.timestamp,
        .quantity = quantity,
        .price = slippage_price,
        .active = true
    };
    wu_position_add(portfolio->positions[asset_index], &pos);
    portfolio->cash -= total_cost;
    portfolio->accum_expenses += tx_cost;
}

static void execute_sell(WU_BasicPortfolio portfolio, WU_Signal signal,
        int asset_index) {
    if (asset_index < 0 || asset_index >= portfolio->num_assets) return;
    WU_PositionVector* vec = portfolio->positions[asset_index];
    double total_quantity = wu_position_total_quantity(vec);
    if (total_quantity <= 0) return;
    double total_cost = 0;
    for (int i = 0; i < vec->capacity; i++) {
        bool found = false;
        struct WU_Position_ pos = wu_position_get(vec, i, &found);
        if (found) {
            total_cost += pos.quantity * pos.price;
        }
    }
    double slippage_price = signal.price * (1.0
            - portfolio->params.slippage_pct);
    double proceeds = total_quantity * slippage_price;
    double tx_cost = proceeds * portfolio->params.tx_cost_pct;
    double pnl = proceeds - total_cost - tx_cost;
    portfolio->cash += proceeds - tx_cost;
    portfolio->accum_expenses += tx_cost;
    wu_portfolio_stats_record_trade(portfolio->stats, pnl,
            WU_CLOSE_REASON_SIGNAL);
    wu_position_clear(vec);
}

static bool close_position(WU_BasicPortfolio portfolio,
        struct WU_Position_ pos, double current_price) {
    double pnl_pct = (current_price - pos.price) / pos.price;
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
    double slippage_price = current_price * (1.0
            - portfolio->params.slippage_pct);
    double proceeds = pos.quantity * slippage_price;
    double tx_cost = proceeds * portfolio->params.tx_cost_pct;
    double pnl = proceeds - (pos.quantity * pos.price) - tx_cost;
    portfolio->cash += proceeds - tx_cost;
    portfolio->accum_expenses += tx_cost;
    wu_portfolio_stats_record_trade(portfolio->stats, pnl, reason);
    return true;
}

static void check_positions(WU_BasicPortfolio portfolio, 
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

static double calculate_portfolio_pnl(const struct WU_Portfolio_ *portfolio) {
    WU_BasicPortfolio p = (WU_BasicPortfolio) portfolio;
    double current_value = calculate_portfolio_value(portfolio);
    return current_value - p->params.initial_cash;
}

static void update_portfolio(WU_Portfolio portfolio,
        const WU_Signal* signals) {
    WU_BasicPortfolio p = (WU_BasicPortfolio) portfolio;
    int count = p->num_assets;
    for (int i = 0; i < count; i++) {
        WU_Signal signal = signals[i];
        if (!wu_signal_validate(&signal)) {
            continue;  // Invalid signal, skip
        }
        p->positions[i]->last_price = signal.price;
        check_positions(p, i, signal.price);
    }
    for (int i = 0; i < count; i++) {
        WU_Signal signal = signals[i];
        if (!wu_signal_validate(&signal)) {
            continue;
        }
        if (signal.side == WU_SIDE_SELL) {
            execute_sell(p, signal, i);
        }
    }
    int buy_count = 0;
    for (int i = 0; i < count; i++) {
        WU_Signal signal = signals[i];
        if (wu_signal_validate(&signal) && signal.side == WU_SIDE_BUY) {
            buy_count++;
        }
    }
    if (buy_count > 0) {
        bool use_cash_splitting = (
                p->params.position_sizing.size_type 
                    != WU_POSITION_SIZE_PCT_EQUAL &&
                p->params.position_sizing.size_type
                    != WU_POSITION_SIZE_STRATEGY_GUIDED);
        double cash_per_signal = use_cash_splitting
                ? (p->cash / buy_count) : 0.0;
        for (int i = 0; i < count; i++) {
            WU_Signal signal = signals[i];
            if (!wu_signal_validate(&signal)) {
                continue;
            }
            if (signal.side == WU_SIDE_BUY) {
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
    }
}

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

WU_BasicPortfolio wu_basic_portfolio_new(WU_PortfolioParams params,
        const char* symbols[]) {
    WU_BasicPortfolio portfolio = malloc(sizeof(struct WU_BasicPortfolio_));
    if (!portfolio) return NULL;
    int num_assets;
    for (num_assets = 0; symbols[num_assets] != NULL; num_assets++);
    portfolio->positions = malloc(num_assets * sizeof(WU_PositionVector*));
    if (!portfolio->positions) {
        free(portfolio);
        return NULL;
    }
    portfolio->base.update = update_portfolio;
    portfolio->base.value = calculate_portfolio_value;
    portfolio->base.pnl = calculate_portfolio_pnl;
    portfolio->base.delete = wu_basic_portfolio_free;
    portfolio->params = params;
    portfolio->cash = params.initial_cash;
    portfolio->num_assets = num_assets;
    portfolio->accum_expenses = 0.0;
    portfolio->stats = wu_portfolio_stats_new();
    for (int i = 0; i < num_assets; i++) {
        portfolio->positions[i] = wu_position_vector_new(symbols[i]);
        if (!portfolio->positions[i]) {
            for (int j = 0; j < i; j++) {
                wu_position_vector_delete(portfolio->positions[j]);
            }
            free(portfolio->positions);
            wu_portfolio_stats_delete(portfolio->stats);
            free(portfolio);
            return NULL;
        }
    }
    return portfolio;
}

double wu_basic_portfolio_asset_value(WU_BasicPortfolio portfolio,
        int asset_index) {
    if (!portfolio || asset_index < 0 
            || asset_index >= portfolio->num_assets) {
        return 0.0;
    }
    double value = 0.0;
    WU_PositionVector* vec = portfolio->positions[asset_index];
    double last_price = vec->last_price;
    for (int i = 0; i < vec->capacity; i++) {
        bool found = false;
        struct WU_Position_ pos = wu_position_get(vec, i, &found);
        if (found) {
            value += pos.quantity * last_price;
        }
    }
    return value;
}

double wu_basic_portfolio_asset_quantity(WU_BasicPortfolio portfolio,
        int asset_index) {
    if (!portfolio || asset_index < 0 
            || asset_index >= portfolio->num_assets) {
        return 0.0;
    }
    return wu_position_total_quantity(portfolio->positions[asset_index]);
}
