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

#ifndef TROMPELOEIL_LIFETIME_HPP
#define TROMPELOEIL_LIFETIME_HPP

#ifndef TROMPELOEIL_MOCK_HPP_
#include "mock.hpp"
#endif

#if defined(__cxx_rtti) || defined(__GXX_RTTI) || defined(_CPPRTTI)
#  define TROMPELOEIL_TYPE_ID_NAME(x) typeid(x).name()
#else
#  define TROMPELOEIL_TYPE_ID_NAME(x) "object"
#endif

namespace trompeloeil
{
struct lifetime_monitor;

template <typename T>
class deathwatched : public T
{
  static_assert(std::has_virtual_destructor<T>::value,
                "virtual destructor is a necessity for deathwatched to work");
public:
  template <typename ... U,
    typename = detail::enable_if_t<std::is_constructible<T,U...>::value>>
  deathwatched(
    U&& ...u)
  noexcept(noexcept(T(std::declval<U>()...)))
    : T(std::forward<U>(u)...)
  {}

  ~deathwatched() override;

  trompeloeil::lifetime_monitor*&
  trompeloeil_expect_death(
    trompeloeil::lifetime_monitor* monitor)
  const
  noexcept
  {
    auto lock = get_lock();
    trompeloeil_lifetime_monitor = monitor;
    return trompeloeil_lifetime_monitor.leak();
  }
private:
  mutable null_on_move<trompeloeil::lifetime_monitor> trompeloeil_lifetime_monitor;
};

struct lifetime_monitor : public expectation
{
  template <typename T>
  lifetime_monitor(
    ::trompeloeil::deathwatched<T> const &obj,
    char const* obj_name_,
    char const* invocation_name_,
    char const* call_name_,
    location loc_)
  noexcept
    : object_monitor(obj.trompeloeil_expect_death(this))
      , loc(loc_)
      , object_name(obj_name_)
      , invocation_name(invocation_name_)
      , call_name(call_name_)
  {
  }

  bool is_satisfied() const noexcept override
  {
    return died;
  }

  bool is_saturated() const noexcept override
  {
    return died;
  }

  lifetime_monitor(lifetime_monitor const&) = delete;

  ~lifetime_monitor() override
  {
    auto lock = get_lock();
    if (!died)
    {
      std::ostringstream os;
      os << "Object " << object_name << " is still alive";
      send_report<specialized>(severity::nonfatal, loc, os.str());
      object_monitor = nullptr; // prevent its death poking this cadaver
    }
  }

  lifetime_monitor& operator=(lifetime_monitor const&) = delete;

  void
  notify()
  noexcept
  {
    died = true;
    sequences->validate(severity::nonfatal, call_name, loc);

    sequences->increment_call();
    if (sequences->is_satisfied())
    {
      sequences->retire_predecessors();
    }
  }

  template <typename ... T>
  void
  set_sequence(
    T&& ... t)
  {
    using handler = sequence_handler<sizeof...(T)>;
    auto seq = detail::make_unique<handler>(*sequences,
                                            invocation_name,
                                            loc,
                                            std::forward<T>(t)...);
    sequences = std::move(seq);
  }
private:
  atomic<bool>       died{false};
  lifetime_monitor *&object_monitor;
  location           loc;
  char const        *object_name;
  char const        *invocation_name;
  char const        *call_name;
  std::unique_ptr<sequence_handler_base>  sequences = detail::make_unique<sequence_handler<0>>();
};

template <typename T>
deathwatched<T>::~deathwatched()
{
  auto lock = get_lock();
  if (trompeloeil_lifetime_monitor)
  {
    trompeloeil_lifetime_monitor->notify();
    return;
  }
  std::ostringstream os;
  os << "Unexpected destruction of "
     << TROMPELOEIL_TYPE_ID_NAME(T) << "@" << this << '\n';
  send_report<specialized>(severity::nonfatal,
                           location{},
                           os.str());
}

template <bool sequence_set>
struct lifetime_monitor_modifier : std::unique_ptr<lifetime_monitor>
{
  explicit
  lifetime_monitor_modifier(
    std::unique_ptr<lifetime_monitor>&& p)
  noexcept
    : unique_ptr(std::move(p))
  {}

  template <typename ... T, bool b = sequence_set>
  auto
  in_sequence(T&& ... t)
  -> lifetime_monitor_modifier<true>
  {
    static_assert(!b,
                  "Multiple IN_SEQUENCE does not make sense."
                  " You can list several sequence objects at once");
    std::unique_ptr<lifetime_monitor>& m = *this;
    m->set_sequence(std::forward<T>(t)...);
    return lifetime_monitor_modifier<true>{std::move(m)};
  }
};

struct lifetime_monitor_releaser
{
  template <bool b>
  std::unique_ptr<trompeloeil::lifetime_monitor>
  operator+(
    lifetime_monitor_modifier<b>&& m)
  const
  {
    return std::move(m);
  }
};

}

#define TROMPELOEIL_REQUIRE_DESTRUCTION(obj)                                   \
  TROMPELOEIL_REQUIRE_DESTRUCTION_(obj, #obj)

#define TROMPELOEIL_REQUIRE_DESTRUCTION_(obj, obj_s)                           \
  std::unique_ptr<trompeloeil::expectation>                                    \
    TROMPELOEIL_CONCAT(trompeloeil_death_monitor_, __LINE__)                   \
    = TROMPELOEIL_NAMED_REQUIRE_DESTRUCTION_(,obj, obj_s)

#define TROMPELOEIL_NAMED_REQUIRE_DESTRUCTION(obj)                             \
  TROMPELOEIL_NAMED_REQUIRE_DESTRUCTION_("NAMED_", obj, #obj)

#define TROMPELOEIL_NAMED_REQUIRE_DESTRUCTION_(prefix, obj, obj_s)             \
  trompeloeil::lifetime_monitor_releaser{} +                                   \
  trompeloeil::lifetime_monitor_modifier<false>{                               \
    ::trompeloeil::detail::make_unique<trompeloeil::lifetime_monitor>(         \
      obj,                                                                     \
      obj_s,                                                                   \
      prefix "REQUIRE_DESTRUCTION(" obj_s ")",                                 \
      "destructor for " obj_s,                                                 \
      ::trompeloeil::location{__FILE__,                                        \
                              static_cast<unsigned long>(__LINE__)})           \
  }

#ifndef TROMPELOEIL_LONG_MACROS
#define REQUIRE_DESTRUCTION       TROMPELOEIL_REQUIRE_DESTRUCTION
#define NAMED_REQUIRE_DESTRUCTION TROMPELOEIL_NAMED_REQUIRE_DESTRUCTION
#endif

#endif //TROMPELOEIL_LIFETIME_HPP
