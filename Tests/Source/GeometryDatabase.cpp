#include <doctest.h>

#include "../../../Source/Core/GeometryDatabase.cpp"

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
