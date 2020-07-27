//
// Created by qicosmos on 2020/7/27.
//

#ifndef FUTURE_DEMO_PROMISE_H
#define FUTURE_DEMO_PROMISE_H

#include "shared_state.h"

namespace ray{
template <typename T> class Future;

template <typename T> class Promise {
public:
  Promise() : shared_state_(std::make_shared<SharedState<T>>()) {}

  template <typename... Args> void SetValue(Args &&... val) {
    static_assert(sizeof...(Args) <= 1, "at most one argument");
    {
      std::unique_lock<std::mutex> lock(shared_state_->then_mtx_);
      if (shared_state_->state_ != FutureStatus::None) {
        return;
      }

      shared_state_->state_ = FutureStatus::Done;
      SetValueInternal(std::forward<Args...>(val)...);
      shared_state_->cond_var_.notify_all();
    }

    if (shared_state_->then_) {
      shared_state_->then_(std::move(shared_state_->value_));
    }
  }

  void SetException(std::exception_ptr &&exp) { SetValue(std::move(exp)); }

  bool IsReady() { return shared_state_->state_ != FutureStatus::None; }

  Future<T> GetFuture() {
    shared_state_->has_retrieved_ = true;
    return Future<T>(shared_state_);
  }

private:
  template <typename V> void SetValueInternal(V &&val) {
    shared_state_->value_ = absl::forward<V>(val);
  }

  void SetValueInternal() { shared_state_->value_ = Try<void>(); }

  void SetValueInternal(std::exception_ptr &&e) {
    shared_state_->value_ =
        typename SharedState<T>::ValueType(std::move(e));
  }

private:
  std::shared_ptr<SharedState<T>> shared_state_;
};
}
#endif // FUTURE_DEMO_PROMISE_H
