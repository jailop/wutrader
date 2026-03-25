#include <stdlib.h>
#include <math.h>
#include "wu.h"

static double update(WU_MaxDrawdown self, double portfolio_value) {
    if (portfolio_value > self->peak)
        self->peak = portfolio_value;
    if (self->peak > 0.0) {
        double curr = (portfolio_value - self->peak) / self->peak;
        if (curr < self->value)
            self->value = curr;
    }
    return self->value;
}

static void wu_max_drawdown_free(WU_MaxDrawdown self) {
    free(self);
}

WU_MaxDrawdown wu_max_drawdown_new(void) {
    WU_MaxDrawdown md = malloc(sizeof(struct WU_MaxDrawdown_));
    if (!md) return NULL;
    md->value = 0.0;
    md->peak = 0.0;
    md->update = update;
    md->delete = wu_max_drawdown_free;
    return md;
}
