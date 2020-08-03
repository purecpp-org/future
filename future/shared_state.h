//
// Created by qicosmos on 2020/7/27.
//

#ifndef FUTURE_DEMO_SHARED_STATE_H
#define FUTURE_DEMO_SHARED_STATE_H
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>

namespace ray{
enum class FutureStatus { None, Timeout, Done, Retrived };

template <typename T> struct SharedState {
  static_assert(std::is_same<T, void>::value ||
                std::is_copy_constructible<T>() ||
                std::is_move_constructible<T>(),
                "must be copyable or movable or void");
  SharedState() : state_(FutureStatus::None), has_retrieved_(false) {}
  using ValueType = typename TryWrapper<T>::type;

  void Wait() {
    std::unique_lock<std::mutex> lock(then_mtx_);
    cond_var_.wait(lock, [this]() { return state_ != FutureStatus::None; });
  }

  template <typename Rep, typename Period>
  FutureStatus
  WaitFor(const std::chrono::duration<Rep, Period> &timeout_duration)  {
    std::unique_lock<std::mutex> lock(then_mtx_);
    bool r = cond_var_.wait_for(lock, timeout_duration,
                              [this]() { return state_ != FutureStatus::None; });
    if(!r){
      state_ = FutureStatus::Timeout;
    }
    return state_;
  }

  template <typename Clock, typename Duration>
  FutureStatus WaitUntil(
      const std::chrono::time_point<Clock, Duration> &timeout_time)  {
    std::unique_lock<std::mutex> lock(then_mtx_);
    bool r = cond_var_.wait_until(lock, timeout_time,
                                [this]() { return state_ != FutureStatus::None; });
    if(!r){
      state_ = FutureStatus::Timeout;
    }
    return state_;
  }

  std::mutex then_mtx_;
  std::condition_variable cond_var_;
  Try<T> value_;
  std::vector<std::function<void()>> continuations_;
  FutureStatus state_;
  std::atomic<bool> has_retrieved_;
};
}
#endif // FUTURE_DEMO_SHARED_STATE_H
