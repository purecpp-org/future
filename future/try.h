//
// Created by qicosmos on 2020/7/21.
//

#ifndef FUTURE_DEMO_TRY_H
#define FUTURE_DEMO_TRY_H

#include <exception>
#include <stdexcept>
#include <cassert>
#include <type_traits>
#if __cplusplus <= 201402L
#include "absl/types/variant.h"
#include "absl/meta/type_traits.h"
#else
#include <variant>
#endif
#include "traits.h"

namespace ray {

#if __cplusplus <= 201402L
template <typename... Args> using ray_variant = absl::variant<Args...>;
#else
// use std::variant in c++17
template <typename... Args> using ray_variant = std::variant<Args...>;
#endif
namespace {
struct Blank {};
}

template <typename T> class Try {
public:
  Try() = default;

  template <typename U> explicit Try(U &&val) : val_(std::forward<U>(val)) {}

  template <typename U> Try<T> &operator=(U &&val) {
    val_ = std::forward<U>(val);
    return *this;
  }

  explicit Try(std::exception_ptr &&e) : val_(std::move(e)) {}

  const T &Value() const & {
    Check();

#if __cplusplus < 201703L
    return absl::get<T>(val_);
#else
    return std::get<T>(val_);
#endif
  }

  operator const T &() const & {
    return Value();
  }
  operator T &() & {
    return Value();
  }
  operator T &&() && {
    return std::move(Value());
  }

  T &Value() & {
    Check();
    return absl::get<T>(val_);
  }

  T &&Value() && {
    Check();
    return std::move(absl::get<T>(val_));
  }

  std::exception_ptr &Exception() {
    if (!HasException()) {
      throw std::logic_error("not exception");
    }

#if __cplusplus < 201703L
    return absl::get<2>(val_);
#else
    return std::get<2>(val_);
#endif
  }

  bool HasValue() const { return val_.index() == 1; }

  bool HasException() const { return val_.index() == 2; }

  bool NotInit() const { return val_.index() == 0; }

  template <typename R> R Get() { return std::forward<R>(Value()); }

private:
  void Check() {
    if (HasException()) {
      std::rethrow_exception(absl::get<2>(val_));
    } else if (NotInit()) {
      throw std::logic_error("not init");
    }
  }

  ray_variant<Blank, T, std::exception_ptr> val_;
};

template <> class Try<void> {
public:
  Try() { val_ = true; }

  explicit Try(std::exception_ptr &&e) : val_(std::move(e)) {}

  bool HasValue() const { return val_.index() == 0; }

  bool HasException() const { return val_.index() == 1; }

  template <typename R> R Get() { return std::forward<R>(*this); }

private:
  ray_variant<bool, std::exception_ptr> val_;
};

template <typename T> struct TryWrapper { using type = Try<T>; };

template <typename T> struct TryWrapper<Try<T>> { using type = Try<T>; };

template <typename T> using try_type_t = typename TryWrapper<T>::type;
}
#endif //FUTURE_DEMO_TRY_H
