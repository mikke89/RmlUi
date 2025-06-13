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

#ifndef TROMPELOEIL_RE_HPP
#define TROMPELOEIL_RE_HPP

#ifndef TROMPELOEIL_MATCHER_HPP
#include "../matcher.hpp"
#endif

#include <regex>
#include <cstring>

namespace trompeloeil
{
namespace lambdas {
struct regex_check
{

  class string_helper // a vastly simplified string_view type of class
  {
  public:
    template <
      typename S,
      typename = decltype(std::declval<const char*&>() = std::declval<S>().data()),
      typename = decltype(std::declval<S>().length())
    >
    string_helper(
      const S& s)
    noexcept
      : begin_(s.data())
        , end_(begin_ + s.length())
    {}

    constexpr
    string_helper(
      char const* s)
    noexcept
      : begin_(s)
        , end_(s ? begin_ + strlen(s) : nullptr)
    {
    }

    constexpr
    explicit
    operator bool() const
    {
      return begin_;
    }
    constexpr
    char const *
    begin()
    const
    {
      return begin_;
    }
    constexpr
    char const *
    end()
    const
    {
      return end_;
    }
  private:
    char const* begin_;
    char const* end_;
  };

  regex_check(
    std::regex&& re_,
    std::regex_constants::match_flag_type match_type_)
    : re(std::move(re_)),
    match_type(match_type_)
  {}

  template <typename T>
  bool
  operator()(
    string_helper str,
    T const&)
  const
  {
    return str && std::regex_search(str.begin(), str.end(), re, match_type);
  }

private:
  std::regex re;
  std::regex_constants::match_flag_type match_type;
};

struct regex_printer
{
  template <typename T>
  void
  operator()(
    std::ostream& os,
    T const& str)
  const
  {
    os << " matching regular expression /" << str << "/";
  }
};
}

template <
  typename Kind = wildcard,
  typename R = make_matcher_return<Kind, lambdas::regex_check, lambdas::regex_printer, std::string&&>>
auto
re(
  std::string s,
  std::regex_constants::syntax_option_type opt = std::regex_constants::ECMAScript,
  std::regex_constants::match_flag_type match_type = std::regex_constants::match_default)
TROMPELOEIL_TRAILING_RETURN_TYPE(R)
{
  return make_matcher<Kind>(lambdas::regex_check(std::regex(s, opt),
                                                 match_type),
                            lambdas::regex_printer(),
                            std::move(s));
}

template <
  typename Kind = wildcard,
  typename R = make_matcher_return<Kind, lambdas::regex_check, lambdas::regex_printer, std::string&&>>
auto
re(
  std::string s,
  std::regex_constants::match_flag_type match_type)
TROMPELOEIL_TRAILING_RETURN_TYPE(R)
{
  return make_matcher<Kind>(lambdas::regex_check(std::regex(s), match_type),
                            lambdas::regex_printer(),
                            std::move(s));
}


}
#endif //TROMPELOEIL_RE_HPP
