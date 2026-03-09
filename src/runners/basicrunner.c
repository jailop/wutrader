#include <stdlib.h>
#include "wu.h"

static void run_backtest(WU_BasicRunner runner, bool verbose) {
    if (!runner) return;
    
    WU_Portfolio portfolio = runner->portfolio;
    WU_Strategy strategy = runner->strategy;
    WU_Reader reader = runner->reader;
    
    void* data;
    while ((data = wu_reader_next(reader)) != NULL) {
        void* wu_strategy_data = data;
        WU_Single converted;
        
        WU_Candle* candle = (WU_Candle*)data;
        if (candle->data_type == WU_DATA_TYPE_CANDLE) {
            converted = wu_candle_to_single_value(candle);
            wu_strategy_data = &converted;
        }
        
        WU_Signal sig = wu_strategy_update(strategy, wu_strategy_data);
        wu_portfolio_update(portfolio, sig);
        
        if (verbose) {
            WU_SingleAssetPortfolio p = (WU_SingleAssetPortfolio)portfolio;
            const char* side_str = sig.side == WU_SIDE_BUY ? "BUY" : 
                                   sig.side == WU_SIDE_SELL ? "SELL" : "HOLD";
            printf("Time: %ld | WU_Signal: %-4s | Price: %.2f | Qty: %.4f | "
                    "Cash: %.2f | Value: %.2f | P&L: %.2f\n",
                   sig.timestamp,
                   side_str,
                   sig.price,
                   sig.quantity,
                   p->track.cash,
                   wu_portfolio_value(portfolio),
                   wu_portfolio_pnl(portfolio));
        }
    }
}

WU_BasicRunner wu_basic_runner_new(WU_Portfolio portfolio, WU_Strategy strategy, WU_Reader reader) {
    WU_BasicRunner runner = malloc(sizeof(struct WU_BasicRunner_));
    if (!runner) return NULL;
    runner->portfolio = portfolio;
    runner->strategy = strategy;
    runner->reader = reader;
    runner->run = run_backtest;
    return runner;
}

void wu_basic_runner_free(WU_BasicRunner runner) {
    if (!runner) return;
    wu_portfolio_delete(runner->portfolio);
    wu_strategy_delete(runner->strategy);
    wu_reader_delete(runner->reader);
    free(runner);
}
