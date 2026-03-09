#ifndef WU_RUNNER_H
#define WU_RUNNER_H

#include "portfolios.h"
#include "strategies.h"
#include "readers.h"

/**
 * WU_Runner is the base interface for a backtest runner.
 */
typedef struct WU_Runner_ {
    void (*exec)(struct WU_Runner_* runner, bool verbose);
}* WU_Runner;

#define runner_exec(runner, verbose) ((runner)->run((runner), (verbose)))

/**
 * WU_BasicRunner is a simple runner implementation that processes data
 * from a reader, updates a strategy, and executes trades in a portfolio.
 */
typedef struct WU_BasicRunner_ {
    WU_Portfolio portfolio;
    WU_Strategy strategy;
    WU_Reader reader;
    void (*run)(struct WU_BasicRunner_* runner, bool verbose);
}* WU_BasicRunner;

WU_BasicRunner wu_basic_runner_new(WU_Portfolio portfolio, WU_Strategy strategy, WU_Reader reader);
void wu_basic_runner_free(WU_BasicRunner runner);

#endif // WU_RUNNER_H
