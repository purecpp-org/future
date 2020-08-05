//
// Created by qicosmos on 2020/7/22.
//

#ifndef FUTURE_DEMO_FUTURE_H
#define FUTURE_DEMO_FUTURE_H

#include "helper.h"
#include "try.h"
#include "shared_state.h"
#include "promise.h"

namespace ray {

enum class Lauch { Async, Sync, Callback };

struct EmptyExecutor {
  template <typename F> void submit(F &&fn) {}
};

template <typename E> struct ExecutorAdaptor {
  ExecutorAdaptor(const ExecutorAdaptor &) = delete;
  ExecutorAdaptor &operator=(const ExecutorAdaptor &) = delete;

  template <typename... Args>
  ExecutorAdaptor(Args &&... args) : ex(std::forward<Args>(args)...) {}

  void submit(std::function<void()> f) { ex.submit(std::move(f)); }

  E ex;
};

template <typename T> class Future {
public:
  using InnerType = T;
  Future() = default;
  Future(const Future &) = delete;
  void operator=(const Future &) = delete;

  Future(Future &&fut) = default;
  Future &operator=(Future &&fut) = default;

  explicit Future(std::shared_ptr<SharedState<T>> state)
      : shared_state_(std::move(state)) {}

  bool Valid() const { return shared_state_ != nullptr; }

  template <typename F>
  Future<typename function_traits<F>::return_type> Then(F &&fn) {
    return Then(Lauch::Async, std::forward<F>(fn));
  }

  template <typename F>
  Future<absl::result_of_t<
      typename std::decay<F>::type(typename TryWrapper<T>::type)>>
  Then(Lauch policy, F &&fn) {
    return ThenImpl(policy, (EmptyExecutor *)nullptr, std::forward<F>(fn));
  }

  template <typename F, typename Ex>
  Future<absl::result_of_t<
      typename std::decay<F>::type(typename TryWrapper<T>::type)>>
  Then(Ex *executor, F &&fn) {
    return ThenImpl(Lauch::Async, executor, std::forward<F>(fn));
  }

  T Get() {
    {
      std::unique_lock<std::mutex> lock(shared_state_->then_mtx_);
      switch (shared_state_->state_) {
      case FutureStatus::None:
        break;
      case FutureStatus::Timeout:
        throw std::runtime_error("timeout");
      case FutureStatus::Done:
        shared_state_->state_ = FutureStatus::Retrived;
        return GetImpl<T>();
      default:
        throw std::runtime_error("already retrieved");
      }
    }
    shared_state_->Wait();
    std::unique_lock<std::mutex> lock(shared_state_->then_mtx_);
    return GetImpl<T>();
  }

  template <typename Rep, typename Period>
  FutureStatus
  WaitFor(const std::chrono::duration<Rep, Period> &timeout_duration) const {
    return shared_state_->WaitFor(timeout_duration);
  }

  template <typename Clock, typename Duration>
  FutureStatus WaitUntil(
      const std::chrono::time_point<Clock, Duration> &timeout_time) const {
    return shared_state_->WaitUntil(timeout_time);
  }

  void Wait() { shared_state_->Wait(); }

  template <typename F>
  void Finally(F&& fn){
    Then(Lauch::Callback, std::forward<F>(fn));
  }

private:
  template <typename FirstArg, typename F, typename Executor, typename U>
  void ExecuteTask(Lauch policy, Executor *executor, MoveWrapper<F> func,
                          MoveWrapper<Promise<U>> next_prom,
                          std::shared_ptr<SharedState<T>> const &state);

  template <typename F, typename Ex>
  Future<typename function_traits<F>::return_type>
  ThenImpl(Lauch policy, Ex *executor, F &&fn) {
    static_assert(function_traits<F>::arity <= 1,
                  "Then must take zero or one argument");
    using FirstArg = typename function_traits<F>::first_arg_t;
    using return_type = typename function_traits<F>::return_type;
    Promise<return_type> next_promise;
    auto next_future = next_promise.GetFuture();

    auto func = MakeMoveWrapper(std::move(fn));
    auto next_prom = MakeMoveWrapper(std::move(next_promise));
    auto state = shared_state_;

    std::unique_lock<std::mutex> lock(shared_state_->then_mtx_);
    if (shared_state_->state_ == FutureStatus::None) {
      shared_state_->continuations_.emplace_back(
          [policy, executor, func, next_prom, state, this]() mutable {
            ExecuteTask<FirstArg>(policy, executor, func, next_prom, state);
          });
    } else if (shared_state_->state_ == FutureStatus::Done) {
      lock.unlock();
      ExecuteTask<FirstArg>(policy, executor, func, next_prom, shared_state_);
    } else if (shared_state_->state_ == FutureStatus::Timeout) {
      throw std::runtime_error("timeout");
    }

    return next_future;
  }

  template <typename R> absl::enable_if_t<std::is_void<R>::value> GetImpl() {}

  template <typename R>
  absl::enable_if_t<!std::is_void<R>::value, T> GetImpl() {
    return std::move(shared_state_->value_.Value());
  }

  template <typename R, typename U, typename F, typename Arg>
  static auto Invoke(F fn, Arg arg) ->
      typename std::enable_if<!IsTry<U>::value && std::is_same<void, U>::value,
                              try_type_t<decltype(fn())>>::type {
    using type = decltype(fn());
    return try_type_t<type>(fn());
  }

  template <typename U, typename F, typename Arg>
  static auto Invoke(F fn, Arg arg) -> typename std::enable_if<
      !IsTry<U>::value && !std::is_same<void, U>::value &&
          !std::is_void<typename function_traits<F>::return_type>::value,
      try_type_t<decltype(fn(arg.Value()))>>::type {
    using type = decltype(fn(arg.Value()));
    return try_type_t<type>(fn(arg.Value()));
  }

  template <typename U, typename F, typename Arg>
  static auto Invoke(F fn, Arg arg) -> typename std::enable_if<
      !IsTry<U>::value && !std::is_same<void, U>::value &&
          std::is_void<typename function_traits<F>::return_type>::value,
      try_type_t<decltype(fn(arg.Value()))>>::type {
    using type = decltype(fn(arg.Value()));
    fn(arg.Value());
    return try_type_t<type>();
  }

  template <typename U, typename F, typename Arg>
  static auto Invoke(F fn, Arg arg) -> typename std::enable_if<
      IsTry<U>::value && !std::is_same<void, typename IsTry<U>::Inner>::value &&
          !std::is_void<typename function_traits<F>::return_type>::value,
      try_type_t<decltype(fn(std::move(arg)))>>::type {
    using type = decltype(fn(std::move(arg)));
    return try_type_t<type>(fn(std::move(arg)));
  }

  template <typename U, typename F, typename Arg>
  static auto Invoke(F fn, Arg arg) -> typename std::enable_if<
      IsTry<U>::value && !std::is_same<void, typename IsTry<U>::Inner>::value &&
          std::is_void<typename function_traits<F>::return_type>::value,
      try_type_t<decltype(fn(std::move(arg)))>>::type {
    using type = decltype(fn(std::move(arg)));
    fn(std::move(arg));
    return try_type_t<type>();
  }

  template <typename U, typename F, typename Arg>
  static auto Invoke(F fn, Arg arg) -> typename std::enable_if<
      IsTry<U>::value && std::is_same<void, typename IsTry<U>::Inner>::value &&
          std::is_void<typename function_traits<F>::return_type>::value,
      try_type_t<decltype(fn(std::move(arg)))>>::type {
    using type = decltype(fn(std::move(arg)));
    fn(std::move(arg));
    return try_type_t<type>();
  }

  template <typename U, typename F, typename Arg>
  static auto Invoke(F fn, Arg arg) -> typename std::enable_if<
      IsTry<U>::value && std::is_same<void, typename IsTry<U>::Inner>::value &&
          !std::is_void<typename function_traits<F>::return_type>::value,
      try_type_t<decltype(fn(std::move(arg)))>>::type {
    using type = decltype(fn(std::move(arg)));
    return try_type_t<type>(fn(std::move(arg)));
  }

  template <typename U, typename F, typename Arg>
  static auto Invoke(F fn, Arg arg) -> typename std::enable_if<
      IsFuture<U>::value &&
          !std::is_same<void, typename IsFuture<U>::Inner>::value,
      try_type_t<decltype(fn(std::move(arg)))>>::type {
    using type = decltype(fn(std::move(arg)));
    return try_type_t<type>(fn(std::move(arg)));
  }

  std::shared_ptr<SharedState<T>> shared_state_;
};

namespace future_internal {
template <typename R, typename P, typename F, typename Tuple>
inline absl::enable_if_t<std::is_void<R>::value> SetValue(P &promise, F &&fn,
                                                          Tuple &&tp) {
  absl::apply(fn, std::move(tp));
  promise.SetValue();
}

template <typename R, typename P, typename F, typename Tuple>
inline absl::enable_if_t<!std::is_void<R>::value> SetValue(P &promise, F &&fn,
                                                           Tuple &&tp) {
  promise.SetValue(absl::apply(fn, std::move(tp)));
}

template <typename F, typename Ex, typename... Args>
Future<typename function_traits<F>::return_type> inline AsyncImpl(
    Lauch policy, Ex *ex, F &&fn, Args &&... args) {
  using R = typename function_traits<F>::return_type;
  Promise<R> promise;
  auto future = promise.GetFuture();
  auto tp = std::forward_as_tuple(std::forward<Args>(args)...);
  auto wrap_tp = MakeMoveWrapper(std::move(tp));

  auto task = [promise, fn, wrap_tp]() mutable {
    try {
      SetValue<R>(promise, std::move(fn), wrap_tp.move());
    } catch (...) {
      promise.SetValue(std::current_exception());
    }
  };

  assert(policy != Lauch::Sync);
  if (ex) {
    ex->submit(std::move(task));
  } else {
    std::thread thd(std::move(task));
    thd.detach();
  }

  return promise.GetFuture();
}
}

// free function of future
template <typename F, typename... Args>
inline Future<
    absl::result_of_t<typename std::decay<F>::type(absl::decay_t<Args>...)>>
Async(F &&fn, Args &&... args) {
  return future_internal::AsyncImpl(Lauch::Async, (EmptyExecutor *)nullptr,
                                    std::forward<F>(fn),
                                    std::forward<Args>(args)...);
}

template <typename F, typename Ex, typename... Args>
inline Future<
    absl::result_of_t<typename std::decay<F>::type(absl::decay_t<Args>...)>>
Async(Ex *ex, F &&fn, Args &&... args) {
  return future_internal::AsyncImpl(Lauch::Async, ex, std::forward<F>(fn),
                                    std::forward<Args>(args)...);
}

template <typename T> inline Future<T> MakeReadyFuture(T &&value) {
  Promise<absl::decay_t<T>> promise;
  promise.SetValue(std::forward<T>(value));
  return promise.GetFuture();
}

inline Future<void> MakeReadyFuture() {
  Promise<void> promise;
  promise.SetValue();
  return promise.GetFuture();
}

template <typename T>
inline Future<T> MakeExceptFuture(std::exception_ptr &&e) {
  Promise<T> promise;
  promise.SetException(std::move(e));

  return promise.GetFuture();
}

template <typename T, typename E> inline Future<T> MakeExceptFuture(E &&e) {
  Promise<T> promise;
  promise.SetException(std::make_exception_ptr(std::forward<E>(e)));

  return promise.GetFuture();
}

template <typename Iterator>
using iterator_value_t = typename std::iterator_traits<Iterator>::value_type;

template <typename T> using future_value_t = typename IsFuture<T>::Inner;

template <typename Iterator>
inline Future<std::pair<size_t, future_value_t<iterator_value_t<Iterator>>>>
WhenAny(Iterator begin, Iterator end) {
  using it_value_type = iterator_value_t<Iterator>;
  using value_t = future_value_t<it_value_type>;

  if (begin == end) {
    return MakeReadyFuture<std::pair<size_t, value_t>>({});
  }

  struct AnyContext {
    AnyContext(){};
    Promise<std::pair<size_t, value_t>> pm;
    std::atomic<bool> done{false};
  };

  auto ctx = std::make_shared<AnyContext>();
  for (size_t i = 0; begin != end; ++begin, ++i) {
    begin->Then([ctx, i](typename TryWrapper<value_t>::type &&t) {
      if (!ctx->done.exchange(true)) {
        ctx->pm.SetValue(std::make_pair(i, std::move(t)));
      }
    });
  }

  return ctx->pm.GetFuture();
}

template <typename Iterator>
inline Future<std::vector<future_value_t<iterator_value_t<Iterator>>>>
WhenAll(Iterator begin, Iterator end) {
  using it_value_type = iterator_value_t<Iterator>;
  using value_t = future_value_t<it_value_type>;

  if (begin == end) {
    return MakeReadyFuture<std::vector<value_t>>({});
  }

  struct AllContext {
    AllContext(int n) : results(n) {}
    Promise<std::vector<value_t>> pm;
    std::vector<value_t> results;
    size_t count{0};
    std::mutex mtx;
  };

  auto ctx = std::make_shared<AllContext>(std::distance(begin, end));

  for (size_t i = 0; begin != end; ++begin, ++i) {
    begin->Then([ctx, i](typename TryWrapper<value_t>::type &&t) {
      std::unique_lock<std::mutex> lock(ctx->mtx);
      ctx->results[i] = std::move(t);
      ctx->count++;
      if (ctx->results.size() == ctx->count) {
        ctx->pm.SetValue(std::move(ctx->results));
      }
    });
  }

  return ctx->pm.GetFuture();
}

namespace internal {
template <typename... F>
class WhenAllContext
    : public std::enable_shared_from_this<WhenAllContext<F...>> {
public:
  template <typename T, typename I> void operator()(T &&f, I i) {
    using value_t =
        typename TryWrapper<typename absl::decay_t<T>::InnerType>::type;
    auto self = this->shared_from_this();
    f.Then([self, this](value_t &&t) {
      std::unique_lock<std::mutex> lock(mtx_);
      count_++;
      std::get<decltype(i)::value>(results_) = std::move(t);
      if (count_ == sizeof...(F)) {
        pm_.SetValue(std::move(results_));
      }
    });
  }

  template <typename Tuple> void for_each(Tuple &&tp) {
    for_each_tp(std::move(tp), *this,
                absl::make_index_sequence<sizeof...(F)>{});
  }

  Future<std::tuple<
      typename TryWrapper<typename absl::decay_t<F>::InnerType>::type...>>
  GetFuture() {
    //    std::unique_lock<std::mutex> lock(mtx_);
    return pm_.GetFuture();
  }

private:
  Promise<std::tuple<
      typename TryWrapper<typename absl::decay_t<F>::InnerType>::type...>>
      pm_;
  std::tuple<typename TryWrapper<typename absl::decay_t<F>::InnerType>::type...>
      results_;
  std::mutex mtx_;
  size_t count_ = 0;
};
}

template <typename... F>
Future<std::tuple<Try<typename absl::decay_t<F>::InnerType>...>>
WhenAll(F &&... futures) {
  static_assert(sizeof...(F) > 0, "at least one argument");
  auto ctx = std::make_shared<internal::WhenAllContext<F...>>();
  ctx->for_each(std::forward_as_tuple(std::forward<F>(futures)...));
  return ctx->GetFuture();
}

template <typename T>
template <typename FirstArg, typename F, typename Executor, typename U>
void Future<T>::ExecuteTask(Lauch policy, Executor *executor,
                            MoveWrapper<F> func,
                            MoveWrapper<Promise<U>> next_prom,
                            std::shared_ptr<SharedState<T>> const &state) {
  auto task = [func, state, next_prom]() mutable {
    try {
      auto result = Invoke<FirstArg>(func.move(), state->value_);
      next_prom->SetValue(std::move(result));
    } catch (...) {
      next_prom->SetException(std::current_exception());
    }
  };

  if (executor) {
    executor->submit(std::move(task));
    return;
  }

  if (policy == Lauch::Async) {
    Async(std::move(task));
  } else if(policy == Lauch::Callback){
    auto future = Async(std::move(task));

    auto mv_future = MakeMoveWrapper(std::move(future));
    Async([mv_future, this]() mutable {
      auto&& f = mv_future.move();
      f.WaitFor(std::chrono::minutes(60));//60 minutes is long enough
      f.Get();
    });
  }else{
    task();
  }
}
}



#endif //FUTURE_DEMO_FUTURE_H
