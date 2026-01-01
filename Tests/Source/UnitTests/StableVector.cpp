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
