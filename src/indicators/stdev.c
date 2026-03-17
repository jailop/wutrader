#include <math.h>
#include "wu/indicators.h"

static inline double update(WU_StDev stdev, double value) {
    if (isnan(value)) {
        return NAN;
    }
    return sqrt(wu_indicator_update(stdev->mvar, value));
}

static inline double get(const struct WU_StDev_ *stdev) {
    return sqrt(stdev->mvar->get(stdev->mvar));
}

static void delete(struct WU_StDev_ *stdev) {
    stdev->mvar->delete(stdev->mvar);
    free(stdev);
}

WU_StDev wu_stdev_new(int window_size, int dof) {
    WU_StDev stdev = malloc(sizeof(struct WU_StDev_));
    stdev->mvar = wu_mvar_new(window_size, dof);
    stdev->update = update;
    stdev->get = get;
    stdev->delete = delete;
    return stdev;
}
