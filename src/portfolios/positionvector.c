#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include "wu.h"

// Forward declaration
static void wu_position_vector_free(WU_PositionVector* vec);

static void add_position(WU_PositionVector* vec, WU_Position pos) {
    if (!vec || !pos) return;
    int slot = -1;
    for (int i = 0; i < vec->capacity; i++) {
        if (!vec->active[i]) {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        int new_capacity = vec->capacity == 0 ? 4 : vec->capacity * 2;
        vec->positions = realloc(vec->positions, new_capacity * sizeof(struct WU_Position_));
        vec->active = realloc(vec->active, new_capacity * sizeof(bool));
        for (int i = vec->capacity; i < new_capacity; i++) {
            vec->active[i] = false;
        }
        slot = vec->capacity;
        vec->capacity = new_capacity;
    }
    // Copy the position value into the array
    vec->positions[slot] = *pos;
    vec->active[slot] = true;
    vec->count++;
}

static void remove_position(WU_PositionVector* vec, int index) {
    if (!vec || index < 0 || index >= vec->capacity) return;
    if (!vec->active[index]) return;
    vec->active[index] = false;
    vec->count--;
}

static void clear_positions(WU_PositionVector* vec) {
    if (!vec) return;
    for (int i = 0; i < vec->capacity; i++) {
        vec->active[i] = false;
    }
    vec->count = 0;
}

static struct WU_Position_ get_position(WU_PositionVector* vec, int index, bool* found) {
    struct WU_Position_ empty = {0};
    if (!vec || index < 0 || index >= vec->capacity || !vec->active[index]) {
        if (found) *found = false;
        return empty;
    }
    if (found) *found = true;
    return vec->positions[index];
}

static double get_total_quantity(WU_PositionVector* vec) {
    if (!vec) return 0.0;
    double total = 0.0;
    for (int i = 0; i < vec->capacity; i++) {
        if (vec->active[i]) {
            total += vec->positions[i].quantity;
        }
    }
    return total;
}

WU_PositionVector* wu_position_vector_new(const char* symbol) {
    WU_PositionVector* vec = malloc(sizeof(WU_PositionVector));
    if (!vec) return NULL;
    vec->symbol = strndup(symbol ? symbol : "", WU_SYMBOL_MAX_LEN); 
    vec->last_price = 0.0;
    vec->positions = NULL;
    vec->active = NULL;
    vec->count = 0;
    vec->capacity = 0;
    vec->add = add_position;
    vec->remove = remove_position;
    vec->clear = clear_positions;
    vec->get = get_position;
    vec->total_quantity = get_total_quantity;
    vec->delete = wu_position_vector_free;
    return vec;
}

static void wu_position_vector_free(WU_PositionVector* vec) {
    if (!vec) return;
    free(vec->symbol);
    free(vec->positions);
    free(vec->active);
    free(vec);
}
