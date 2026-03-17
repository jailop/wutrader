#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "wu.h"

static char* wu_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* copy = malloc(len);
    if (copy) memcpy(copy, s, len);
    return copy;
}

static void stats_update(struct WU_PortfolioStats_* stats, double cash,
        double portfolio_value, WU_TimeStamp timestamp) {
    if (!stats) return;
    stats->current_cash = cash;
    stats->portfolio_value = portfolio_value;
    stats->last_update = timestamp;
    
    // Create performance update struct for time-aware metrics
    WU_PerformanceUpdate perf = {
        .portfolio_value = portfolio_value,
        .timestamp = timestamp
    };
    
    // Update performance indicators
    if (stats->max_drawdown)
        stats->max_drawdown->update(stats->max_drawdown, portfolio_value);
    if (stats->sharpe_ratio)
        stats->sharpe_ratio->update(stats->sharpe_ratio, perf);
    if (stats->sortino_ratio)
        stats->sortino_ratio->update(stats->sortino_ratio, perf);
    if (stats->calmar_ratio)
        stats->calmar_ratio->update(stats->calmar_ratio, perf);
}

static void stats_record_trade(struct WU_PortfolioStats_* stats, double pnl, 
        WU_CloseReason reason) {
    if (!stats) return;
    
    stats->total_trades++;
    stats->accum_pnl += pnl;
    
    // Update PnL statistics
    if (stats->pnl_stats)
        stats->pnl_stats->update(stats->pnl_stats, pnl);
    
    if (pnl > 0) {
        stats->winning_trades++;
        stats->total_profit += pnl;
        if (pnl > stats->max_win)
            stats->max_win = pnl;
    } else if (pnl < 0) {
        stats->losing_trades++;
        stats->total_loss += pnl;
        if (pnl < stats->max_loss)
            stats->max_loss = pnl;
    }
    
    if (reason == WU_CLOSE_REASON_STOP_LOSS)
        stats->stop_loss_exits++;
    else if (reason == WU_CLOSE_REASON_TAKE_PROFIT)
        stats->take_profit_exits++;
}

static void stats_update_position(struct WU_PortfolioStats_* stats, int asset_index,
        const char* symbol, double quantity, double value, double last_price) {
    if (!stats) return;
    if (asset_index < 0) return;
    
    // Expand capacity if needed
    if (asset_index >= stats->capacity) {
        int new_capacity = asset_index + 1;
        stats->symbols = realloc(stats->symbols, new_capacity * sizeof(char*));
        stats->quantities = realloc(stats->quantities, new_capacity * sizeof(double));
        stats->values = realloc(stats->values, new_capacity * sizeof(double));
        stats->last_prices = realloc(stats->last_prices, new_capacity * sizeof(double));
        
        for (int i = stats->capacity; i < new_capacity; i++) {
            stats->symbols[i] = NULL;
            stats->quantities[i] = 0.0;
            stats->values[i] = 0.0;
            stats->last_prices[i] = 0.0;
        }
        stats->capacity = new_capacity;
    }
    
    if (asset_index >= stats->num_assets)
        stats->num_assets = asset_index + 1;
    
    // Update or set symbol
    if (symbol && (!stats->symbols[asset_index] || 
        strcmp(stats->symbols[asset_index], symbol) != 0)) {
        free(stats->symbols[asset_index]);
        stats->symbols[asset_index] = wu_strdup(symbol);
    }
    
    stats->quantities[asset_index] = quantity;
    stats->values[asset_index] = value;
    stats->last_prices[asset_index] = last_price;
}

static char* stats_to_keyvalue(struct WU_PortfolioStats_* stats) {
    if (!stats) return NULL;
    
    double pnl = stats->portfolio_value - stats->initial_cash;
    double pnl_pct = (pnl / stats->initial_cash) * 100.0;
    double win_rate = stats->total_trades > 0 ? 
        (stats->winning_trades * 100.0) / stats->total_trades : 0.0;
    double avg_pnl = stats->total_trades > 0 ? stats->accum_pnl / stats->total_trades : 0.0;
    WU_PnLStatsResult pnl_stats_result = stats->pnl_stats ? stats->pnl_stats->get(stats->pnl_stats) : (WU_PnLStatsResult){NAN, NAN};
    
    char* result = malloc(8192);
    if (!result) return NULL;
    
    int offset = snprintf(result, 8192,
        "initial_cash=%.2f current_cash=%.2f portfolio_value=%.2f "
        "pnl=%.2f pnl_pct=%.2f tx_fees=%.2f borrow_interest=%.2f "
        "total_trades=%ld winning_trades=%ld losing_trades=%ld "
        "win_rate=%.2f max_win=%.2f max_loss=%.2f avg_pnl=%.2f pnl_stddev=%.2f "
        "stop_loss_exits=%ld take_profit_exits=%ld",
        stats->initial_cash, stats->current_cash, stats->portfolio_value,
        pnl, pnl_pct, stats->accum_tx_fees, stats->accum_borrow_interest,
        stats->total_trades, stats->winning_trades, stats->losing_trades,
        win_rate, stats->max_win, stats->max_loss, avg_pnl,
        isnan(pnl_stats_result.stddev) ? 0.0 : pnl_stats_result.stddev,
        stats->stop_loss_exits, stats->take_profit_exits);
    
    // Add performance metrics
    if (stats->max_drawdown) {
        double mdd = stats->max_drawdown->get(stats->max_drawdown);
        offset += snprintf(result + offset, 8192 - offset,
            " max_drawdown=%.4f", mdd);
    }
    if (stats->sharpe_ratio) {
        double sharpe = stats->sharpe_ratio->get(stats->sharpe_ratio);
        offset += snprintf(result + offset, 8192 - offset,
            " sharpe_ratio=%.4f", isnan(sharpe) ? 0.0 : sharpe);
    }
    if (stats->sortino_ratio) {
        double sortino = stats->sortino_ratio->get(stats->sortino_ratio);
        offset += snprintf(result + offset, 8192 - offset,
            " sortino_ratio=%.4f", isnan(sortino) ? 0.0 : sortino);
    }
    if (stats->calmar_ratio) {
        double calmar = stats->calmar_ratio->get(stats->calmar_ratio);
        offset += snprintf(result + offset, 8192 - offset,
            " calmar_ratio=%.4f", isnan(calmar) ? 0.0 : calmar);
    }
    
    // Add position data
    for (int i = 0; i < stats->num_assets; i++) {
        if (stats->symbols[i]) {
            offset += snprintf(result + offset, 8192 - offset,
                " %s_quantity=%.4f %s_value=%.2f %s_price=%.2f",
                stats->symbols[i], stats->quantities[i],
                stats->symbols[i], stats->values[i],
                stats->symbols[i], stats->last_prices[i]);
        }
    }
    
    return result;
}

static char* stats_to_json(struct WU_PortfolioStats_* stats, bool pretty) {
    if (!stats) return NULL;
    
    const char* nl = pretty ? "\n" : "";
    const char* indent1 = pretty ? "  " : "";
    const char* indent2 = pretty ? "    " : "";
    const char* space = pretty ? " " : "";
    
    double pnl = stats->portfolio_value - stats->initial_cash;
    double pnl_pct = (pnl / stats->initial_cash) * 100.0;
    double win_rate = stats->total_trades > 0 ? 
        (stats->winning_trades * 100.0) / stats->total_trades : 0.0;
    double avg_pnl = stats->total_trades > 0 ? stats->accum_pnl / stats->total_trades : 0.0;
    WU_PnLStatsResult pnl_stats_result = stats->pnl_stats ? stats->pnl_stats->get(stats->pnl_stats) : (WU_PnLStatsResult){NAN, NAN};
    double pnl_stddev = isnan(pnl_stats_result.stddev) ? 0.0 : pnl_stats_result.stddev;
    
    char* result = malloc(16384);
    if (!result) return NULL;
    
    int offset = snprintf(result, 16384,
        "{%s"
        "%s\"portfolio\":%s{%s"
        "%s\"initial_cash\":%s%.2f,%s"
        "%s\"current_cash\":%s%.2f,%s"
        "%s\"portfolio_value\":%s%.2f,%s"
        "%s\"pnl\":%s%.2f,%s"
        "%s\"pnl_pct\":%s%.2f,%s"
        "%s\"tx_fees\":%s%.2f,%s"
        "%s\"borrow_interest\":%s%.2f%s"
        "%s},%s"
        "%s\"trades\":%s{%s"
        "%s\"total\":%s%ld,%s"
        "%s\"winning\":%s%ld,%s"
        "%s\"losing\":%s%ld,%s"
        "%s\"win_rate\":%s%.2f,%s"
        "%s\"max_win\":%s%.2f,%s"
        "%s\"max_loss\":%s%.2f,%s"
        "%s\"avg_pnl\":%s%.2f,%s"
        "%s\"pnl_stddev\":%s%.2f,%s"
        "%s\"stop_loss_exits\":%s%ld,%s"
        "%s\"take_profit_exits\":%s%ld%s"
        "%s},%s"
        "%s\"performance\":%s{%s"
        "%s\"max_drawdown\":%s%.4f,%s"
        "%s\"sharpe_ratio\":%s%.4f,%s"
        "%s\"sortino_ratio\":%s%.4f,%s"
        "%s\"calmar_ratio\":%s%.4f%s"
        "%s}",
        nl,
        indent1, space, nl,
        indent2, space, stats->initial_cash, nl,
        indent2, space, stats->current_cash, nl,
        indent2, space, stats->portfolio_value, nl,
        indent2, space, pnl, nl,
        indent2, space, pnl_pct, nl,
        indent2, space, stats->accum_tx_fees, nl,
        indent2, space, stats->accum_borrow_interest, nl,
        indent1, nl,
        indent1, space, nl,
        indent2, space, stats->total_trades, nl,
        indent2, space, stats->winning_trades, nl,
        indent2, space, stats->losing_trades, nl,
        indent2, space, win_rate, nl,
        indent2, space, stats->max_win, nl,
        indent2, space, stats->max_loss, nl,
        indent2, space, avg_pnl, nl,
        indent2, space, pnl_stddev, nl,
        indent2, space, stats->stop_loss_exits, nl,
        indent2, space, stats->take_profit_exits, nl,
        indent1, nl,
        indent1, space, nl,
        indent2, space, (stats->max_drawdown ? stats->max_drawdown->get(stats->max_drawdown) : 0.0), nl,
        indent2, space, (stats->sharpe_ratio ? (isnan(stats->sharpe_ratio->get(stats->sharpe_ratio)) ? 0.0 : stats->sharpe_ratio->get(stats->sharpe_ratio)) : 0.0), nl,
        indent2, space, (stats->sortino_ratio ? (isnan(stats->sortino_ratio->get(stats->sortino_ratio)) ? 0.0 : stats->sortino_ratio->get(stats->sortino_ratio)) : 0.0), nl,
        indent2, space, (stats->calmar_ratio ? (isnan(stats->calmar_ratio->get(stats->calmar_ratio)) ? 0.0 : stats->calmar_ratio->get(stats->calmar_ratio)) : 0.0), nl,
        indent1);
    
    // Add positions array
    if (stats->num_assets > 0) {
        offset += snprintf(result + offset, 16384 - offset,
            ",%s%s\"positions\":%s[", nl, indent1, space);
        
        for (int i = 0; i < stats->num_assets; i++) {
            if (stats->symbols[i]) {
                if (i > 0) offset += snprintf(result + offset, 16384 - offset, ",");
                offset += snprintf(result + offset, 16384 - offset,
                    "%s%s{%s\"symbol\": \"%s\",%s\"quantity\": %.4f,%s\"value\": %.2f,%s\"last_price\": %.2f%s}",
                    nl, indent2,
                    space, stats->symbols[i],
                    space, stats->quantities[i],
                    space, stats->values[i],
                    space, stats->last_prices[i],
                    nl);
            }
        }
        offset += snprintf(result + offset, 16384 - offset,
            "%s%s]", nl, indent1);
    }
    
    offset += snprintf(result + offset, 16384 - offset, "%s}", nl);
    
    return result;
}

static void stats_reset(struct WU_PortfolioStats_* stats) {
    if (!stats) return;
    stats->current_cash = stats->initial_cash;
    stats->portfolio_value = stats->initial_cash;
    stats->last_update = (WU_TimeStamp){.mark = 0, .units = WU_TIME_UNIT_SECONDS};
    stats->accum_tx_fees = 0.0;
    stats->accum_borrow_interest = 0.0;
    stats->total_trades = 0;
    stats->winning_trades = 0;
    stats->losing_trades = 0;
    stats->stop_loss_exits = 0;
    stats->take_profit_exits = 0;
    stats->total_profit = 0.0;
    stats->total_loss = 0.0;
    stats->max_win = 0.0;
    stats->max_loss = 0.0;
    stats->accum_pnl = 0.0;
    
    for (int i = 0; i < stats->num_assets; i++) {
        stats->quantities[i] = 0.0;
        stats->values[i] = 0.0;
        stats->last_prices[i] = 0.0;
    }
}

static void stats_free(struct WU_PortfolioStats_* stats) {
    if (!stats) return;
    for (int i = 0; i < stats->capacity; i++) {
        free(stats->symbols[i]);
    }
    free(stats->symbols);
    free(stats->quantities);
    free(stats->values);
    free(stats->last_prices);
    
    // Free performance indicators
    if (stats->max_drawdown)
        stats->max_drawdown->delete(stats->max_drawdown);
    if (stats->sharpe_ratio)
        stats->sharpe_ratio->delete(stats->sharpe_ratio);
    if (stats->sortino_ratio)
        stats->sortino_ratio->delete(stats->sortino_ratio);
    if (stats->calmar_ratio)
        stats->calmar_ratio->delete(stats->calmar_ratio);
    if (stats->pnl_stats)
        stats->pnl_stats->delete(stats->pnl_stats);
    
    free(stats);
}

WU_PortfolioStats wu_portfolio_stats_new(double initial_cash, double risk_free_rate) {
    WU_PortfolioStats stats = malloc(sizeof(struct WU_PortfolioStats_));
    if (!stats) return NULL;
    
    stats->update = stats_update;
    stats->record_trade = stats_record_trade;
    stats->update_position = stats_update_position;
    stats->to_keyvalue = stats_to_keyvalue;
    stats->to_json = stats_to_json;
    stats->reset = stats_reset;
    stats->delete = stats_free;
    
    stats->initial_cash = initial_cash;
    stats->current_cash = initial_cash;
    stats->portfolio_value = initial_cash;
    stats->last_update = (WU_TimeStamp){.mark = 0, .units = WU_TIME_UNIT_SECONDS};
    
    stats->accum_tx_fees = 0.0;
    stats->accum_borrow_interest = 0.0;
    
    stats->total_trades = 0;
    stats->winning_trades = 0;
    stats->losing_trades = 0;
    stats->stop_loss_exits = 0;
    stats->take_profit_exits = 0;
    stats->total_profit = 0.0;
    stats->total_loss = 0.0;
    stats->max_win = 0.0;
    stats->max_loss = 0.0;
    stats->accum_pnl = 0.0;
    
    stats->symbols = NULL;
    stats->quantities = NULL;
    stats->values = NULL;
    stats->last_prices = NULL;
    stats->num_assets = 0;
    stats->capacity = 0;
    
    // Initialize performance indicators
    stats->max_drawdown = wu_max_drawdown_new();
    stats->sharpe_ratio = wu_sharpe_ratio_new(initial_cash, risk_free_rate);
    stats->sortino_ratio = wu_sortino_ratio_new(initial_cash, risk_free_rate);
    stats->calmar_ratio = wu_calmar_ratio_new(initial_cash);
    stats->pnl_stats = wu_pnl_stats_new();
    
    if (!stats->max_drawdown || !stats->sharpe_ratio || 
        !stats->sortino_ratio || !stats->calmar_ratio || !stats->pnl_stats) {
        stats_free(stats);
        return NULL;
    }
    
    return stats;
}
