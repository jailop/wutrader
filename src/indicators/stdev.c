#include <math.h>
#include "wu/indicators.h"

static double update(WU_MStDev stdev, double value) {
    if (isnan(value)) {
        return NAN;
    }
    stdev->value = sqrt(wu_indicator_update(stdev->mvar, value));
    return stdev->value;
}

static void delete(struct WU_MStDev_ *stdev) {
    stdev->mvar->delete(stdev->mvar);
    free(stdev);
}

WU_MStDev wu_stdev_new(int window_size, int dof) {
    WU_MStDev stdev = malloc(sizeof(struct WU_MStDev_));
    stdev->mvar = wu_mvar_new(window_size, dof);
    stdev->update = update;
    stdev->delete = delete;
    return stdev;
}
