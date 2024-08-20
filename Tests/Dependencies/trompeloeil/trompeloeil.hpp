/*
 * Trompeloeil C++ mocking framework
 *
 * Copyright (C) Björn Fahller 2014-2021
 *
 *  Use, modification and distribution is subject to the
 *  Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy atl
 *  http://www.boost.org/LICENSE_1_0.txt)
 *
 * Project home: https://github.com/rollbear/trompeloeil
 */

#ifndef TROMPELOEIL_HPP_
#define TROMPELOEIL_HPP_


// trompe l'oeil noun    (Concise Encyclopedia)
// Style of representation in which a painted object is intended
// to deceive the viewer into believing it is the object itself...

// project home: https://github.com/rollbear/trompeloeil


// Deficiencies and missing features
// * Mocking function templates is not supported
// * If a macro kills a kitten, this threatens extinction of all felines!

#include "trompeloeil/mock.hpp"
#include "trompeloeil/lifetime.hpp"
#include "trompeloeil/matcher.hpp"
#include "trompeloeil/matcher/any.hpp"
#include "trompeloeil/matcher/compare.hpp"
#include "trompeloeil/matcher/deref.hpp"
#include "trompeloeil/matcher/not.hpp"
#include "trompeloeil/matcher/re.hpp"
#include "trompeloeil/sequence.hpp"
#include "trompeloeil/stream_tracer.hpp"
#ifdef __cpp_impl_coroutine
#include "trompeloeil/coro.hpp"
#endif
#endif // include guard
