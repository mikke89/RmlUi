#include <RmlUi/Core/Math.h>
#include <RmlUi/Core/Types.h>
#include <doctest.h>

using namespace Rml;

TEST_CASE("Math.RoundedLerp")
{
	const ColourbPremultiplied c0(0, 0, 0, 255);
	const ColourbPremultiplied c1(255, 0, 0, 255);
	const ColourbPremultiplied c2(127, 0, 0, 255);

	ColourbPremultiplied c;

	c = Math::RoundedLerp(0.0f, c0, c1);
	REQUIRE(c.red == c0.red);

	c = Math::RoundedLerp(179.f / 255.f, c0, c1);
	REQUIRE(c.red == 179);

	c = Math::RoundedLerp(50.4f / 255.f, c0, c1);
	REQUIRE(c.red == 50);

	c = Math::RoundedLerp(50.6f / 255.f, c0, c1);
	REQUIRE(c.red == 51);

	c = Math::RoundedLerp(0.0f, c1, c0);
	REQUIRE(c.red == c1.red);

	c = Math::RoundedLerp(1.0f, c0, c1);
	REQUIRE(c.red == c1.red);

	c = Math::RoundedLerp(1.0f, c1, c0);
	REQUIRE(c.red == c0.red);

	c = Math::RoundedLerp(1.0f, c0, c2);
	REQUIRE(c.red == c2.red);

	c = Math::RoundedLerp(0.0f, c2, c1);
	REQUIRE(c.red == c2.red);

	// Full-range interpolation
	for (int i = 0; i <= 255; i++)
	{
		const float t = float(i) / 255.f;
		const auto r = Math::RoundedLerp(t, c0, c1).red;
		REQUIRE(r == i);
	}

	// Reverse full-range interpolation
	for (int i = 0; i <= 255; i++)
	{
		const float t = float(i) / 255.f;
		const auto r = Math::RoundedLerp(t, c1, c0).red;
		REQUIRE(r == 255 - i);
	}

	// Half-range interpolation
	for (int i = 0; i <= 255; i++)
	{
		const float t = float(i) / 255.f;
		const auto r = Math::RoundedLerp(t, c0, c2).red;
		REQUIRE(r == i / 2);
	}

	// Same-color interpolation with fractional t
	for (int i = 0; i <= 379; i++)
	{
		const float t = float(i) / 379.f;
		const auto r0 = Math::RoundedLerp(t, c0, c0).red;
		const auto r1 = Math::RoundedLerp(t, c1, c1).red;
		const auto r2 = Math::RoundedLerp(t, c2, c2).red;
		REQUIRE(r0 == c0.red);
		REQUIRE(r1 == c1.red);
		REQUIRE(r2 == c2.red);
	}
}

TEST_CASE("Math.Clamp")
{
	// Clamp(Value, Min, Max)
	CHECK(Math::Clamp(1, 1, 2) == 1);
	CHECK(Math::Clamp(0, 1, 2) == 1);
	CHECK(Math::Clamp(3, 1, 2) == 2);

	CHECK(Math::Clamp(0, 2, 1) == 2);
	CHECK(Math::Clamp(3, 2, 1) == 1);

	CHECK(Math::Clamp<Vector2i>({1, 1}, {1, 1}, {2, 2}) == Vector2i(1, 1));
	CHECK(Math::Clamp<Vector2i>({0, 0}, {1, 1}, {2, 2}) == Vector2i(1, 1));
	CHECK(Math::Clamp<Vector2i>({3, 3}, {1, 1}, {2, 2}) == Vector2i(2, 2));
	CHECK(Math::Clamp<Vector2i>({1, 3}, {1, 1}, {2, 2}) == Vector2i(1, 2));
	CHECK(Math::Clamp<Vector2i>({1, 1}, {2, 0}, {2, 0}) == Vector2i(2, 0));

	CHECK(Math::Clamp<Vector2f>({1, 1}, {1, 1}, {2, 2}) == Vector2f(1, 1));
	CHECK(Math::Clamp<Vector2f>({0, 0}, {1, 1}, {2, 2}) == Vector2f(1, 1));
	CHECK(Math::Clamp<Vector2f>({3, 3}, {1, 1}, {2, 2}) == Vector2f(2, 2));
	CHECK(Math::Clamp<Vector2f>({1, 3}, {1, 1}, {2, 2}) == Vector2f(1, 2));
	CHECK(Math::Clamp<Vector2f>({1, 1}, {2, 0}, {2, 0}) == Vector2f(2, 0));
}

TEST_CASE("Math.Min")
{
	CHECK(Math::Min(1, 2) == 1);
	CHECK(Math::Min(2, 1) == 1);

	CHECK(Math::Min<Vector2i>({1, 1}, {2, 2}) == Vector2i(1, 1));
	CHECK(Math::Min<Vector2i>({2, 2}, {1, 1}) == Vector2i(1, 1));
	CHECK(Math::Min<Vector2i>({2, 1}, {1, 2}) == Vector2i(1, 1));

	CHECK(Math::Min<Vector2f>({1, 1}, {2, 2}) == Vector2f(1, 1));
	CHECK(Math::Min<Vector2f>({2, 2}, {1, 1}) == Vector2f(1, 1));
	CHECK(Math::Min<Vector2f>({2, 1}, {1, 2}) == Vector2f(1, 1));
}

TEST_CASE("Math.Max")
{
	CHECK(Math::Max(1, 2) == 2);
	CHECK(Math::Max(2, 1) == 2);

	CHECK(Math::Max<Vector2i>({1, 1}, {2, 2}) == Vector2i(2, 2));
	CHECK(Math::Max<Vector2i>({2, 2}, {1, 1}) == Vector2i(2, 2));
	CHECK(Math::Max<Vector2i>({2, 1}, {1, 2}) == Vector2i(2, 2));

	CHECK(Math::Max<Vector2f>({1, 1}, {2, 2}) == Vector2f(2, 2));
	CHECK(Math::Max<Vector2f>({2, 2}, {1, 1}) == Vector2f(2, 2));
	CHECK(Math::Max<Vector2f>({2, 1}, {1, 2}) == Vector2f(2, 2));
}

TEST_CASE("Math.Round")
{
	CHECK(Math::Round(-1.0f) == -1.f);
	CHECK(Math::Round(0.0f) == 0.f);
	CHECK(Math::Round(1.0f) == 1.f);

	CHECK(Math::Round(0.49f) == 0.f);
	CHECK(Math::Round(0.50f) == 1.f);
	CHECK(Math::Round(0.51f) == 1.f);
	CHECK(Math::Round(-0.49f) == 0.f);
	CHECK(Math::Round(-0.50f) == 0.f); // Always round half up, see commit message.
	CHECK(Math::Round(-0.51f) == -1.f);

	CHECK(Math::Round(1000.49f) == 1000.f);
	CHECK(Math::Round(1000.50f) == 1001.f);
	CHECK(Math::Round(1000.51f) == 1001.f);
	CHECK(Math::Round(-1000.49f) == -1000.f);
	CHECK(Math::Round(-1000.50f) == -1000.f);
	CHECK(Math::Round(-1000.51f) == -1001.f);

	CHECK(Math::Round(100000.49f) == 100000.f);
	CHECK(Math::Round(100000.50f) == 100001.f);
	CHECK(Math::Round(100000.51f) == 100001.f);
	CHECK(Math::Round(-100000.49f) == -100000.f);
	CHECK(Math::Round(-100000.50f) == -100000.f);
	CHECK(Math::Round(-100000.51f) == -100001.f);
}
