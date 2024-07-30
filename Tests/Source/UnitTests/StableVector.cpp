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

#include <RmlUi/Core/StableVector.h>
#include <doctest.h>

using namespace Rml;

TEST_CASE("StableVector")
{
	StableVector<int> v;

	REQUIRE(v.empty() == true);
	REQUIRE(v.size() == 0);

	const int a = 3;
	const auto index_a = v.insert(a);
	REQUIRE(!v.empty());
	REQUIRE(v.size() == 1);

	const int b = 4;
	const auto index_b = v.insert(b);
	REQUIRE(!v.empty());
	REQUIRE(v.size() == 2);

	const int expected_values[] = {a, b};
	v.for_each([&, i = 0](int& value) mutable {
		REQUIRE(value == expected_values[i]);
		i++;
	});

	REQUIRE(v[index_a] == a);
	REQUIRE(v[index_b] == b);

	const int a_out = v.erase(index_a);
	REQUIRE(a_out == a);
	REQUIRE(v.size() == 1);
	REQUIRE(v[index_b] == b);

	const int b_out = v.erase(index_b);
	REQUIRE(b_out == b);
	REQUIRE(v.empty());
	REQUIRE(v.size() == 0);

	const int c = 5;
	const auto index_c = v.insert(c);
	REQUIRE(!v.empty());
	REQUIRE(v.size() == 1);
	REQUIRE(v[index_c] == c);
	v.for_each([&](int& value) { REQUIRE(value == c); });
}
