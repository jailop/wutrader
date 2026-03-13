#ifndef WU_RUNNER_H
#define WU_RUNNER_H

#include "portfolios.h"
#include "strategies.h"
#include "readers.h"

/**
 * WU_Runner is a backtest runner that processes data from multiple readers,
 * updates a strategy, and executes trades in a portfolio.
 * 
 * Key features:
 * - Supports both single-input and multi-input strategies
 * - Validates reader count matches strategy input requirements
 * - Validates and converts data types automatically (e.g., Candle → Single)
 * - Synchronizes data from multiple readers sequentially (lockstep)
 * - No fixed limits on number of inputs (heap-allocated)
 * 
 * Usage:
 *   // Single-input strategy (e.g., crossover on SPY)
 *   WU_Reader readers[] = {spy_reader};
 *   WU_Runner runner = wu_runner_new(portfolio, strategy, readers);
 * 
 *   // Multi-input strategy (e.g., pairs trading on SPY/QQQ)
 *   WU_Reader readers[] = {spy_reader, qqq_reader};
 *   WU_Runner runner = wu_runner_new(portfolio, strategy, readers);
 * 
 *   wu_runner_exec(runner, verbose);
 *   wu_runner_free(runner);
 */

typedef struct WU_Runner_ {
    WU_Portfolio portfolio;
    WU_Strategy strategy;
    WU_Reader* readers;
    int num_readers;
    void (*run)(struct WU_Runner_* runner, bool verbose);
}* WU_Runner;

#define wu_runner_exec(runner, verbose) ((runner)->run((runner), (verbose)))

/**
 * Creates a new runner with multiple readers.
 * 
 * @param portfolio The portfolio to execute trades in
 * @param strategy The strategy to generate signals
 * @param readers Array of readers (one per strategy input)
 * @return New runner instance, or NULL on validation failure
 * 
 * Validation performed:
 * - Number of readers determined from strategy->num_inputs
 * - Each reader's data type must be compatible with strategy's input_types
 * 
 * Memory is dynamically allocated based on strategy input requirements.
 */
WU_Runner wu_runner_new(
    WU_Portfolio portfolio, 
    WU_Strategy strategy, 
    WU_Reader readers[]
);

/**
 * Convenience function for single-input strategies.
 * Equivalent to wu_runner_new(portfolio, strategy, &reader, 1).
 */
static inline WU_Runner wu_runner_new_single(
    WU_Portfolio portfolio,
    WU_Strategy strategy,
    WU_Reader reader
) {
    return wu_runner_new(portfolio, strategy, &reader);
}

void wu_runner_free(WU_Runner runner);

#endif // WU_RUNNER_H
