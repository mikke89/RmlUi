/*
 * Trompeloeil C++ mocking framework
 *
 * Copyright (C) Björn Fahller
 * Copyright (C) Andrew Paxie
 *
 *  Use, modification and distribution is subject to the
 *  Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy atl
 *  http://www.boost.org/LICENSE_1_0.txt)
 *
 * Project home: https://github.com/rollbear/trompeloeil
 */

#ifndef TROMPELOEIL_MATCHER_HPP
#define TROMPELOEIL_MATCHER_HPP

#ifndef TROMPELOEIL_MOCK_HPP_
#include "mock.hpp"
#endif

namespace trompeloeil
{
  template <typename T>
  struct typed_matcher : matcher
  {
    template <bool b = false>
    operator T() const {
        static_assert(b,
                      "Getting a value from a typed matcher is not allowed.\n"
                      "See https://github.com/rollbear/trompeloeil/issues/270\n"
                      "and https://github.com/rollbear/trompeloeil/issues/290");
        return {};
    }
  };

  template <>
  struct typed_matcher<std::nullptr_t> : matcher
  {
    template <
      typename T,
      typename = decltype(std::declval<T>() == nullptr)
    >
    operator T&&() const
    {
      static_assert(std::is_same<T, void>{},
                    "Getting a value from a typed matcher is not allowed.\n"
                    "See https://github.com/rollbear/trompeloeil/issues/270\n"
                    "https://github.com/rollbear/trompeloeil/issues/290");
      return *this;
    }

    template <
      typename T,
      typename = decltype(std::declval<T>() == nullptr)
    >
    operator T&()const volatile
    {
      static_assert(std::is_same<T, void>{},
                    "Getting a value from a typed matcher is not allowed.\n"
                    "See https://github.com/rollbear/trompeloeil/issues/270\n"
                    "and https://github.com/rollbear/trompeloeil/issues/290");
      return *this;
    }

    template <typename T, typename C>
    operator T C::*() const
    {
      static_assert(std::is_same<C, void>{},
                    "Getting a value from a typed matcher is not allowed.\n"
                    "See https://github.com/rollbear/trompeloeil/issues/270\n"
                    "and https://github.com/rollbear/trompeloeil/issues/290");
      return *this;
    }
  };

  template <typename Pred, typename ... T>
  class duck_typed_matcher : public matcher
  {
  public:
#if (!TROMPELOEIL_GCC) || \
    (TROMPELOEIL_GCC && TROMPELOEIL_GCC_VERSION >= 40900)

    // g++ 4.8 gives a "conversion from <T> to <U> is ambiguous" error
    // if this operator is defined.
    template <
      typename V,
      typename = detail::enable_if_t<!is_matcher<V>{}>,
      typename = invoke_result_type<Pred, V&&, T...>
    >
    operator V&&() const {
#if !TROMPELOEIL_CLANG || TROMPELOEIL_CLANG_VERSION >= 40000
      // clang 3.x instantiates the function even when it's only used
      // for compile time signature checks and never actually called.
      static_assert(std::is_same<V, void>{},
                    "Getting a value from a duck typed matcher is not allowed.\n"
                    "See https://github.com/rollbear/trompeloeil/issues/270\n"
                    "and https://github.com/rollbear/trompeloeil/issues/290");
#endif
      return *this;
    }

#endif

    template <
      typename V,
      typename = detail::enable_if_t<!is_matcher<V>{}>,
      typename = invoke_result_type<Pred, V&, T...>
    >
    operator V&() const volatile
    {
      static_assert(std::is_same<V, void>{},
                    "Getting a value from a duck typed matcher is not allowed.\n"
                    "See https://github.com/rollbear/trompeloeil/issues/270\n"
                    "and https://github.com/rollbear/trompeloeil/issues/290");

      return *this;
    }
  };

  template <typename MatchType, typename Predicate, typename ... ActualType>
  struct matcher_kind
  {
    using type = typed_matcher<MatchType>;
  };

  template <typename Predicate, typename ... ActualType>
  struct matcher_kind<wildcard, Predicate, ActualType...>
  {
    using type = duck_typed_matcher<Predicate, ActualType...>;
  };

  template <typename MatchType, typename Predicate, typename ... ActualType>
  using matcher_kind_t =
    typename matcher_kind<MatchType, Predicate, ActualType...>::type;

  template <typename Predicate, typename Printer, typename MatcherType, typename ... T>
  class predicate_matcher
    : private Predicate
      , private Printer
      , public MatcherType
  {
  public:
    template <typename ... U>
    constexpr
    predicate_matcher(
      Predicate&& pred,
      Printer&& printer,
      U&& ... v)
    noexcept(noexcept(std::tuple<T...>(std::declval<U>()...)) && noexcept(Predicate(std::declval<Predicate&&>())) && noexcept(Printer(std::declval<Printer&&>())))
      : Predicate(std::move(pred))
        , Printer(std::move(printer))
        , value(std::forward<U>(v)...)
    {}

    template <typename V>
    constexpr
    bool
    matches(
      V&& v)
    const
    noexcept(noexcept(std::declval<Predicate const&>()(std::declval<V&&>(), std::declval<const T&>()...)))
    {
      return matches_(std::forward<V>(v), detail::make_index_sequence<sizeof...(T)>{});
    }

    friend
    std::ostream&
    operator<<(
      std::ostream& os,
      predicate_matcher const& v)
    {
      return v.print_(os, detail::make_index_sequence<sizeof...(T)>{});
    }
  private:
    // The below function call operator must be declared to
    // work around gcc bug 78446
    //
    // For some reason microsoft compiler from VS2015 update 3
    // requires the function call operator to be private to avoid
    // ambiguities.
    template <typename ... U>
    void operator()(U&&...) const = delete;

    template <typename V, size_t ... I>
    bool matches_(V&& v, detail::index_sequence<I...>) const
    {
      return Predicate::operator()(std::forward<V>(v), std::get<I>(value)...);
    }

    template <size_t ... I>
    std::ostream& print_(std::ostream& os_, detail::index_sequence<I...>) const
    {
      Printer::operator()(os_, std::get<I>(value)...);
      return os_;
    }

    std::tuple<T...> value;
  };

  template <typename MatchType, typename Predicate, typename Printer, typename ... T>
  using make_matcher_return =
    predicate_matcher<Predicate,
      Printer,
      matcher_kind_t<MatchType, Predicate, detail::decay_t<T>...>,
      detail::decay_t<T>...>;


  template <typename MatchType, typename Predicate, typename Printer, typename ... T>
  inline
  make_matcher_return<MatchType, Predicate, Printer, T...>
  make_matcher(Predicate pred, Printer print, T&& ... t)
  {
    return {std::move(pred), std::move(print), std::forward<T>(t)...};
  }



}

#endif //TROMPELOEIL_MATCHER_HPP
