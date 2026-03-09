#ifndef WU_RUNNER_H
#define WU_RUNNER_H

#include "portfolio.h"
#include "strategy.h"
#include "reader.h"

/**
 * Runner is the base interface for a backtest runner.
 */
typedef struct Runner_ {
    void (*run)(struct Runner_* runner, bool verbose);
}* Runner;

#define runner_run(runner, verbose) ((runner)->run((runner), (verbose)))

/**
 * BasicRunner is a simple runner implementation that processes data
 * from a reader, updates a strategy, and executes trades in a portfolio.
 */
typedef struct BasicRunner_ {
    Portfolio portfolio;
    Strategy strategy;
    Reader reader;
    void (*run)(struct BasicRunner_* runner, bool verbose);
}* BasicRunner;

BasicRunner basic_runner_new(Portfolio portfolio, Strategy strategy, Reader reader);
void basic_runner_free(BasicRunner runner);

#endif // WU_RUNNER_H
