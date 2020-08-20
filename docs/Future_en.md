# An Asynchronous Parallel Future

# outline

* [motiviation](#motiviation)
* [basic concepts](#basic concepts)
* [overview of Future](#overview of Future)
* [quick example](#quick example)
* [create a future](#create a future)
* [get the value of a future](#get the value of a future)
* [thread policy](#thread policy)
* [threadpool adaptor](#threadpool adaptor)
* [exception handling](#exception handling)
* [WhenAll variadic version](#WhenAll variadic version)
* [roadMap](#roadmap)

# motiviation

1. the problem of std::future

    std::future can't provide continuable call, can't support asynchronous parallel applications.

2. avoid [callback hell](https://en.wiktionary.org/wiki/callback_hell)

So we need to extend std::future to support continuable call. This the motiviation of [purecpp.Future](https://github.com/qicosmos/future_demo/blob/master/future/future.h).

# basic concepts

## future
A future value not calculated yet.

## promise

The creator of a future, Each [promise](https://en.cppreference.com/w/cpp/thread/promise)
is associated with a shared state, which contains some state information and a result which may be not yet evaluated, 
evaluated to a value (possibly void) or evaluated to an exception. 

## Try<T>
Represent value T or an exception, it used to handle exception when call continuable.

# overview of Future
```c++

Promise

Future

MakeReadyFuture

Async

Then
       
WhenAll

WhenAny

Finally

```

# quick example

## future-then

```
  auto future = Async([]{
      return 42;
  }).Then([](int i){
    return i + 2;
  }).Then([](int x){
    return std::to_string(x);
  });

  std::string str = future.Get(); //44
```

## use threadpool
```
  boost::basic_thread_pool pool(4);
  auto future = Async(&pool, []{
    return 42;
  });

  auto f = future.Then(&pool, [](int i){
    return i+2;
  }).Then(&pool, [](int i){
    return i +2;
  }).Then([](int i){
    return i+2;
  });

  EXPECT_EQ(f.Get(), 48);
```

## WhenAll
```
  std::vector<Future<int> > futures;
  futures.emplace_back(Async([]{return 42;}));
  futures.emplace_back(Async([]{return 21;}));

  auto future = WhenAll(futures.begin(), futures.end());
  std::vector<int> result = future.Get();
  auto r1 = result[0];
  auto r2 = result[1];

  EXPECT_EQ(r1, 42);
  EXPECT_EQ(r2, 21);
```

## WhenAny
```
  std::vector<Future<int> > futures;
  futures.emplace_back(Async([]{return 42;}));
  futures.emplace_back(Async([]{return 21;}));

  auto future = WhenAny(futures.begin(), futures.end());
  std::pair<size_t, int> result = future.Get();
  auto which_one = result.first;
  auto value = result.second;

  EXPECT_TRUE((which_one == 0) || (which_one == 1));
  EXPECT_TRUE((value == 42) || (value == 21));
```

## finally
```
    auto future = Async([] {
      std::this_thread::sleep_for(std::chrono::milliseconds (50));
      return 2;
    });

    //Finally callback, no block here
    future.Finally([](int i){
      EXPECT_EQ(i, 2);
    });
```

# create a future

## creat a future with a thread
```c++
  Future<int> f1 = Async([]{return 42;});
  EXPECT_EQ(f1.Get(), 42);

  Future<int> f2 = Async([](int i){return i + 2; }, 42);
  EXPECT_EQ(f2.Get(), 44);

  int GetVal(int i){
    return i + 2;
  }

  Future<int> f3 = Async(&GetVal, 42);
  EXPECT_EQ(f3.Get(), 44);
```

## create a future with a promise
```c++
  Promise<int> promise;
  Future<int> future = promise.GetFuture();
  promise.SetValue(42);
  EXPECT_EQ(future.Get(), 42);
```

## create a future with a value
```c++
  Future<int> future = MakeReadyFuture(2);
  EXPECT_EQ(future.Get(), 2);
  EXPECT_TRUE(future.Valid());

  Future<void> void_future = MakeReadyFuture();
  EXPECT_TRUE(vfuture.Valid());
```

# get the value of a future

## wait
```c++
  auto future = Async([]{
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return 1;
  });

  EXPECT_EQ(future.Get(), 1);
```

## wait for
```c++
  auto future = Async([]{
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return 1;
  });

  auto status = future.WaitFor(std::chrono::milliseconds(20));
  EXPECT_EQ(status, FutureStatus::Timeout);
  EXPECT_THROW(future.Get(), std::exception);
```

## wait until
```c++
    auto future = Async([]{
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      return 1;
    }).Then([](int i){
      return i+2;
    });

    auto now = std::chrono::system_clock::now();
    auto status = future.WaitUntil(now + std::chrono::milliseconds(20));
    EXPECT_THROW(future.Get(), std::exception);
    EXPECT_EQ(status, FutureStatus::Timeout);
```

# thread policy

## async future in a thread
```c++
  auto future = Async([]{
          return 42;
  });
  EXPECT_EQ(future.Get(), 42);
```

## async future in a threadpool
```c++
  boost::basic_thread_pool pool(4);
  auto future = Async(&pool, []{
    return 42;
  });

  EXPECT_EQ(future.Get(), 42);
```

## then in the same thread
```c++
  auto future = MakeReadyFuture(std::this_thread::get_id());
  auto f = future.Then(Lauch::Sync, [](std::thread::id id){
    return id==std::this_thread::get_id();
  });

  EXPECT_TRUE(f.Get());
```

## then in another thread
```c++
  auto future = Async([]{
      return 42;
  }).Then([](int i){
    return i + 2;
  }).Then([](int x){
    return std::to_string(x);
  });

  std::string str = future.Get();
```

## then in a threadpool
```c++
  boost::basic_thread_pool pool(4);
  auto future = Async(&pool, []{
    return 42;
  });

  auto f = future.Then(&pool, [](int i){
    return i+2;
  }).Then(&pool, [](int i){
    return i +2;
  }).Then([](int i){
    return i+2;
  });

  EXPECT_EQ(f.Get(), 48);
```
# threadpool adaptor
If you want to use a thirdparty threadpool in future-then, you could create a adaptor for the threadpool.

```c++
  //a custom adaptor
  template <typename E> struct ExecutorAdaptor {
    ExecutorAdaptor(const ExecutorAdaptor &) = delete;
    ExecutorAdaptor &operator=(const ExecutorAdaptor &) = delete;

    template <typename... Args>
    ExecutorAdaptor(Args &&... args) : ex(std::forward<Args>(args)...) {}

    void submit(std::function<void()> f) {
      ex.submit(std::move(f));
    }

    E ex;
  };

  //usage of the adaptor
  ExecutorAdaptor<boost::basic_thread_pool> ex(4);

  auto future = Async(&ex, []{
    return 42;
  }).Then(&ex, [](int i){
    return i+2;
  });

  EXPECT_EQ(future.Get(), 44);
```

# exception handling
Two ways to handle excpetion:
1. catch the exception, omit the exception or pass it to the next continuable;

```c++
//omit the exception and continue then
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
```

2. throw the exception and terminate then
```c++
  Promise<int> promise;
  auto future = promise.GetFuture();
  auto f = future.Then([](int x){
    throw std::runtime_error("error");
    return x+2;
  }).Then([](int x){
    return x;
  });

  promise.SetValue(1);

  EXPECT_THROW(f.Get(), std::exception);
```

# WhenAll variadic version

Pass multiple different types of futures to WhenAll and return a multiple future.
```c++
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
```

# roadmap
1. support more asynchronous function of future;
2. support build future expression;
3. support build future graph
4. support coroutine

# reference
1. [boost.future](https://www.boost.org/doc/libs/1_73_0/doc/html/thread/synchronization.html#thread.synchronization.futures.then)
2. [folly.future](https://github.com/facebook/folly)
3. [advance future](http://www.home.hs-karlsruhe.de/~suma0002/publications/advanced-futures-promises-cpp.pdf)
4. [Continuable](https://meetingcpp.com/mcpp/slides/2018/Continuable.pdf)
5. [ananas.future](https://github.com/loveyacper/ananas/tree/master/future)