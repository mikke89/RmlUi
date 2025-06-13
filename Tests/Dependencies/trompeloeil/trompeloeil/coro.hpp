/*
 * Trompeloeil C++ mocking framework
 *
 * Copyright (C) Björn Fahller
 *
 *  Use, modification and distribution is subject to the
 *  Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy atl
 *  http://www.boost.org/LICENSE_1_0.txt)
 *
 * Project home: https://github.com/rollbear/trompeloeil
 */

#ifndef TROMPELOEIL_CORO_HPP
#define TROMPELOEIL_CORO_HPP

#if defined(__cpp_impl_coroutine)
#  define TROMPELOEIL_COROUTINES_SUPPORTED 1
#else
#  error "Coroutines are not supported by this compiler"
#endif

#ifdef TROMPELOEIL_COROUTINES_SUPPORTED

#ifndef TROMPELOEIL_MOCK_HPP_
#include "mock.hpp"
#endif

namespace trompeloeil
{
  template <typename U>
  struct type_wrapper{
  template <typename V>
    static V remove_rvalue_ref(V&&);
    using type = decltype(remove_rvalue_ref(std::declval<U>()));
  };
  template <>
  struct type_wrapper<void>
  {
    using type = void;
  };

  template <typename T>
  struct coro_value_type
  {
    static auto func()
    {
      if constexpr (requires {std::declval<T>().operator co_await();})
      {
        return type_wrapper<decltype(std::declval<T>().operator co_await().await_resume())>{};
      }
      else
      {
        return type_wrapper<decltype(std::declval<T>().await_resume())>{};
      }
    }
    using type = typename decltype(func())::type;
  };

  template <typename T>
  using coro_value_type_t = typename coro_value_type<T>::type;

  template <typename Sig>

  struct yield_expr_base<Sig, true> : public list_elem<yield_expr_base<Sig, true>>
  {
    ~yield_expr_base() override = default;

    using ret_type = coro_value_type_t<return_of_t<Sig>>;
    virtual
    ret_type
    expr(
      call_params_type_t<Sig>&)
    const = 0;
  };

  template <typename Sig>
  struct yield_expr_base<Sig, false> : public list_elem<yield_expr_base<Sig, false>>
  {
    using ret_type = return_of_t<Sig>;
    virtual
    ret_type
    expr(
      call_params_type_t<Sig>&)
    const = 0;
  };

  template <typename Sig, typename Expr>
  struct yield_expr : yield_expr_base<Sig>
  {
    using ret_type = typename yield_expr_base<Sig>::ret_type;
    template <typename E>
    explicit
    yield_expr(
      E&& e_)
      : e(std::forward<E>(e_))
    {}

    ret_type
    expr(
      call_params_type_t<Sig>& t)
    const
    override
    {
      return e(t);
    }
  private:
    Expr e;
  };

  template <typename Sig, typename T>
  class co_return_handler_t : public return_handler<Sig>
  {
  public:
    template <typename U>
    explicit
    co_return_handler_t(
      U&& u,
      std::shared_ptr<yield_expr_list<Sig>> yields_)
      : func(std::forward<U>(u))
      , yields(std::move(yields_))
    {}

    return_of_t<Sig>
    call(
      trace_agent& /*agent*/,
      call_params_type_t<Sig>& params)
    override
    {
      using coro_type = return_of_t<Sig>;
      using promise_type = typename std::coroutine_traits<coro_type>::promise_type;
      using value_type = coro_value_type_t<coro_type>;
      if constexpr (requires {std::declval<promise_type&>().yield_value(std::declval<value_type>());})
      {
        for (auto & e : *yields)
        {
          co_yield e.expr(params);
        }
      }
      co_return func(params);
    }
  private:
    T func;
    std::shared_ptr<yield_expr_list<Sig>> yields;
  };

  template <typename H, typename signature>
  struct co_throw_handler_t
  {
    using R = decltype(default_return<return_of_t<signature>>());
    using promise_value_type = trompeloeil::coro_value_type_t<R>;
    explicit
    co_throw_handler_t(H&& h_)
      : h(std::move(h_))
    {}

    template <typename T>
    promise_value_type operator()(T& p)
    {
      return ((void)h(p), trompeloeil::default_return<promise_value_type>());
    }
  private:
    H h;
  };

  struct handle_co_yield
  {
    template <typename Matcher, typename modifier_tag, typename Parent, typename E>
    static
    call_modifier<Matcher, modifier_tag, Parent>
    action(
      call_modifier<Matcher, modifier_tag, Parent>&& m,
      E&& e)
    {
      using signature = typename Parent::signature;
      using params_type = call_params_type_t<signature>&;
      using sigret = return_of_t<signature>;
      using ret = std::invoke_result_t<E, params_type>;

      constexpr bool is_void = std::is_same_v<ret, void>;
      constexpr bool is_coroutine = trompeloeil::is_coroutine<sigret>::value;
      constexpr bool is_matching_type = std::invoke([]{
        if constexpr (is_coroutine && !is_void) {
          using promise = typename std::coroutine_traits<sigret>::promise_type;
          return requires(promise p, ret r){ p.yield_value(r); };
        } else {
          return false;
        }
      });

      static_assert(is_coroutine,
                    "CO_YIELD when return type is not a coroutine");
      static_assert(!is_coroutine || !is_void,
                    "You cannot CO_YIELD void");
      static_assert(!is_coroutine || is_void || is_matching_type,
                    "CO_YIELD is incompatible with the promise type");
      constexpr auto valid = is_coroutine && !is_void && is_matching_type;
      if constexpr (valid)
      {
	if (!m.matcher->yield_expressions)
	{
	  m.matcher->yield_expressions = std::make_shared<yield_expr_list<signature>>();
	}
        auto expr = new yield_expr<signature, E>(std::forward<E>(e));
        m.matcher->yield_expressions->push_back(expr);
      }
      return {std::move(m).matcher};
    }
  };

  struct handle_co_return
  {
    template <typename Matcher, typename modifier_tag, typename Parent, typename H>
    static
    call_modifier<Matcher, modifier_tag, co_return_injector<return_of_t<typename Parent::signature>, Parent>>
    action(
      call_modifier<Matcher, modifier_tag, Parent>&& m,
      H&& h)
    {
      using signature = typename Parent::signature;
      using return_type = typename Parent::return_type;
      using co_return_type = typename Parent::co_return_type;
      using params_type = call_params_type_t<signature>&;
      using sigret = return_of_t<signature>;
      using ret = std::invoke_result_t<H, params_type>;
      constexpr bool has_return = !std::is_same_v<return_type, void>;
      constexpr bool has_co_return = !std::is_same_v<co_return_type, void>;
      constexpr bool is_coroutine = trompeloeil::is_coroutine<sigret>::value;
      constexpr bool is_matching_type = std::invoke([]{if constexpr (is_coroutine) {
        using promise = typename std::coroutine_traits<sigret>::promise_type;
        if constexpr (std::is_same_v<void, ret>) {
          return requires (promise p){p.return_void();};
        } else {
          return requires(promise p) { p.return_value(std::declval<ret>()); };
        }}
        return true;});

      static_assert(!has_return,
                    "CO_RETURN and RETURN cannot be combined");
      static_assert(has_return || !has_co_return,
                    "Multiple CO_RETURN does not make sense");
      static_assert(has_return || is_coroutine, "CO_RETURN when return type is not a coroutine");
      static_assert(has_return || has_co_return || !is_coroutine || is_matching_type,
                    "Expression type does not match the coroutine promise type");
      static_assert(!Parent::throws || Parent::upper_call_limit == 0,
                    "CO_THROW and CO_RETURN does not make sense");
      static_assert(Parent::upper_call_limit > 0,
                    "CO_RETURN for forbidden call does not make sense");

      constexpr bool valid = !has_return && !has_co_return && is_coroutine && is_matching_type && !Parent::throws && Parent::upper_call_limit > 0;
      if constexpr (valid)
      {
        using basic_t = typename std::remove_reference<H>::type;
        using handler = co_return_handler_t<signature, basic_t>;
	if (!m.matcher->yield_expressions)
	{
	  m.matcher->yield_expressions = std::make_shared<yield_expr_list<signature>>();
	}
        m.matcher->return_handler_obj.reset(
          new handler(std::forward<H>(h),
          m.matcher->yield_expressions)
        );
      }
      return {std::move(m).matcher};
    }
  };

  struct handle_co_throw
  {
    template <typename Matcher, typename modifier_tag, typename Parent, typename H>
    static
    call_modifier<Matcher, modifier_tag, throw_injector<Parent>>
    action(
      call_modifier<Matcher, modifier_tag, Parent>&& m,
      H&& h)
    {
      using signature = typename Parent::signature;
      using co_return_type = typename Parent::co_return_type;
      using sigret = return_of_t<signature>;
      constexpr bool is_coroutine      = trompeloeil::is_coroutine<sigret>::value;
      static_assert(is_coroutine,
                    "Do not use CO_THROW from a normal function, use THROW");
      static_assert(!is_coroutine || !Parent::throws,
                    "Multiple CO_THROW does not make sense");
      constexpr bool has_return = !std::is_same<co_return_type, void>::value;
      static_assert(!is_coroutine || !has_return,
                    "CO_THROW and CO_RETURN does not make sense");

      constexpr bool forbidden = Parent::upper_call_limit == 0U;

      static_assert(!is_coroutine || !forbidden,
                    "CO_THROW for forbidden call does not make sense");

      constexpr bool valid = is_coroutine && !Parent::throws && !has_return;
      if constexpr (valid) {
        if (!m.matcher->yield_expressions)
        {
          m.matcher->yield_expressions = std::make_shared<yield_expr_list<signature>>();
        }
        using throw_handler_t = co_throw_handler_t<H, signature>;
        auto handler = throw_handler_t(std::forward<H>(h));
        using ret_handler = co_return_handler_t<signature, throw_handler_t>;
        m.matcher->return_handler_obj.reset(new ret_handler(std::move(handler),
                                                            m.matcher->yield_expressions));
      }
      return {std::move(m).matcher};
    }
  };

}

#define TROMPELOEIL_CO_RETURN(...)    TROMPELOEIL_CO_RETURN_(=, __VA_ARGS__)
#define TROMPELOEIL_LR_CO_RETURN(...) TROMPELOEIL_CO_RETURN_(&, __VA_ARGS__)

#define TROMPELOEIL_CO_RETURN_(capture, ...)                                   \
  template action<trompeloeil::handle_co_return>([capture](auto& trompeloeil_x)\
                                                 -> decltype(auto) {           \
    auto&& _1 = ::trompeloeil::mkarg<1>(trompeloeil_x);                        \
    auto&& _2 = ::trompeloeil::mkarg<2>(trompeloeil_x);                        \
    auto&& _3 = ::trompeloeil::mkarg<3>(trompeloeil_x);                        \
    auto&& _4 = ::trompeloeil::mkarg<4>(trompeloeil_x);                        \
    auto&& _5 = ::trompeloeil::mkarg<5>(trompeloeil_x);                        \
    auto&& _6 = ::trompeloeil::mkarg<6>(trompeloeil_x);                        \
    auto&& _7 = ::trompeloeil::mkarg<7>(trompeloeil_x);                        \
    auto&& _8 = ::trompeloeil::mkarg<8>(trompeloeil_x);                        \
    auto&& _9 = ::trompeloeil::mkarg<9>(trompeloeil_x);                        \
    auto&&_10 = ::trompeloeil::mkarg<10>(trompeloeil_x);                       \
    auto&&_11 = ::trompeloeil::mkarg<11>(trompeloeil_x);                       \
    auto&&_12 = ::trompeloeil::mkarg<12>(trompeloeil_x);                       \
    auto&&_13 = ::trompeloeil::mkarg<13>(trompeloeil_x);                       \
    auto&&_14 = ::trompeloeil::mkarg<14>(trompeloeil_x);                       \
    auto&&_15 = ::trompeloeil::mkarg<15>(trompeloeil_x);                       \
    ::trompeloeil::ignore(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15); \
    return ::trompeloeil::decay_return_type(__VA_ARGS__);                      \
  })

#define TROMPELOEIL_CO_THROW(...)    TROMPELOEIL_CO_THROW_(=, __VA_ARGS__)
#define TROMPELOEIL_LR_CO_THROW(...) TROMPELOEIL_CO_THROW_(&, __VA_ARGS__)

#define TROMPELOEIL_CO_THROW_(capture, ...)                                    \
  template action<trompeloeil::handle_co_throw>([capture](auto& trompeloeil_x){\
    auto&& _1 = ::trompeloeil::mkarg<1>(trompeloeil_x);                        \
    auto&& _2 = ::trompeloeil::mkarg<2>(trompeloeil_x);                        \
    auto&& _3 = ::trompeloeil::mkarg<3>(trompeloeil_x);                        \
    auto&& _4 = ::trompeloeil::mkarg<4>(trompeloeil_x);                        \
    auto&& _5 = ::trompeloeil::mkarg<5>(trompeloeil_x);                        \
    auto&& _6 = ::trompeloeil::mkarg<6>(trompeloeil_x);                        \
    auto&& _7 = ::trompeloeil::mkarg<7>(trompeloeil_x);                        \
    auto&& _8 = ::trompeloeil::mkarg<8>(trompeloeil_x);                        \
    auto&& _9 = ::trompeloeil::mkarg<9>(trompeloeil_x);                        \
    auto&&_10 = ::trompeloeil::mkarg<10>(trompeloeil_x);                       \
    auto&&_11 = ::trompeloeil::mkarg<11>(trompeloeil_x);                       \
    auto&&_12 = ::trompeloeil::mkarg<12>(trompeloeil_x);                       \
    auto&&_13 = ::trompeloeil::mkarg<13>(trompeloeil_x);                       \
    auto&&_14 = ::trompeloeil::mkarg<14>(trompeloeil_x);                       \
    auto&&_15 = ::trompeloeil::mkarg<15>(trompeloeil_x);                       \
    ::trompeloeil::ignore(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15); \
    throw __VA_ARGS__;                                                         \
 })

#define TROMPELOEIL_CO_YIELD(...)    TROMPELOEIL_CO_YIELD_(=, __VA_ARGS__)
#define TROMPELOEIL_LR_CO_YIELD(...) TROMPELOEIL_CO_YIELD_(&, __VA_ARGS__)


#define TROMPELOEIL_CO_YIELD_(capture, ...)                                    \
  template action<trompeloeil::handle_co_yield>([capture](auto& trompeloeil_x){\
    auto&& _1 = ::trompeloeil::mkarg<1>(trompeloeil_x);                        \
    auto&& _2 = ::trompeloeil::mkarg<2>(trompeloeil_x);                        \
    auto&& _3 = ::trompeloeil::mkarg<3>(trompeloeil_x);                        \
    auto&& _4 = ::trompeloeil::mkarg<4>(trompeloeil_x);                        \
    auto&& _5 = ::trompeloeil::mkarg<5>(trompeloeil_x);                        \
    auto&& _6 = ::trompeloeil::mkarg<6>(trompeloeil_x);                        \
    auto&& _7 = ::trompeloeil::mkarg<7>(trompeloeil_x);                        \
    auto&& _8 = ::trompeloeil::mkarg<8>(trompeloeil_x);                        \
    auto&& _9 = ::trompeloeil::mkarg<9>(trompeloeil_x);                        \
    auto&&_10 = ::trompeloeil::mkarg<10>(trompeloeil_x);                       \
    auto&&_11 = ::trompeloeil::mkarg<11>(trompeloeil_x);                       \
    auto&&_12 = ::trompeloeil::mkarg<12>(trompeloeil_x);                       \
    auto&&_13 = ::trompeloeil::mkarg<13>(trompeloeil_x);                       \
    auto&&_14 = ::trompeloeil::mkarg<14>(trompeloeil_x);                       \
    auto&&_15 = ::trompeloeil::mkarg<15>(trompeloeil_x);                       \
    ::trompeloeil::ignore(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15); \
    return __VA_ARGS__;                                                         \
 })

#ifndef TROMPELOEIL_LONG_MACROS

#define CO_RETURN                 TROMPELOEIL_CO_RETURN
#define LR_CO_RETURN              TROMPELOEIL_LR_CO_RETURN
#define CO_THROW                  TROMPELOEIL_CO_THROW
#define LR_CO_THROW               TROMPELOEIL_LR_CO_THROW
#define CO_YIELD                  TROMPELOEIL_CO_YIELD
#define LR_CO_YIELD               TROMPELOEIL_LR_CO_YIELD

#endif

#endif

#endif //TROMPELOEIL_CORO_HPP
