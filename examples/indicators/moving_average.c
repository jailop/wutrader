#include <wu.h>
#include <stdio.h>

int main() {
    MovingAverage ma = moving_average_new(3);
    indicator_update(ma, 1.0);
    indicator_update(ma, 2.0);
    indicator_update(ma, 3.0);
    printf("Moving Average: %f\n", DOUBLE(ma)); // Should print 2.0
    indicator_delete(ma);
    return 0;
}
