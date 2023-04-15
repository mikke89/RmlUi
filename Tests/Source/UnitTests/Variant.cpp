/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <RmlUi/Core/Types.h>
#include <RmlUi/Core/Variant.h>
#include <doctest.h>

using namespace Rml;

TEST_CASE("Variant.ScopedEnum")
{
	enum class X : uint64_t {
		A = 1,
		B = 5,
		C = UINT64_MAX,
	};

	X e1 = X::A;
	const X e2 = X::B;
	const X& e3 = e1;

	Variant v1 = Variant(X::A);
	Variant v2(X::B);
	Variant v3(X::C);

	Variant v4(e1);
	Variant v5(e2);
	Variant v6(e3);

	CHECK(v1.Get<X>() == X::A);
	CHECK(v2.Get<X>() == X::B);
	CHECK(v3.Get<X>() == X::C);

	CHECK(v4.Get<X>() == X::A);
	CHECK(v5.Get<X>() == X::B);
	CHECK(v6.Get<X>() == X::A);

	CHECK(v1 != v2);
	CHECK(v1 == v4);

	Variant v7 = v5;
	CHECK(v7.Get<X>() == X::B);

	CHECK(v1.Get<int>() == 1);
	CHECK(v2.Get<int>() == 5);
	CHECK(v3.Get<uint64_t>() == UINT64_MAX);
	CHECK(v3.Get<int64_t>() == static_cast<int64_t>(UINT64_MAX));
}

TEST_CASE("Variant.UnscopedEnum")
{
	enum X : uint64_t {
		A = 1,
		B = 5,
		C = UINT64_MAX,
	};

	X e1 = X::A;
	const X e2 = X::B;
	const X& e3 = e1;

	Variant v1 = Variant(X::A);
	Variant v2(X::B);
	Variant v3(X::C);

	Variant v4(e1);
	Variant v5(e2);
	Variant v6(e3);

	CHECK(v1.Get<X>() == X::A);
	CHECK(v2.Get<X>() == X::B);
	CHECK(v3.Get<X>() == X::C);

	CHECK(v4.Get<X>() == X::A);
	CHECK(v5.Get<X>() == X::B);
	CHECK(v6.Get<X>() == X::A);

	CHECK(v1 != v2);
	CHECK(v1 == v4);

	Variant v7 = v5;
	CHECK(v7.Get<X>() == X::B);

	CHECK(v1.Get<int>() == 1);
	CHECK(v2.Get<int>() == 5);
	CHECK(v3.Get<uint64_t>() == UINT64_MAX);
	CHECK(v3.Get<int64_t>() == static_cast<int64_t>(UINT64_MAX));
}
