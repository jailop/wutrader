/**
 * Single Asset WU_Portfolio implementation for backtesting.
 * Manages positions, cash, and transaction costs for a single asset.
 * 
 * (C) 2026 Jaime Lopez
 */

#include <stdlib.h>
#include <string.h>
#include "wu.h"

static double calculate_wu_position_size(WU_SingleAssetPortfolio portfolio, double price) {
    struct WU_PositionSizingParams* ps = &portfolio->params.wu_position_sizing;
    if (ps->size_type == WU_POSITION_SIZE_ABS) {
        return ps->size_value;
    } else {
        double available_cash = portfolio->track.cash;
        double max_value = available_cash * ps->size_value;
        return max_value / price;
    }
}

static void execute_buy(WU_SingleAssetPortfolio portfolio, WU_Signal signal) {
    if (portfolio->track.cash <= 0) return;
    double quantity = calculate_wu_position_size(portfolio, signal.price);
    double slippage_price = signal.price * (1.0 + portfolio->params.slippage_pct);
    double cost = quantity * slippage_price;
    double tx_cost = cost * portfolio->params.tx_cost_pct;
    double total_cost = cost + tx_cost;
    if (total_cost > portfolio->track.cash) {
        quantity = portfolio->track.cash / (slippage_price * (1.0 + portfolio->params.tx_cost_pct));
        cost = quantity * slippage_price;
        tx_cost = cost * portfolio->params.tx_cost_pct;
        total_cost = cost + tx_cost;
    }
    if (quantity <= 0) return;
    struct WU_Position_ pos = {
        .timestamp = signal.timestamp,
        .quantity = quantity,
        .price = slippage_price
    };
    wu_position_add(portfolio->track.positions, &pos);
    portfolio->track.cash -= total_cost;
    portfolio->track.accum_expenses += tx_cost;
}

static void execute_sell(WU_SingleAssetPortfolio portfolio, WU_Signal signal) {
    double total_quantity = wu_position_total_quantity(portfolio->track.positions);
    if (total_quantity <= 0) return;
    double total_cost = 0;
    WU_PositionVector* vec = portfolio->track.positions;
    for (int i = 0; i < vec->capacity; i++) {
        bool found = false;
        struct WU_Position_ pos = wu_position_get(vec, i, &found);
        if (found) {
            total_cost += pos.quantity * pos.price;
        }
    }
    
    double slippage_price = signal.price * (1.0 - portfolio->params.slippage_pct);
    double proceeds = total_quantity * slippage_price;
    double tx_cost = proceeds * portfolio->params.tx_cost_pct;
    double pnl = proceeds - total_cost - tx_cost;
    
    portfolio->track.cash += proceeds - tx_cost;
    portfolio->track.accum_expenses += tx_cost;
    wu_portfolio_stats_record_trade(portfolio->track.stats, pnl, WU_CLOSE_REASON_SIGNAL);
    wu_position_clear(portfolio->track.positions);
}

static void check_and_close_positions(WU_SingleAssetPortfolio portfolio, double current_price) {
    WU_PositionVector* vec = portfolio->track.positions;
    if (vec->count == 0) return;
    for (int i = 0; i < vec->capacity; i++) {
        bool found = false;
        struct WU_Position_ pos = wu_position_get(vec, i, &found);
        if (!found) continue;
        double pnl_pct = (current_price - pos.price) / pos.price;
        bool should_close = false;
        WU_CloseReason reason = WU_CLOSE_REASON_SIGNAL;
        
        if (portfolio->params.stop_loss_pct > 0 && pnl_pct <= -portfolio->params.stop_loss_pct) {
            should_close = true;
            reason = WU_CLOSE_REASON_STOP_LOSS;
        }
        if (portfolio->params.take_profit_pct > 0 && pnl_pct >= portfolio->params.take_profit_pct) {
            should_close = true;
            reason = WU_CLOSE_REASON_TAKE_PROFIT;
        }
        if (should_close) {
            double slippage_price = current_price * (1.0 - portfolio->params.slippage_pct);
            double proceeds = pos.quantity * slippage_price;
            double tx_cost = proceeds * portfolio->params.tx_cost_pct;
            double pnl = proceeds - (pos.quantity * pos.price) - tx_cost;
            
            portfolio->track.cash += proceeds - tx_cost;
            portfolio->track.accum_expenses += tx_cost;
            wu_portfolio_stats_record_trade(portfolio->track.stats, pnl, reason);
            wu_position_remove(vec, i);
        }
    }
}

double calculate_wu_portfolio_value(const struct WU_Portfolio_ *portfolio) {
    const WU_SingleAssetPortfolio p = (const WU_SingleAssetPortfolio) portfolio;
    double wu_position_value = 0.0;
    WU_PositionVector* vec = p->track.positions;
    for (int i = 0; i < vec->capacity; i++) {
        bool found = false;
        struct WU_Position_ pos = wu_position_get(vec, i, &found);
        if (found) {
            wu_position_value += pos.quantity * p->track.last_price;
        }
    }
    return p->track.cash + wu_position_value;
}

static double calculate_wu_portfolio_pnl(const struct WU_Portfolio_ *portfolio) {
    WU_SingleAssetPortfolio p = (WU_SingleAssetPortfolio) portfolio;
    double current_value = calculate_wu_portfolio_value(portfolio);
    return current_value - p->params.initial_cash;
}

static void update_portfolio(WU_Portfolio portfolio, const WU_Signal signal) {
    WU_SingleAssetPortfolio p = (WU_SingleAssetPortfolio) portfolio;
    p->track.last_price = signal.price;
    check_and_close_positions(p, signal.price);
    switch (signal.side) {
        case WU_SIDE_BUY:
            execute_buy(p, signal);
            break;
        case WU_SIDE_SELL:
            execute_sell(p, signal);
            break;
        case WU_SIDE_HOLD:
            break;
    }
}

static void wu_singleasset_portfolio_free(struct WU_Portfolio_* portfolio) {
    WU_SingleAssetPortfolio p = (WU_SingleAssetPortfolio) portfolio;
    if (!p) return;
    wu_position_vector_delete(p->track.positions);
    wu_portfolio_stats_delete(p->track.stats);
    free(portfolio);
}

WU_SingleAssetPortfolio wu_singleasset_portfolio_new(WU_SingleAssetPortfolioParams params) {
    WU_SingleAssetPortfolio portfolio = malloc(sizeof(struct WU_SingleAssetPortfolio_));
    if (!portfolio) return NULL;
    
    portfolio->base.update = update_portfolio;
    portfolio->base.value = calculate_wu_portfolio_value;
    portfolio->base.pnl = calculate_wu_portfolio_pnl;
    portfolio->base.delete = wu_singleasset_portfolio_free;
    
    portfolio->params = params;
    portfolio->track.cash = params.initial_cash;
    portfolio->track.positions = wu_position_vector_new();
    portfolio->track.last_price = 0.0;
    portfolio->track.accum_expenses = 0.0;
    portfolio->track.stats = wu_portfolio_stats_new();
    
    return portfolio;
}


