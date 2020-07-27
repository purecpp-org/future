#include <iostream>
#include <gtest/gtest.h>
#include "../future/future.h"

using namespace ray;

int test(int* p){
  if(p==nullptr){
    return -1;
  }

  if(*p==1){
    return 1;
  }else{
    return -2;
  }
}

TEST(dummy, dummy_test)
{
  EXPECT_EQ(test(nullptr), -1);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
