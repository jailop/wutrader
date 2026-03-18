#include "wu/timeutils.h"

double wu_annualization_factor(WU_TimeUnit unit) {
    switch (unit) {
        case WU_TIME_UNIT_SECONDS:    return 365.25 * 24 * 3600;
        case WU_TIME_UNIT_MILLIS:     return 365.25 * 24 * 3600 * 1000;
        case WU_TIME_UNIT_MICROS:     return 365.25 * 24 * 3600 * 1e6;
        case WU_TIME_UNIT_NANOS:      return 365.25 * 24 * 3600 * 1e9;
        default:                      return 365.25;
    }
}
