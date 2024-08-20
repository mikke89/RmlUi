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

#ifndef TROMPELOEIL_ANY_HPP
#define TROMPELOEIL_ANY_HPP

#ifndef TROMPELOEIL_MATCHER_HPP
#include "../matcher.hpp"
#endif


namespace trompeloeil
{
  namespace lambdas {

    struct any_predicate
    {
      template <typename T>
      bool
      operator()(
        T&&)
      const
      {
        return true;
      }
    };


    // Define `struct` with `operator()` to replace generic lambdas.
    struct any_printer
    {
      explicit
      any_printer(
        char const* type_name_)
        : type_name(type_name_)
      {}

      void
      operator()(
        std::ostream& os)
      const
      {
        os << " matching ANY(" << type_name << ")";
      }

    private:
      char const* type_name;
    };

  }

  template <
    typename T,
    typename R = make_matcher_return<T, lambdas::any_predicate, lambdas::any_printer>>
  inline
  auto
  any_matcher_impl(char const* type_name, std::false_type)
  TROMPELOEIL_TRAILING_RETURN_TYPE(R)
  {
    return make_matcher<T>(lambdas::any_predicate(), lambdas::any_printer(type_name));
  }

  template <typename T>
  wildcard
  any_matcher_impl(char const*, std::true_type);

  template <typename T>
  inline
  auto
  any_matcher(char const* name)
  TROMPELOEIL_TRAILING_RETURN_TYPE(decltype(any_matcher_impl<T>(name, std::is_array<T>{})))
  {
    static_assert(!std::is_array<T>::value,
                  "array parameter type decays to pointer type for ANY()"
                  " matcher. Please rephrase as pointer instead");
    return any_matcher_impl<T>(name, std::is_array<T>{});
  }

}

#define TROMPELOEIL_ANY(type) ::trompeloeil::any_matcher<type>(#type)

#ifndef TROMPELOEIL_LONG_MACROS
#define ANY                       TROMPELOEIL_ANY
#endif

#endif //TROMPELOEIL_ANY_HPP
