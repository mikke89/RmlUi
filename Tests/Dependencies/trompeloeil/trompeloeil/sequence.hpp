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

#ifndef TROMPELOEIL_SEQUENCE_HPP
#define TROMPELOEIL_SEQUENCE_HPP

#ifndef TROMPELOEIL_MOCK_HPP_
#include "mock.hpp"
#endif

namespace trompeloeil {

  class sequence_type
  {
  public:
    sequence_type() = default;
    sequence_type(sequence_type&&) = delete;
    sequence_type(const sequence_type&) = delete;
    sequence_type& operator=(sequence_type&&) = delete;
    sequence_type& operator=(const sequence_type&) = delete;
    ~sequence_type();

    bool
    is_completed()
    const
    noexcept;

    bool
    is_first(
      sequence_matcher const *m)
    const
    noexcept;

    unsigned
    cost(
      sequence_matcher const *m)
    const
    noexcept;

    void
    retire_until(
      sequence_matcher const* m)
    noexcept;

    void
    add_last(
      sequence_matcher *m)
    noexcept;

    void
    validate_match(
      severity s,
      sequence_matcher const *matcher,
      char const *seq_name,
      char const *match_name,
      location loc)
    const;

  private:
    list<sequence_matcher> matchers{};
  };

  class sequence
  {
  public:
    sequence_type& operator*() { return *obj; }
    bool is_completed() const { return obj->is_completed(); }
  private:
    std::unique_ptr<sequence_type> obj{detail::make_unique<sequence_type>()};
  };

  class sequence_matcher : public list_elem<sequence_matcher>
  {
  public:
    using init_type = std::pair<char const*, sequence&>;

    sequence_matcher(
      char const *exp,
      location loc,
      const sequence_handler_base& handler,
      init_type i)
    noexcept
      : seq_name(i.first)
      , exp_name(exp)
      , exp_loc(loc)
      , sequence_handler(handler)
      , seq(*i.second)
    {
      auto lock = get_lock();
      seq.add_last(this);
    }

    sequence_matcher(const sequence_matcher&) = delete;
    sequence_matcher(sequence_matcher&&) = default;
    sequence_matcher& operator=(sequence_matcher const&) = delete;

    void
    validate_match(
      severity s,
      char const *match_name,
      location loc)
    const
    {
      seq.validate_match(s, this, seq_name, match_name, loc);
    }

    unsigned
    cost()
    const
    noexcept
    {
      return seq.cost(this);
    }

    bool
    is_satisfied()
    const
    noexcept;

    bool
    is_optional()
    const
    noexcept;

    void
    retire()
    noexcept
    {
      this->unlink();
    }

    void
    retire_predecessors()
    noexcept
    {
      seq.retire_until(this);
    }

    void
    print_expectation(std::ostream& os)
    const
    {
      os << exp_name << " at " << exp_loc;
    }

    char const*
    sequence_name()
    const
    noexcept
    {
      return seq_name;
    }
  private:
    char const *seq_name;
    char const *exp_name;
    location    exp_loc;
    const sequence_handler_base& sequence_handler;
    sequence_type& seq;
  };

  inline
  bool
  sequence_type::is_completed()
  const
  noexcept
  {
    for (const auto& matcher : matchers)
    {
      if (!matcher.is_satisfied())
      {
        return false;
      }
    }
    return true;
  }

  inline
  bool
  sequence_type::is_first(
    sequence_matcher const *m)
  const
  noexcept
  {
    return !matchers.empty() && &*matchers.begin() == m;
  }

  inline
  unsigned
  sequence_type::cost(
    sequence_matcher const* m)
  const
  noexcept
  {
    unsigned sequence_cost = 0U;
    for (auto const& e : matchers)
    {
      if (&e == m) return sequence_cost;
      if (!e.is_satisfied())
      {
        return ~0U;
      }
      ++sequence_cost;
    }
    return ~0U;
  }

  inline
  void
  sequence_type::retire_until(
    sequence_matcher const* m)
  noexcept
  {
    while (!matchers.empty())
    {
      auto first = &*matchers.begin();
      if (first == m) return;
      first->retire();
    }
  }

  inline
  void
  sequence_type::validate_match(
    severity s,
    sequence_matcher const *matcher,
    char const* seq_name,
    char const* match_name,
    location loc)
  const
  {
    if (is_first(matcher)) return;
    if (matchers.empty())
    {
      std::ostringstream os;
      os << "Sequence mismatch for sequence \"" << seq_name
         << "\" with matching call of " << match_name
         << " at " << loc
         << ". Sequence \"" << seq_name
         << "\" has no more pending expectations\n";
      send_report<specialized>(s, loc, os.str());
    }
    bool first = true;
    std::ostringstream os;
    os << "Sequence mismatch for sequence \"" << seq_name
       << "\" with matching call of " << match_name
       << " at " << loc << ".\n";
    for (auto const& m : matchers)
    {
      if (first || !m.is_optional())
      {
        if (first)
        {
          os << "Sequence \"" << seq_name << "\" has ";
        }
        else
        {
          os << "and has ";
        }
        m.print_expectation(os);
        if (m.is_optional())
        {
          os << " first in line\n";
        }
        else
        {
          os << " as first required expectation\n";
          break;
        }
      }
      first = false;
    }
    send_report<specialized>(s, loc, os.str());
  }

  inline
  sequence_type::~sequence_type()
  {
    bool touched = false;
    std::ostringstream os;
    while (!matchers.empty())
    {
      auto m = matchers.begin();
      if (!touched)
      {
        os << "Sequence expectations not met at destruction of sequence object \""
           << m->sequence_name() << "\":";
        touched = true;
      }
      os << "\n  missing ";
      m->print_expectation(os);
      m->unlink();
    }
    if (touched)
    {
      os << "\n";
      send_report<specialized>(severity::nonfatal, location{}, os.str());
    }
  }

  inline
  void
  sequence_type::add_last(
    sequence_matcher *m)
  noexcept
  {
    matchers.push_back(m);
  }

  inline
  bool
  sequence_matcher::is_satisfied()
  const
  noexcept
  {
    return sequence_handler.is_satisfied();
  }

  inline
  bool
  sequence_matcher::is_optional()
  const
  noexcept
  {
    return sequence_handler.get_min_calls() == 0;
  }

  template <size_t N>
  struct sequence_matchers
  {
    void
    validate(
      severity s,
      char const *match_name,
      location loc)
    {
      for (auto& e : matchers)
      {
        e.validate_match(s, match_name, loc);
      }
    }

    unsigned
    order()
      const
      noexcept
    {
      unsigned highest_order = 0U;
      for (auto &m: matchers) {
        auto cost = m.cost();
        if (cost > highest_order) {
          highest_order = cost;
        }
      }
      return highest_order;
    }
    void
    retire()
    noexcept
    {
      for (auto& e : matchers)
      {
        e.retire();
      }
    }
    void
    retire_predecessors()
      noexcept
    {
      for (auto& e : matchers)
      {
        e.retire_predecessors();
      }
    }

    std::array<sequence_matcher, N> matchers;
  };
}

#define TROMPELOEIL_IN_SEQUENCE(...)                                           \
  in_sequence(TROMPELOEIL_INIT_WITH_STR(::trompeloeil::sequence_matcher::init_type, __VA_ARGS__))

#ifndef TROMPELOEIL_LONG_MACRCOS
#define IN_SEQUENCE               TROMPELOEIL_IN_SEQUENCE
#endif

#endif //TROMPELOEIL_SEQUENCE_HPP
