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
  fany.Then([]( Try<std::pair<size_t, int>> result) {
    std::cerr << "Result " << result.Value().first << " = " << result.Value().second << std::endl;
    EXPECT_LT(result.Value().second, 18);
  });

  for (auto& t : threads)
    t.join();
}

TEST(future_then, then_void)
{
  Promise<int> promise;
  auto future = promise.GetFuture();
  auto f = future.Then([](int x){
    EXPECT_EQ(x, 1);
  });

  promise.SetValue(1);
  f.Get();
}

TEST(future_exception, async_ommit_exception) {
  auto future = Async([] {
    throw std::runtime_error("");
    return 1;
  });

  auto f = future
               .Then([](Try<int> t) {
                 if (t.HasException()) {
                   std::cout << "has exception\n";
                 }

                 return 42;
               })
               .Then([](int i) {
                 return i + 2;
               });

  EXPECT_EQ(f.Get(), 44);
}

TEST(future_exception, async_exception) {
  auto future = Async([] {
    throw std::runtime_error("");
    return 1;
  });

  auto f = future
               .Then([](Try<int> t) {
                 if (t.HasException()) {
                   std::cout << "has exception\n";
                 }

                 return t + 42;
               })
               .Then([](int i) { return i + 2; });

  EXPECT_THROW(f.Get(), std::exception);
}

TEST(future_exception, value_ommit_exception){
  Promise<int> promise;
  auto future = promise.GetFuture();
  auto f = future.Then([](int x){
    throw std::runtime_error("error");
    return x+2;
  }).Then([](Try<int> y){
    if(y.HasException()){
      std::cout<<"has exception\n";
    }

    return 2;
  });

  promise.SetValue(1);

  EXPECT_EQ(f.Get(), 2);
}

TEST(future_exception, value_exception){
  Promise<int> promise;
  auto future = promise.GetFuture();
  auto f = future.Then([](int x){
    throw std::runtime_error("error");
    return x+2;
  }).Then([](int y){
    return y+2;
  });

  promise.SetValue(1);

  EXPECT_THROW(f.Get(), std::exception);
}

TEST(when_all, when_all_vector){
  Promise<int> p1;
  Promise<int> p2;
  std::vector<Future<int> > futures;
  futures.emplace_back(p1.GetFuture());
  futures.emplace_back(p2.GetFuture());

  auto future = WhenAll(futures.begin(), futures.end());
  p1.SetValue(42);
  p2.SetValue(21);
  try {
    auto result = future.Get();
    auto& r1 = result[0];
    auto& r2 = result[1];

    EXPECT_EQ(r1, 42);
    EXPECT_EQ(r2, 21);
  }catch(std::exception& e){
    Print(e.what());
  }
}

std::atomic<int> g_val = {0};
TEST(when_all, when_all_in_thread){
  std::vector<std::thread> threads;
  std::vector<Promise<int> > pmv(8);
  for (auto& pm : pmv) {
    std::thread t([&pm]{
      g_val++;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      pm.SetValue(g_val);
    });
    threads.emplace_back(std::move(t));
  }

  std::vector<Future<int> > futures;
  for (auto& pm : pmv) {
    futures.emplace_back(pm.GetFuture());
  }

  auto fall = WhenAll(std::begin(futures), std::end(futures));
  fall.Then([]( Try<std::vector<int>> result) {
    EXPECT_EQ(result.Value().size(), 8);
  });

  for (auto& t : threads)
    t.join();
}

TEST(when_all, when_all_variadic){
  Promise<int> p1;
  Promise<void> p2;

  auto f1 = p1.GetFuture();
  auto f2 = p2.GetFuture();

  auto future = WhenAll(f1, f2);
  p1.SetValue(42);
  p2.SetValue();

  auto f = future.Then([](Try<std::tuple<Try<int>, Try<void>>>&& t){
    assert(std::get<0>(t.Value()).HasValue());
    auto result = t.Value();
    auto& r1 = std::get<0>(result);
    auto& r2 = std::get<1>(result);

    EXPECT_EQ(r1.Value(), 42);
    EXPECT_TRUE(r1.HasValue());
  });

  f.Get();
}

TEST(when_all, when_all_variadic_same){
  Promise<int> p1;
  Promise<int> p2;

  auto f1 = p1.GetFuture();
  auto f2 = p2.GetFuture();

  auto future = WhenAll(f1, f2);
  p1.SetValue(42);
  p2.SetValue(21);

  auto f = future.Then([](Try<std::tuple<Try<int>, Try<int>>>&& t){
    auto result = t.Value();
    auto r1 = std::get<0>(result);
    auto r2 = std::get<1>(result);

    EXPECT_EQ(r1.Value(), 42);
    EXPECT_EQ(r2.Value(), 21);
  });

  f.Get();
}

TEST(when_all, when_all_variadic_get){
  Promise<int> p1;
  Promise<void> p2;

  auto f1 = p1.GetFuture();
  auto f2 = p2.GetFuture();

  auto future = WhenAll(f1, f2);
  p1.SetValue(42);
  p2.SetValue();

  auto result = future.Get();
  assert(std::get<0>(future.Get()).HasValue());
  assert(std::get<0>(result).HasValue());
  auto& r1 = std::get<0>(result);
  auto& r2 = std::get<1>(result);

  EXPECT_EQ(r1.Value(), 42);
  EXPECT_TRUE(r1.HasValue());
}

TEST(try_get, try_get_val){
  Try<int> t;
  EXPECT_THROW(t.Value(), std::exception);

  Try<void> t1;
  EXPECT_TRUE(t1.HasValue());
  EXPECT_FALSE(t1.HasException());

  Try<void> t2({std::exception_ptr()});
  EXPECT_TRUE(t2.HasException());
  EXPECT_FALSE(t2.HasValue());
}

TEST(promise_set, set_val){
  Promise<int> promise;
  auto future = promise.GetFuture();
  promise.SetValue(1);
  promise.SetValue(2);
  EXPECT_EQ(future.Get(), 1);
}

TEST(ready_future, make_ready){
  Future<int> future = MakeReadyFuture(2);
  EXPECT_EQ(future.Get(), 2);
  EXPECT_TRUE(future.Valid());

  Future<void> vfuture = MakeReadyFuture();
  EXPECT_TRUE(vfuture.Valid());
  EXPECT_TRUE(std::is_void<decltype(vfuture.Get())>::value);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
