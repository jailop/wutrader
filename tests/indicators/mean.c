#include <CUnit/CUnit.h>
#include <math.h>
#include "wu.h"

void test_mean(void) {
    WU_Mean mean = wu_mean_new();
    CU_ASSERT_TRUE(isnan(wu_indicator_get(mean)));
    CU_ASSERT_TRUE(wu_indicator_update(mean, 5.0) == 5.0);
    CU_ASSERT_TRUE(wu_indicator_update(mean, 6.5) == 5.75);
    CU_ASSERT_TRUE(wu_indicator_update(mean, 7.9) == 19.4 / 3.0);
    CU_ASSERT_TRUE(wu_indicator_update(mean, 4.0) == 5.85);
    CU_ASSERT_TRUE(wu_indicator_update(mean, -3.0) == 4.08);
    CU_ASSERT_TRUE(wu_indicator_update(mean, 22.0) == 42.4 / 6.0);
    wu_indicator_delete(mean);
}
