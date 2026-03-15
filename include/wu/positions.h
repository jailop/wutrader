#ifndef _POSITIONS_H
#define _POSITIONS_H

#include <stdint.h>
#include <stdbool.h>
#include "types.h"

#define WU_SYMBOL_MAX_LEN 32

/**
 * WU_Position represents an open position in the portfolio.
 */
typedef struct WU_Position_ {
    int64_t timestamp;
    double quantity;
    double price;
    bool active;
}* WU_Position;

/**
 * WU_PositionVector is a data structure that holds multiple positions
 * for a single asset. It includes the asset symbol and last price.
 */
typedef struct WU_PositionVector {
    void (*add)(struct WU_PositionVector* vec, WU_Position pos);
    void (*remove)(struct WU_PositionVector* vec, int index);
    void (*clear)(struct WU_PositionVector* vec);
    struct WU_Position_ (*get)(struct WU_PositionVector* vec, int index,
            bool* found);
    double (*total_quantity)(struct WU_PositionVector* vec);
    void (*delete)(struct WU_PositionVector* vec);
    char* symbol;
    double last_price;
    struct WU_Position_* positions;
    bool* active;
    int count;
    int capacity;
} WU_PositionVector;

#define wu_position_add(vec, pos) ((vec)->add((vec), (pos)))
#define wu_position_remove(vec, index) ((vec)->remove((vec), (index)))
#define wu_position_clear(vec) ((vec)->clear((vec)))
#define wu_position_get(vec, index, found) ((vec)->get((vec), (index), (found)))
#define wu_position_total_quantity(vec) ((vec)->total_quantity((vec)))
#define wu_position_vector_delete(vec) do { \
    if ((vec)->delete) \
        (vec)->delete((WU_PositionVector*)(vec)); \
} while(0)

WU_PositionVector* wu_position_vector_new(const char* symbol);

/**
 * WU_PositionSizeType represents the type of position sizing used in the
 * portfolio. It can be an absolute quantity, a percentage of the
 * portfolio value, an equal percentage across all assets, or a
 * strategy-guided position sizing where the strategy determines the
 * size of the position based on its own logic.
 */
typedef enum WU_PositionSizeType {
    WU_POSITION_SIZE_ABS = 0,
    WU_POSITION_SIZE_PCT = 1,
    WU_POSITION_SIZE_PCT_EQUAL = 2,
    WU_POSITION_SIZE_STRATEGY_GUIDED = 3
} WU_PositionSizeType;

/**
 * Position sizing policy. Defines how to determine the size of each
 * trade based on the signal and the portfolio's current state.
 */
typedef struct WU_PositionSizingParams {
    WU_PositionSizeType size_type;
    double size_value;
} WU_PositionSizingParams;

#endif  // _POSITIONS_H
