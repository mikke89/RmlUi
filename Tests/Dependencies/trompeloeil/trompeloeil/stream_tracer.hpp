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

#ifndef TROMPELOEIL_STREAM_TRACER_HPP
#define TROMPELOEIL_STREAM_TRACER_HPP

#ifndef TROMPELOEIL_MOCK_HPP_
#include "mock.hpp"
#endif

namespace trompeloeil {

class stream_tracer : public tracer
{
public:
  explicit
  stream_tracer(
    std::ostream& stream_)
    : stream(stream_) {}
  void
  trace(
    char const *file,
    unsigned long line,
    std::string const &call)
  override
  {
    stream << location{file, line} << '\n' << call << '\n';
  }
private:
  std::ostream& stream;
};

}
#endif //TROMPELOEIL_STREAM_TRACER_HPP
