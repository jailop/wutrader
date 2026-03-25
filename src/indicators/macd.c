#include <stdlib.h>
#include "wu.h"

static WU_MACDResult macd_update(WU_MACD macd, double value) {
    if (isnan(value)) {
        return macd->value;
    }
    macd->len++;
    double ema_short = wu_indicator_update(macd->ema_short, value);
    double ema_long = wu_indicator_update(macd->ema_long, value);
    if (macd->len <= macd->start) {
        return macd->value;
    }
    double diff = ema_short - ema_long;
    double signal = wu_indicator_update(macd->signal_line, diff);
    macd->value = (WU_MACDResult){.macd = diff, .signal = signal, .histogram = diff - signal};
    return macd->value;
}

static void macd_free(WU_MACD macd) {
    wu_indicator_delete(macd->ema_short);
    wu_indicator_delete(macd->ema_long);
    wu_indicator_delete(macd->signal_line);
    free(macd);
}

WU_MACD wu_macd_new(int short_window, int long_window, int signal_window, 
        double smoothing) {
    WU_MACD macd = malloc(sizeof(struct WU_MACD_));
    macd->ema_short = wu_ema_new(short_window, smoothing);
    macd->ema_long = wu_ema_new(long_window, smoothing);
    macd->signal_line = wu_ema_new(signal_window, smoothing);
    macd->update = macd_update;
    macd->delete = macd_free;
    macd->value = (WU_MACDResult){.macd = NAN, .signal = NAN, .histogram = NAN};
    macd->len = 0;
    macd->start = long_window > short_window ? long_window : short_window;
    return macd;
}
