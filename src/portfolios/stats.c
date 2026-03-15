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
}

static void stats_record_trade(struct WU_PortfolioStats_* stats, double pnl, 
        WU_CloseReason reason) {
    if (!stats) return;
    
    stats->total_trades++;
    
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
    
    char* result = malloc(8192);
    if (!result) return NULL;
    
    int offset = snprintf(result, 8192,
        "initial_cash=%.2f current_cash=%.2f portfolio_value=%.2f "
        "pnl=%.2f pnl_pct=%.2f tx_fees=%.2f borrow_interest=%.2f "
        "total_trades=%ld winning_trades=%ld losing_trades=%ld "
        "win_rate=%.2f max_win=%.2f max_loss=%.2f "
        "stop_loss_exits=%ld take_profit_exits=%ld",
        stats->initial_cash, stats->current_cash, stats->portfolio_value,
        pnl, pnl_pct, stats->accum_tx_fees, stats->accum_borrow_interest,
        stats->total_trades, stats->winning_trades, stats->losing_trades,
        win_rate, stats->max_win, stats->max_loss,
        stats->stop_loss_exits, stats->take_profit_exits);
    
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
        "%s\"stop_loss_exits\":%s%ld,%s"
        "%s\"take_profit_exits\":%s%ld%s"
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
        indent2, space, stats->stop_loss_exits, nl,
        indent2, space, stats->take_profit_exits, nl,
        indent1);
    
    // Add positions array
    if (stats->num_assets > 0) {
        offset += snprintf(result + offset, 16384 - offset,
            ",%s%s\"positions\":%s[", nl, indent1, space);
        
        for (int i = 0; i < stats->num_assets; i++) {
            if (stats->symbols[i]) {
                if (i > 0) offset += snprintf(result + offset, 16384 - offset, ",");
                offset += snprintf(result + offset, 16384 - offset,
                    "%s%s{%s\"symbol\":%s\"%s\",%s\"quantity\":%s%.4f,%s\"value\":%s%.2f,%s\"last_price\":%s%.2f%s}",
                    nl, indent2,
                    space, space, stats->symbols[i],
                    space, space, stats->quantities[i],
                    space, space, stats->values[i],
                    space, space, stats->last_prices[i],
                    nl, indent2);
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
    free(stats);
}

WU_PortfolioStats wu_portfolio_stats_new(double initial_cash) {
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
    
    stats->symbols = NULL;
    stats->quantities = NULL;
    stats->values = NULL;
    stats->last_prices = NULL;
    stats->num_assets = 0;
    stats->capacity = 0;
    
    return stats;
}
