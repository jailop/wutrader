#include <stdlib.h>
#include "wu/indicators.h"

/**
 * The variance is compute based on the difference between
 * \sum x^2 - n * mean. Degrees of freedom are considered to decided
 * when valid values start generating.
 */
static double var_update(WU_Var self, double value) {
    if (isnan(value)) return NAN;
    self->count++;
    self->sum2 += value * value;
    double mean = wu_indicator_update(self->mean, value);
    if (self->count <= self->dof) return NAN;
    self->value = (self->sum2 - self->count * mean * mean)
        / (self->count - self->dof);
    return self->value;
}

static void var_delete(struct WU_Var_* self) {
    wu_indicator_delete(self->mean);
    free(self);
}

WU_Var wu_var_new(int dof) {
    WU_Var var = malloc(sizeof(struct WU_Var_));
    if (!var) return NULL;
    var->update = var_update;
    var->delete = var_delete;
    var->value = NAN;
    var->mean = wu_mean_new();
    var->sum2 = 0.0;
    var->dof = dof;
    var->count = 0;
    return var;
}

static double stdev_update(WU_StDev self, double value) {
    double var = wu_indicator_update(self->var, value);
    self->value = isnan(var) ? NAN : sqrt(var);
    return self->value;
}

static void stdev_delete(struct WU_StDev_* self) {
    wu_indicator_delete(self->var);
    free(self);
}

WU_StDev wu_stdev_new(int dof) {
    WU_StDev stdev = malloc(sizeof(struct WU_StDev_));
    if (!stdev) return NULL;
    stdev->var = wu_var_new(dof);
    if (!stdev->var) {
        free(stdev);
        return NULL;
    }
    stdev->update = stdev_update;
    stdev->delete = stdev_delete;
    stdev->value = NAN;
    return stdev;
}
