#include <iostream>
#include <gtest/gtest.h>
#include "../future/future.h"

using namespace ray;

TEST(test_my_class, get_age)
{
  EXPECT_EQ(1, 1);
  EXPECT_TRUE(3 > 0);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
