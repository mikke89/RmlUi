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

#include "GeometryDatabase.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include <algorithm>

namespace Rml {
namespace Core {
namespace GeometryDatabase {

class Database {
public:
	Database() {
		constexpr size_t reserve_size = 512;
		geometry_list.reserve(reserve_size);
		free_list.reserve(reserve_size);
	}

	~Database() {
#ifdef RMLUI_TESTS_ENABLED
		RMLUI_ASSERT(geometry_list.size() == free_list.size());
		std::sort(free_list.begin(), free_list.end());
		for (size_t i = 0; i < free_list.size(); i++)
		{
			RMLUI_ASSERT(i == free_list[i]);
		}
#endif
	}

	GeometryDatabaseHandle insert(Geometry* value)
	{
		GeometryDatabaseHandle handle;
		if (free_list.empty())
		{
			handle = GeometryDatabaseHandle(geometry_list.size());
			geometry_list.push_back(value);
		}
		else
		{
			handle = free_list.back();
			free_list.pop_back();
			geometry_list[handle] = value;
		}
		return handle;
	}

	void erase(GeometryDatabaseHandle handle)
	{
		free_list.push_back(handle);
	}

	// Iterate over every item in the database, skipping free slots.
	template<typename Func>
	void for_each(Func func)
	{
		std::sort(free_list.begin(), free_list.end());

		size_t i_begin_next = 0;
		for (GeometryDatabaseHandle freelist_entry : free_list)
		{
			const size_t i_end = size_t(freelist_entry);
			const size_t i_begin = i_begin_next;
			i_begin_next = i_end + 1;

			for (size_t i = i_begin; i < i_end; i++)
				func(geometry_list[i]);
		}

		for (size_t i = i_begin_next; i < geometry_list.size(); i++)
				func(geometry_list[i]);
	}

private:
	// List of all active geometry, in addition to free slots.
	// Free slots (as defined by the 'free_list') may contain dangling pointers and must not be dereferenced.
	std::vector<Geometry*> geometry_list;
	// Declares free slots in the 'geometry_list' as indices.
	std::vector<GeometryDatabaseHandle> free_list;
};


static Database geometry_database;

GeometryDatabaseHandle Insert(Geometry* geometry)
{
	return geometry_database.insert(geometry);
}

void Erase(GeometryDatabaseHandle handle)
{
	geometry_database.erase(handle);
}

void ReleaseAll()
{
	geometry_database.for_each([](Geometry* geometry) {
		geometry->Release();
	});
}



#ifdef RMLUI_TESTS_ENABLED

static class TestGeometryDatabase {
private:
	std::vector<Geometry> geometry_list;

	bool list_database_equivalent()
	{
		int i = 0;
		bool result = true;
		GetDatabase().for_each([this, &i, &result](Geometry* geometry) {
			result &= (geometry == &geometry_list[i++]);
		});
		return result;
	}

public:
	TestGeometryDatabase() : geometry_list(10)
	{
		bool result = true;

		int i = 0;
		for (auto& geometry : geometry_list)
			geometry.GetIndices().push_back(i++);

		result &= list_database_equivalent();

		geometry_list.reserve(2000);
		result &= list_database_equivalent();

		geometry_list.erase(geometry_list.begin() + 5);
		result &= list_database_equivalent();

		std::swap(geometry_list.front(), geometry_list.back());
		geometry_list.pop_back();
		result &= list_database_equivalent();

		std::swap(geometry_list.front(), geometry_list.back());
		result &= list_database_equivalent();

		geometry_list.emplace_back();
		result &= list_database_equivalent();

		geometry_list.clear();
		result &= list_database_equivalent();

		RMLUI_ASSERT(result);
	}

} test_geometry_database;

#endif

}
}
}
