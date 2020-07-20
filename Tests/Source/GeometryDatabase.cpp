/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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

#include "../../../Source/Core/GeometryDatabase.cpp"
#include <doctest.h>

using namespace Rml;


TEST_CASE("Geometry database")
{
	std::vector<Geometry> geometry_list(10);

	auto list_database_equivalent = [&geometry_list]() -> bool {
		int i = 0;
		bool result = true;
		GeometryDatabase::geometry_database.for_each([&geometry_list, &i, &result](Geometry* geometry) {
			result &= (geometry == &geometry_list[i++]);
			});
		return result;
	};

	int i = 0;
	for (auto& geometry : geometry_list)
		geometry.GetIndices().push_back(i++);

	CHECK(list_database_equivalent());

	geometry_list.reserve(2000);
	CHECK(list_database_equivalent());

	geometry_list.erase(geometry_list.begin() + 5);
	CHECK(list_database_equivalent());

	std::swap(geometry_list.front(), geometry_list.back());
	geometry_list.pop_back();
	CHECK(list_database_equivalent());

	std::swap(geometry_list.front(), geometry_list.back());
	CHECK(list_database_equivalent());

	geometry_list.emplace_back();
	CHECK(list_database_equivalent());

	geometry_list.clear();
	CHECK(list_database_equivalent());
}
