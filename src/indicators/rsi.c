#include <stdlib.h>
#include "wu.h"

static double update(WU_RSI rsi, const WU_Candle* candle) {
    if (!candle) {
        return rsi->value;
    }
    double diff = candle->close - candle->open;
    wu_indicator_update(rsi->gain, diff > 0 ? diff : 0.0);
    wu_indicator_update(rsi->loss, diff < 0 ? -diff : 0.0);
    rsi->value = isnan(wu_indicator_get(rsi->loss))
        ? NAN
        : 100.0 - (100.0 / (1.0 + (wu_indicator_get(rsi->gain) /
                        wu_indicator_get(rsi->loss))));
    return rsi->value;
}

static void delete(WU_RSI rsi) {
    wu_indicator_delete(rsi->gain);
    wu_indicator_delete(rsi->loss);
    free(rsi);
}

WU_RSI wu_rsi_new(int window_size) {
    WU_RSI rsi = malloc(sizeof(struct WU_RSI_));
    rsi->gain = wu_ema_new(window_size, 1.0);
    rsi->loss = wu_ema_new(window_size, 1.0);
    rsi->value = NAN;
    rsi->update = update;
    rsi->delete = delete;
    return rsi;
}
