/*
 * Trompeloeil C++ mocking framework
 *
 * Copyright Björn Fahller 2014-2022
 *
 *  Use, modification and distribution is subject to the
 *  Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 *
 * Project home: https://github.com/rollbear/trompeloeil
 */


#ifndef TROMPELOEIL_DOCTEST_HPP_
#define TROMPELOEIL_DOCTEST_HPP_

#ifndef DOCTEST_VERSION_MAJOR
#error "<doctest.h> must be included before <doctest/trompeloeil.hpp>"
#endif

#include "../trompeloeil.hpp"

namespace trompeloeil
{
  template <>
  inline void reporter<specialized>::send(
    severity s,
    const char* file,
    unsigned long line,
    const char* msg)
  {
    doctest::String msgstr(msg);
    auto f = line ? file : "[file/line unavailable]";
    if (s == severity::fatal)
    {
      DOCTEST_ADD_FAIL_AT(f, line, msgstr);
    }
    else
    {
      DOCTEST_ADD_FAIL_CHECK_AT(f, line, msgstr);
    }
  }

  template <>
  inline void reporter<specialized>::sendOk(
    const char* trompeloeil_mock_calls_done_correctly)
  {
#ifdef DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
    DOCTEST_REQUIRE_UNARY(trompeloeil_mock_calls_done_correctly);
#else
    DOCTEST_REQUIRE_NE(doctest::String(trompeloeil_mock_calls_done_correctly), "");
#endif
  }
}


#endif //TROMPELOEIL_DOCTEST_HPP_
