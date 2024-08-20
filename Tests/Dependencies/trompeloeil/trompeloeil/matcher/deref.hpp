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


#ifndef TROMPELOEIL_DEREF_HPP
#define TROMPELOEIL_DEREF_HPP

#ifndef TROMPELOEIL_MATCHER_HPP
#include "../matcher.hpp"
#endif

namespace trompeloeil {

template <typename M>
class ptr_deref : public matcher
{
public:
  template <typename U,
    typename = decltype(can_match_parameter<detail::remove_reference_t<decltype(*std::declval<U>())>>(std::declval<M>()))>
  operator U() const;

  template <typename U>
  explicit
  ptr_deref(
    U&& m_)
    : m( std::forward<U>(m_) )
  {}

  template <typename U>
  bool
  matches(
    const U& u)
  const
  noexcept(noexcept(std::declval<M>().matches(*u)))
  {
    return (u != nullptr) && m.matches(*u);
  }

  friend
  std::ostream&
  operator<<(
    std::ostream& os,
    ptr_deref<M> const& p)
  {
    return os << p.m;
  }
  friend
  std::string
  param_name_prefix(
    const ptr_deref*)
  {
    return "*" + param_name_prefix(static_cast<M*>(nullptr));
  }

private:
  M m;
};


template <typename M,
  typename = detail::enable_if_t<::trompeloeil::is_matcher<M>::value>>
inline
::trompeloeil::ptr_deref<detail::decay_t<M>>
operator*(
  M&& m)
{
  return ::trompeloeil::ptr_deref<detail::decay_t<M>>{std::forward<M>(m)};
}


}
#endif //TROMPELOEIL_DEREF_HPP
