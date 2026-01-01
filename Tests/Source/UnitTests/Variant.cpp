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
