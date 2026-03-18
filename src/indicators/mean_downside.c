#include <stdlib.h>
#include <math.h>
#include "wu/indicators.h"

static double update(WU_Downside self, double value) {
    if (isnan(value)) return NAN;
    self->count++;
    if (value < 0.0) {
        self->downside_m2 += value * value;
    }
    if (self->count == 0) {
        self->value = NAN;
    } else {
        self->value = sqrt(self->downside_m2 / self->count);
    }
    return self->value;
}

static void delete(WU_Downside self) {
    free(self);
}

WU_Downside wu_downside_new(void) {
    WU_Downside d = malloc(sizeof(struct WU_Downside_));
    if (!d) return NULL;
    d->downside_m2 = 0.0;
    d->count = 0;
    d->value = NAN;
    d->update = update;
    d->delete = delete;
    return d;
}
