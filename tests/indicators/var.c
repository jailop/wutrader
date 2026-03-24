#include <CUnit/CUnit.h>
#include <math.h>
#include "wu.h"

#define ERROR 1e-6
#define APPROX(x, y) (fabs((x) - (y)) < ERROR)

void test_var_dof1(void) {
    WU_Var var = wu_var_new(1);
    CU_ASSERT_TRUE(isnan(wu_indicator_get(var)));
    CU_ASSERT_TRUE(isnan(wu_indicator_update(var, 5.0)));
    CU_ASSERT_TRUE(APPROX(wu_indicator_update(var, 6.5), 1.125));
    CU_ASSERT_TRUE(APPROX(wu_indicator_update(var, 7.9), 2.103333333));
    CU_ASSERT_TRUE(APPROX(wu_indicator_update(var, 4.0), 2.923333333));
    CU_ASSERT_TRUE(APPROX(wu_indicator_update(var, -3.0), 17.857));
    CU_ASSERT_TRUE(APPROX(wu_indicator_update(var, 22.0), 67.80666667));
    wu_indicator_delete(var);
}

void test_var_dof0(void) {
    WU_Var var = wu_var_new(0);
    CU_ASSERT_TRUE(isnan(wu_indicator_get(var)));
    CU_ASSERT_TRUE(APPROX(wu_indicator_update(var, 5.0), 0.0));
    CU_ASSERT_TRUE(APPROX(wu_indicator_update(var, 6.5), 0.5625));
    CU_ASSERT_TRUE(APPROX(wu_indicator_update(var, 7.9), 1.402222222));
    CU_ASSERT_TRUE(APPROX(wu_indicator_update(var, 4.0), 2.1925));
    CU_ASSERT_TRUE(APPROX(wu_indicator_update(var, -3.0), 14.2856));
    CU_ASSERT_TRUE(APPROX(wu_indicator_update(var, 22.0), 56.50555556));
    wu_indicator_delete(var);
}

void test_stdev_dof1(void) {
    WU_StDev stdev = wu_stdev_new(1);
    CU_ASSERT_TRUE(isnan(wu_indicator_get(stdev)));
    CU_ASSERT_TRUE(isnan(wu_indicator_update(stdev, 5.0)));
    CU_ASSERT_TRUE(APPROX(wu_indicator_update(stdev, 6.5), 1.060660172));
    CU_ASSERT_TRUE(APPROX(wu_indicator_update(stdev, 7.9), 1.450287328));
    CU_ASSERT_TRUE(APPROX(wu_indicator_update(stdev, 4.0), 1.709775814));
    CU_ASSERT_TRUE(APPROX(wu_indicator_update(stdev, -3.0), 4.225754371));
    CU_ASSERT_TRUE(APPROX(wu_indicator_update(stdev, 22.0), 8.234480352)); 
    wu_indicator_delete(stdev);
}
