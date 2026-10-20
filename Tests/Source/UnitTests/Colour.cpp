#include <RmlUi/Core/Types.h>
#include <doctest.h>

using namespace Rml;

TEST_CASE("Colour.ByteLayout")
{
	{
		Colourb c;
		REQUIRE(&c.red == &c.data()[0]);
		REQUIRE(&c.green == &c.data()[1]);
		REQUIRE(&c.blue == &c.data()[2]);
		REQUIRE(&c.alpha == &c.data()[3]);
	}

	{
		Colourf c;
		REQUIRE(&c.red == &c.data()[0]);
		REQUIRE(&c.green == &c.data()[1]);
		REQUIRE(&c.blue == &c.data()[2]);
		REQUIRE(&c.alpha == &c.data()[3]);
	}
}

TEST_CASE("Colour.ArrayByteLayout")
{
	// Ensure Colour[] can be passed as float* for glUniform4fv
	{
		Colourf colours[2];
		REQUIRE(&colours[0].data()[4] == colours[1].data());
	}
}

