//
// Created by qicosmos on 2020/7/23.
//

#ifndef FUTURE_DEMO_TRAITS_H
#define FUTURE_DEMO_TRAITS_H
#if __cplusplus <= 201402L
#include "absl/meta/type_traits.h"
#endif
namespace ray {
#if __cplusplus <= 201402L
template <typename T, typename... Args> struct holder {};

template <typename Holder, typename = void>
struct is_invocable_impl : std::false_type {};

template <template <class T, class... Args> class Holder, typename T,
          typename... Args>
struct is_invocable_impl<
    Holder<T, Args...>,
    absl::void_t<decltype(std::declval<T>()(std::declval<Args>()...))>>
    : std::true_type {};

template <typename T, typename... Args>
using is_invocable = is_invocable_impl<holder<T, Args...>>;
template <typename T, typename... Args>
using is_invocable_t = typename is_invocable<T, Args...>::type;
#if __cplusplus > 201103L
template <typename T, typename... Args>
constexpr bool is_invocable_v = is_invocable<T, Args...>::value;
#endif

template <typename T> class Future;

template <typename T> struct IsFuture : std::false_type { using Inner = T; };

template <typename T> struct IsFuture<Future<T>> : std::true_type {
  using Inner = T;
};

template <typename T> class Try;

template <typename T> struct IsTry : std::false_type { using Inner = T; };

template <typename T> struct IsTry<Try<T>> : std::true_type {
  using Inner = T;
};

template <typename T> struct IsTry<Try<T>&&> : std::true_type {
  using Inner = T;
};

template <typename T> struct IsTry<Try<T>&> : std::true_type {
  using Inner = T;
};

template <typename T> struct IsTry<const Try<T>&> : std::true_type {
  using Inner = T;
};
#endif

template <typename... Args, typename F, std::size_t... Idx>
inline void for_each_tp(std::tuple<Args...>& t, F&& f, absl::index_sequence<Idx...>)
{
  (void)std::initializer_list<int>{(std::forward<F>(f)(std::get<Idx>(t), std::integral_constant<size_t, Idx>{}), 0)...};
}

template <typename... Args, typename F, std::size_t... Idx>
inline void for_each_tp(const std::tuple<Args...>& t, F&& f, absl::index_sequence<Idx...>)
{
  (void)std::initializer_list<int>{(std::forward<F>(f)(std::get<Idx>(t), std::integral_constant<size_t, Idx>{}), 0)...};
}

template <typename T> struct function_traits;

template <typename Ret, typename... Args> struct function_traits<Ret(Args...)> {
public:
  enum { arity = sizeof...(Args) };
  typedef Ret function_type(Args...);
  typedef Ret return_type;
  using stl_function_type = std::function<function_type>;
  typedef Ret (*pointer)(Args...);

  typedef std::tuple<Args...> tuple_type;
  typedef std::tuple<absl::remove_const_t<absl::remove_reference_t<Args>>...>
      bare_tuple_type;
  using first_arg_t = typename std::tuple_element<0, tuple_type>::type;
};

template <typename Ret> struct function_traits<Ret()> {
public:
  enum { arity = 0 };
  typedef Ret function_type();
  typedef Ret return_type;
  using stl_function_type = std::function<function_type>;
  typedef Ret (*pointer)();

  typedef std::tuple<> tuple_type;
  typedef std::tuple<> bare_tuple_type;
  using args_tuple = std::tuple<std::string>;
  using args_tuple_2nd = std::tuple<std::string>;
  using first_arg_t = void;
};

template <typename Ret, typename... Args>
struct function_traits<Ret (*)(Args...)> : function_traits<Ret(Args...)> {};

template <typename Ret, typename... Args>
struct function_traits<std::function<Ret(Args...)>>
    : function_traits<Ret(Args...)> {};

template <typename ReturnType, typename ClassType, typename... Args>
struct function_traits<ReturnType (ClassType::*)(Args...)>
    : function_traits<ReturnType(Args...)> {};

template <typename ReturnType, typename ClassType, typename... Args>
struct function_traits<ReturnType (ClassType::*)(Args...) const>
    : function_traits<ReturnType(Args...)> {};

template <typename Callable>
struct function_traits : function_traits<decltype(&Callable::operator())> {};
}
#endif //FUTURE_DEMO_TRAITS_H
