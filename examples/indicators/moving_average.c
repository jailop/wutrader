#include <wu.h>
#include <stdio.h>

int main() {
    WU_SMA ma = wu_sma_new(3);
    wu_indicator_update(ma, 1.0);
    wu_indicator_update(ma, 2.0);
    wu_indicator_update(ma, 3.0);
    printf("Moving Average: %f\n", wu_indicator_get(ma));
    wu_indicator_delete(ma);
    return 0;
}
