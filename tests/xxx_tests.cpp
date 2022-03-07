#include <gtest/gtest.h>

inline int simple_add(int a, int b)
{
    return a + b;
}

TEST(varlisp_tests, simple_add)
{
    GTEST_ASSERT_EQ(simple_add(1, 2), 3);
}
