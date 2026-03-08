/**
 * Single Asset Portfolio implementation for backtesting.
 * Manages positions, cash, and transaction costs for a single asset.
 * 
 * (C) 2026 Jaime Lopez
 */

#include <stdlib.h>
#include <string.h>
#include "wu.h"

static double calculate_position_size(SingleAssetPortfolio portfolio, double price) {
    struct PositionSizingParams* ps = &portfolio->params.position_sizing;
    if (ps->size_type == POSITION_SIZE_ABS) {
        return ps->size_value;
    } else {
        double available_cash = portfolio->track.cash;
        double max_value = available_cash * ps->size_value;
        return max_value / price;
    }
}

static void execute_buy(SingleAssetPortfolio portfolio, Signal signal) {
    if (portfolio->track.cash <= 0) return;
    double quantity = calculate_position_size(portfolio, signal.price);
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
    struct Position_ pos = {
        .timestamp = signal.timestamp,
        .quantity = quantity,
        .price = slippage_price
    };
    position_add(portfolio->track.positions, &pos);
    portfolio->track.cash -= total_cost;
    portfolio->track.accum_expenses += tx_cost;
}

static void execute_sell(SingleAssetPortfolio portfolio, Signal signal) {
    double total_quantity = position_total_quantity(portfolio->track.positions);
    if (total_quantity <= 0) return;
    
    double avg_price = 0;
    double total_cost = 0;
    PositionVector* vec = portfolio->track.positions;
    for (int i = 0; i < vec->capacity; i++) {
        bool found = false;
        struct Position_ pos = position_get(vec, i, &found);
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
    portfolio_stats_record_trade(portfolio->track.stats, pnl, CLOSE_REASON_SIGNAL);
    position_clear(portfolio->track.positions);
}

static void check_and_close_positions(SingleAssetPortfolio portfolio, double current_price, int64_t timestamp) {
    PositionVector* vec = portfolio->track.positions;
    if (vec->count == 0) return;
    for (int i = 0; i < vec->capacity; i++) {
        bool found = false;
        struct Position_ pos = position_get(vec, i, &found);
        if (!found) continue;
        double pnl_pct = (current_price - pos.price) / pos.price;
        bool should_close = false;
        CloseReason reason = CLOSE_REASON_SIGNAL;
        
        if (portfolio->params.stop_loss_pct > 0 && pnl_pct <= -portfolio->params.stop_loss_pct) {
            should_close = true;
            reason = CLOSE_REASON_STOP_LOSS;
        }
        if (portfolio->params.take_profit_pct > 0 && pnl_pct >= portfolio->params.take_profit_pct) {
            should_close = true;
            reason = CLOSE_REASON_TAKE_PROFIT;
        }
        if (should_close) {
            double slippage_price = current_price * (1.0 - portfolio->params.slippage_pct);
            double proceeds = pos.quantity * slippage_price;
            double tx_cost = proceeds * portfolio->params.tx_cost_pct;
            double pnl = proceeds - (pos.quantity * pos.price) - tx_cost;
            
            portfolio->track.cash += proceeds - tx_cost;
            portfolio->track.accum_expenses += tx_cost;
            portfolio_stats_record_trade(portfolio->track.stats, pnl, reason);
            position_remove(vec, i);
        }
    }
}

double calculate_portfolio_value(SingleAssetPortfolio portfolio) {
    double position_value = 0.0;
    PositionVector* vec = portfolio->track.positions;
    for (int i = 0; i < vec->capacity; i++) {
        bool found = false;
        struct Position_ pos = position_get(vec, i, &found);
        if (found) {
            position_value += pos.quantity * portfolio->track.last_price;
        }
    }
    return portfolio->track.cash + position_value;
}

double calculate_portfolio_pnl(SingleAssetPortfolio portfolio) {
    double current_value = calculate_portfolio_value(portfolio);
    return current_value - portfolio->params.initial_cash;
}

static void update_portfolio(SingleAssetPortfolio portfolio, Signal signal) {
    portfolio->track.last_price = signal.price;
    check_and_close_positions(portfolio, signal.price, signal.timestamp);
    switch (signal.side) {
        case SIDE_BUY:
            execute_buy(portfolio, signal);
            break;
        case SIDE_SELL:
            execute_sell(portfolio, signal);
            break;
        case SIDE_HOLD:
            break;
    }
}

SingleAssetPortfolio single_asset_portfolio_new(SingleAssetPortfolioParams params) {
    SingleAssetPortfolio portfolio = malloc(sizeof(struct SingleAssetPortfolio_));
    if (!portfolio) return NULL;
    
    portfolio->base.update = (void (*)(struct Portfolio_*, Signal))update_portfolio;
    portfolio->base.value = (double (*)(struct Portfolio_*))calculate_portfolio_value;
    portfolio->base.pnl = (double (*)(struct Portfolio_*))calculate_portfolio_pnl;
    
    portfolio->params = params;
    portfolio->track.cash = params.initial_cash;
    portfolio->track.positions = position_vector_new();
    portfolio->track.last_price = 0.0;
    portfolio->track.accum_expenses = 0.0;
    portfolio->track.stats = portfolio_stats_new();
    
    return portfolio;
}

void single_asset_portfolio_free(SingleAssetPortfolio portfolio) {
    if (!portfolio) return;
    position_vector_free(portfolio->track.positions);
    portfolio_stats_free(portfolio->track.stats);
    free(portfolio);
}
