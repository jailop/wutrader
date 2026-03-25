#include "wu/indicators.h"

static double update(WU_Mean self, double value) {
    if (isnan(value)) return NAN;
    self->count++;
    self->accum += value;
    self->value = self->accum / self->count;
    return self->value;
}

static void delete(WU_Mean self) {
    free(self);
}

WU_Mean wu_mean_new(void) {
    WU_Mean m = malloc(sizeof(struct WU_Mean_));
    if (!m) return NULL;
    m->count = 0;
    m->accum = 0.0;
    m->value = NAN;
    m->update = update;
    m->delete = delete;
    return m;
}
