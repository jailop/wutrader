#include <stdlib.h>
#include "wu.h"

static void run_backtest(BasicRunner runner, bool verbose) {
    if (!runner) return;
    
    Portfolio portfolio = runner->portfolio;
    Strategy strategy = runner->strategy;
    Reader reader = runner->reader;
    
    void* data;
    while ((data = reader_next(reader)) != NULL) {
        void* strategy_data = data;
        SingleValue converted;
        
        Candle* candle = (Candle*)data;
        if (candle->data_type == DATA_TYPE_CANDLE) {
            converted = candle_to_single_value(candle);
            strategy_data = &converted;
        }
        
        Signal sig = strategy_update(strategy, strategy_data);
        portfolio_update(portfolio, sig);
        
        if (verbose) {
            SingleAssetPortfolio p = (SingleAssetPortfolio)portfolio;
            const char* side_str = sig.side == SIDE_BUY ? "BUY" : 
                                   sig.side == SIDE_SELL ? "SELL" : "HOLD";
            printf("Time: %ld | Signal: %-4s | Price: %.2f | Qty: %.4f | "
                    "Cash: %.2f | Value: %.2f | P&L: %.2f\n",
                   sig.timestamp,
                   side_str,
                   sig.price,
                   sig.quantity,
                   p->track.cash,
                   portfolio_value(portfolio),
                   portfolio_pnl(portfolio));
        }
    }
}

BasicRunner basic_runner_new(Portfolio portfolio, Strategy strategy, Reader reader) {
    BasicRunner runner = malloc(sizeof(struct BasicRunner_));
    if (!runner) return NULL;
    runner->portfolio = portfolio;
    runner->strategy = strategy;
    runner->reader = reader;
    runner->run = run_backtest;
    return runner;
}

void basic_runner_free(BasicRunner runner) {
    if (!runner) return;
    free(runner);
}
