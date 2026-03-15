#ifndef _STATS_H_
#define _STATS_H_

/**
 * WU_PortfolioStats tracks trading statistics.
 */
typedef struct WU_PortfolioStats_ {
    void (*record_trade)(struct WU_PortfolioStats_* stats, double pnl,
            WU_CloseReason reason);
    void (*reset)(struct WU_PortfolioStats_* stats);
    void (*delete)(struct WU_PortfolioStats_* stats);
    int64_t total_trades;
    int64_t winning_trades;
    int64_t losing_trades;
    int64_t stop_loss_exits;
    int64_t take_profit_exits;
    double total_profit;
    double total_loss;
    double max_win;
    double max_loss;
}* WU_PortfolioStats;

/**
 * Creates a new WU_PortfolioStats instance.
 */
WU_PortfolioStats wu_portfolio_stats_new(void);

/**
 * Records a trade in the stats.
 */
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

#endif // _STATS_H_
