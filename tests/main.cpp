#include <iostream>
#include <gtest/gtest.h>
#include "../future/future.h"

using namespace ray;

template<typename... Args>
inline void Print(Args&&... args) {
  (void)std::initializer_list<int>{(std::cout << std::forward<Args>(args) << ' ', 0)...};
  std::cout << "\n";
}

TEST(future_then, basic_then)
{
  Promise<int> promise;
  auto future = promise.GetFuture();
  auto f = future.Then([](int x){
    Print(std::this_thread::get_id());
    return x+2;
  }).Then([](int y){
    Print(std::this_thread::get_id());
    return y+2;
  }).Then([](int z){
    Print(std::this_thread::get_id());
    return z+2;
  });

  promise.SetValue(2);
  EXPECT_EQ(f.Get(), 8);
}

TEST(future_then, async_then)
{
  auto future = Async([]{return 2;}).Then([](int x){return x+2;}).Then([](int x){ return x+2; });
  EXPECT_EQ(future.Get(), 6);
}

TEST(when_any, any)
{
  std::vector<std::thread> threads;
  std::vector<Promise<int> > pmv(8);
  for (auto& pm : pmv) {
    std::thread t([&pm]{
      static int val = 10;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      pm.SetValue(val++);
    });
    threads.emplace_back(std::move(t));
  }

  std::vector<Future<int> > futures;
  for (auto& pm : pmv) {
    futures.emplace_back(pm.GetFuture());
  }

  auto fany = WhenAny(std::begin(futures), std::end(futures));
  fany.Then([]( Try<std::pair<size_t, int>> result) {//not support now
    std::cerr << "Then collet int any!\n";
    std::cerr << "Result " << result.Value().first << " = " << result.Value().second << std::endl;
    return 0;
  });

  for (auto& t : threads)
    t.join();
}

TEST(future_then, then_void)
{
  Promise<int> promise;
  auto future = promise.GetFuture();
//  auto f = future.Then([](Try<int> x){
//    std::cout<<x<<'\n';
//  });

  auto f1 = future.Then([](int x){
    std::cout<<x<<'\n';
  });

  promise.SetValue(1);
//  int r = future.Get();
  f1.Get();
//  EXPECT_EQ(test(nullptr), -1);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
