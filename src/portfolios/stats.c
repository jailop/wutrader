#include <stdlib.h>
#include "wu.h"

static void stats_record_trade(struct WU_PortfolioStats_* stats, double pnl, WU_CloseReason reason) {
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

static void stats_reset(struct WU_PortfolioStats_* stats) {
    if (!stats) return;
    stats->total_trades = 0;
    stats->winning_trades = 0;
    stats->losing_trades = 0;
    stats->stop_loss_exits = 0;
    stats->take_profit_exits = 0;
    stats->total_profit = 0.0;
    stats->total_loss = 0.0;
    stats->max_win = 0.0;
    stats->max_loss = 0.0;
}

static void stats_free(struct WU_PortfolioStats_* stats) {
    if (stats) free(stats);
}

WU_PortfolioStats wu_portfolio_stats_new(void) {
    WU_PortfolioStats stats = malloc(sizeof(struct WU_PortfolioStats_));
    if (!stats) return NULL;
    
    stats->record_trade = stats_record_trade;
    stats->reset = stats_reset;
    stats->delete = stats_free;
    
    stats_reset(stats);
    return stats;
}
