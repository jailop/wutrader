#ifndef _TIMEUTILS_H
#define _TIMEUTILS_H

#include <stdint.h>

/**
 * WU_TimeUnit represents a unit of time used for time-based calculations
 * such as time-weighted returns or time-based aggregations.
 */
typedef enum {
    WU_TIME_UNIT_SECONDS = 0,
    WU_TIME_UNIT_MILLIS = 1,
    WU_TIME_UNIT_MICROS = 2,
    WU_TIME_UNIT_NANOS = 3
} WU_TimeUnit;

/**
 * A timestamp represent a mark in time given relative to unix epoch.
 * It can be expressed in seconds, millis, micros, or nanos.
 */
typedef struct WU_TimeStamp_ {
    int64_t mark;
    WU_TimeUnit units;
} WU_TimeStamp;

/**
 * Returns the number of units (seconds, millis, micros, or nanos)
 * contained in a year.
 */
double wu_annualization_factor(WU_TimeUnit unit);

#endif // _TIMEUTILS_H
