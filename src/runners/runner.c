/**
 * Multi-Input Runner Implementation
 * Supports both single and multi-input strategies with automatic type conversion.
 * 
 * (C) 2026 Jaime Lopez
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "wu.h"

/**
 * Validates that reader data type is compatible with expected strategy input type.
 * Handles automatic conversion (e.g., Candle can be converted to Single).
 */
static bool validate_input_type(WU_DataType reader_type, WU_DataType expected_type) {
    // Exact match is always valid
    if (reader_type == expected_type) {
        return true;
    }
    
    // Candle can be converted to Single (using close price)
    if (reader_type == WU_DATA_TYPE_CANDLE && expected_type == WU_DATA_TYPE_SINGLE_VALUE) {
        return true;
    }
    
    // Trade can be converted to Single (using trade price)
    if (reader_type == WU_DATA_TYPE_TRADE && expected_type == WU_DATA_TYPE_SINGLE_VALUE) {
        return true;
    }
    
    return false;
}

/**
 * Determines the data type that a reader produces.
 * For CSV readers, we can check the data_type field.
 */
static WU_DataType get_reader_data_type(WU_Reader reader) {
    // Try to peek at the data type from CSV reader
    WU_CsvReader csv = (WU_CsvReader)reader;
    if (csv && csv->data_type != 0) {
        return csv->data_type;
    }
    
    // Default assumption: unknown, will validate at runtime
    return WU_DATA_TYPE_SINGLE_VALUE;
}

static void run_backtest(WU_Runner runner, bool verbose) {
    if (!runner) return;
    
    WU_Portfolio portfolio = runner->portfolio;
    WU_Strategy strategy = runner->strategy;
    
    // Allocate input buffers dynamically based on number of readers
    void** input_buffers = malloc(sizeof(void*) * runner->num_readers);
    WU_Single* converted = malloc(sizeof(WU_Single) * runner->num_readers);
    
    if (!input_buffers || !converted) {
        fprintf(stderr, "Error: Failed to allocate input buffers\n");
        free(input_buffers);
        free(converted);
        return;
    }
    
    if (verbose) {
        printf("Starting backtest with %d input(s)...\n", runner->num_readers);
    }
    
    int iteration = 0;
    while (true) {
        // Read from all readers in lockstep
        bool all_valid = true;
        
        for (int i = 0; i < runner->num_readers; i++) {
            void* data = wu_reader_next(runner->readers[i]);
            
            if (!data) {
                all_valid = false;
                break;
            }
            
            // Determine actual data type from the data itself
            WU_DataType actual_type = ((WU_Candle*)data)->data_type;
            WU_DataType expected_type = strategy->input_types[i];
            
            // Apply automatic type conversion if needed
            if (actual_type == WU_DATA_TYPE_CANDLE && 
                expected_type == WU_DATA_TYPE_SINGLE_VALUE) {
                // Convert Candle to Single using close price
                converted[i] = wu_candle_to_single_value((WU_Candle*)data);
                input_buffers[i] = &converted[i];
            } 
            else if (actual_type == WU_DATA_TYPE_TRADE && 
                     expected_type == WU_DATA_TYPE_SINGLE_VALUE) {
                // Convert Trade to Single using trade price
                converted[i] = wu_trade_to_single_value((WU_Trade*)data);
                input_buffers[i] = &converted[i];
            }
            else if (actual_type == expected_type) {
                // Direct pass-through, no conversion needed
                input_buffers[i] = data;
            }
            else {
                // Type mismatch that cannot be converted
                fprintf(stderr, "Error at iteration %d: Input %d has type %d but strategy expects type %d\n",
                        iteration, i, actual_type, expected_type);
                all_valid = false;
                break;
            }
        }
        
        // Stop if any reader finished or had an error
        if (!all_valid) {
            break;
        }
        
        // Update strategy with all inputs
        wu_strategy_update(strategy, (const void**)input_buffers);
        
        // Update portfolio with signals from strategy
        wu_portfolio_update(portfolio, strategy->signal_buffer);
        
        // Print verbose output if requested
        if (verbose && iteration % 100 == 0) {
            printf("Iteration %d | Signals: %d | Value: %.2f | P&L: %.2f\n",
                   iteration,
                   strategy->num_outputs,
                   wu_portfolio_value(portfolio),
                   wu_portfolio_pnl(portfolio));
        }
        
        iteration++;
    }
    
    if (verbose) {
        printf("Backtest completed after %d iterations\n", iteration);
    }
    
    // Free allocated buffers
    free(input_buffers);
    free(converted);
}

WU_Runner wu_runner_new(
    WU_Portfolio portfolio, 
    WU_Strategy strategy, 
    WU_Reader readers[]
) {
    // Validate inputs
    if (!portfolio || !strategy || !readers) {
        fprintf(stderr, "Error: NULL argument passed to wu_runner_new\n");
        return NULL;
    }

    int num_readers = wu_strategy_num_inputs(strategy);
    
    // Validate each reader's data type compatibility
    for (int i = 0; i < num_readers; i++) {
        if (!readers[i]) {
            fprintf(stderr, "Error: Reader %d is NULL\n", i);
            return NULL;
        }
        
        WU_DataType reader_type = get_reader_data_type(readers[i]);
        WU_DataType expected_type = strategy->input_types[i];
        
        if (!validate_input_type(reader_type, expected_type)) {
            fprintf(stderr, "Error: Reader %d produces type %d but strategy expects type %d\n",
                    i, reader_type, expected_type);
            return NULL;
        }
    }
    
    // Allocate runner
    WU_Runner runner = malloc(sizeof(struct WU_Runner_));
    if (!runner) {
        fprintf(stderr, "Error: Failed to allocate runner\n");
        return NULL;
    }
    
    // Allocate and copy reader array
    runner->readers = malloc(sizeof(WU_Reader) * num_readers);
    if (!runner->readers) {
        fprintf(stderr, "Error: Failed to allocate reader array\n");
        free(runner);
        return NULL;
    }
    
    memcpy(runner->readers, readers, sizeof(WU_Reader) * num_readers);
    
    // Initialize runner
    runner->portfolio = portfolio;
    runner->strategy = strategy;
    runner->num_readers = num_readers;
    runner->run = run_backtest;
    
    return runner;
}

void wu_runner_free(WU_Runner runner) {
    if (!runner) return;
    
    // Free portfolio
    wu_portfolio_delete(runner->portfolio);
    
    // Free strategy
    wu_strategy_delete(runner->strategy);
    
    // Free all readers
    for (int i = 0; i < runner->num_readers; i++) {
        wu_reader_delete(runner->readers[i]);
    }
    free(runner->readers);
    
    // Free runner itself
    free(runner);
}
