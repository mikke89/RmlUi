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

#ifndef TROMPELOEIL_COMPARE_HPP
#define TROMPELOEIL_COMPARE_HPP

#ifndef TROMPELOEIL_MATCHER_HPP
#include "../matcher.hpp"
#endif

namespace trompeloeil
{

  namespace lambdas {
    // The below must be classes/structs to work with VS 2015 update 3
    // since it doesn't respect the trailing return type declaration on
    // the lambdas of template deduction context

#define TROMPELOEIL_MK_PRED_BINOP(name, op)                             \
    struct name {                                                       \
      template <typename X, typename Y>                                 \
      auto operator()(X const& x, Y const& y) const -> decltype(x op y) \
      {                                                                 \
        ::trompeloeil::ignore(x,y);                                     \
        return x op y;                                                  \
      }                                                                 \
    }
    TROMPELOEIL_MK_PRED_BINOP(equal, ==);
    TROMPELOEIL_MK_PRED_BINOP(not_equal, !=);
    TROMPELOEIL_MK_PRED_BINOP(less, <);
    TROMPELOEIL_MK_PRED_BINOP(less_equal, <=);
    TROMPELOEIL_MK_PRED_BINOP(greater, >);
    TROMPELOEIL_MK_PRED_BINOP(greater_equal, >=);
#undef TROMPELOEIL_MK_PRED_BINOP

#define TROMPELOEIL_MK_OP_PRINTER(name, op_string)                      \
    struct name ## _printer                                             \
    {                                                                   \
      template <typename T>                                             \
      void                                                              \
      operator()(                                                       \
        std::ostream& os,                                               \
        T const& value)                                                 \
      const                                                             \
      {                                                                 \
        os << op_string;                                                \
        ::trompeloeil::print(os, value);                                \
      }                                                                 \
    }
    TROMPELOEIL_MK_OP_PRINTER(equal, " == ");
    TROMPELOEIL_MK_OP_PRINTER(not_equal, " != ");
    TROMPELOEIL_MK_OP_PRINTER(less, " < ");
    TROMPELOEIL_MK_OP_PRINTER(less_equal, " <= ");
    TROMPELOEIL_MK_OP_PRINTER(greater, " > ");
    TROMPELOEIL_MK_OP_PRINTER(greater_equal, " >= ");
#undef TROMPELOEIL_MK_OP_PRINTER


  }
template <
  typename T = wildcard,
  typename V,
  typename R = make_matcher_return<T, lambdas::equal, lambdas::equal_printer, V>>
inline
auto
eq(
  V&& v)
TROMPELOEIL_TRAILING_RETURN_TYPE(R)
{
  return make_matcher<T>(lambdas::equal(),
                         lambdas::equal_printer(),
                         std::forward<V>(v));
}

template <
  typename T = wildcard,
  typename V,
  typename R = make_matcher_return<T, lambdas::not_equal, lambdas::not_equal_printer, V>>
inline
auto
ne(
  V&& v)
TROMPELOEIL_TRAILING_RETURN_TYPE(R)
{
  return make_matcher<T>(lambdas::not_equal(),
                         lambdas::not_equal_printer(),
                         std::forward<V>(v));
}

template <
  typename T = wildcard,
  typename V,
  typename R = make_matcher_return<T, lambdas::greater_equal, lambdas::greater_equal_printer, V>>
inline
auto
ge(
  V&& v)
TROMPELOEIL_TRAILING_RETURN_TYPE(R)
{
  return make_matcher<T>(lambdas::greater_equal(),
                         lambdas::greater_equal_printer(),
                         std::forward<V>(v));
}

template <
  typename T = wildcard,
  typename V,
  typename R = make_matcher_return<T, lambdas::greater, lambdas::greater_printer, V>>
inline
auto
gt(
  V&& v)
TROMPELOEIL_TRAILING_RETURN_TYPE(R)
{
  return make_matcher<T>(lambdas::greater(),
                         lambdas::greater_printer(),
                         std::forward<V>(v));
}

template <
  typename T = wildcard,
  typename V,
  typename R = make_matcher_return<T, lambdas::less, lambdas::less_printer, V>>
inline
auto
lt(
  V&& v)
TROMPELOEIL_TRAILING_RETURN_TYPE(R)
{
  return make_matcher<T>(lambdas::less(),
                         lambdas::less_printer(),
                         std::forward<V>(v));
}

template <
  typename T = wildcard,
  typename V,
  typename R = make_matcher_return<T, lambdas::less_equal, lambdas::less_equal_printer,  V>>
inline
auto
le(
  V&& v)
TROMPELOEIL_TRAILING_RETURN_TYPE(R)
{
  return make_matcher<T>(lambdas::less_equal(),
                         lambdas::less_equal_printer(),
                         std::forward<V>(v));
}

}

#endif //TROMPELOEIL_COMPARE_HPP
