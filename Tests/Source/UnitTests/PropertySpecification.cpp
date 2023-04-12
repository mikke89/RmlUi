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

#include "../Common/TestsInterface.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/PropertyDefinition.h>
#include <RmlUi/Core/PropertyDictionary.h>
#include <RmlUi/Core/PropertySpecification.h>
#include <doctest.h>
#include <limits.h>

using namespace Rml;

TEST_CASE("PropertySpecification")
{
	TestsSystemInterface system_interface;
	TestsRenderInterface render_interface;
	SetRenderInterface(&render_interface);
	SetSystemInterface(&system_interface);
	Rml::Initialise();

	PropertySpecification specification(1, 0);
	const PropertyId id = specification.RegisterProperty("name", "", false, false).AddParser("string").GetId();

	// Parse value into the <string> property.
	auto Parse = [&](const String& test_value, const String& expected) {
		PropertyDictionary properties;
		const bool parse_success = specification.ParsePropertyDeclaration(properties, id, test_value);
		const auto num_properties = properties.GetProperties().size();
		CHECK(parse_success);
		CHECK(num_properties == 1);
		CHECK(properties.GetProperty(id));

		if (const Property* property = properties.GetProperty(id))
		{
			const String parsed_value = property->Get<String>();
			CHECK_MESSAGE(parsed_value == expected, "Test value: ", test_value);
		}
	};

	Parse("a", "a");
	Parse(" a ", "a");
	Parse("green", "green");

	Parse("image(ress:///.ress#/images/a.png)", "image(ress:///.ress#/images/a.png)");
	Parse(R"(image("ress:///.ress#/images/a.png"))", R"(image("ress:///.ress#/images/a.png"))");
	Parse(R"("ress:///.ress#/images/a.png")", R"(ress:///.ress#/images/a.png)");

	Parse(R"("escaped\"quotes")", R"(escaped"quotes)");
	Parse(R"("escaped\\backslash")", R"(escaped\backslash)");
	Parse(R"("bad_\escape")", R"(bad_\escape)");

	Parse(R"(C:\Windows\test.png)", R"(C:\Windows\test.png)");
	Parse(R"("C:\Windows\test.png")", R"(C:\Windows\test.png)");
	Parse(R"(C:\\Windows\\test.png)", R"(C:\\Windows\\test.png)");
	Parse(R"("C:\\Windows\\test.png")", R"(C:\Windows\test.png)");

	Parse(R"(\\host\test.png)", R"(\\host\test.png)");
	Parse(R"(\\\host\test.png)", R"(\\\host\test.png)");
	Parse(R"("\\host\\test.png")", R"(\host\test.png)");

	Parse("image(a)", "image(a)");
	Parse(R"(image(a))", R"(image(a))");
	Parse(R"(image(a, "b"))", R"(image(a, "b"))");
	Parse(R"V("image(a, \"b\")")V", R"V(image(a, "b"))V");

	Parse(R"(image( ))", R"(image( ))");
	Parse(R"(image( a\)b ))", R"(image( a)b ))");
	Parse(R"(image("a\)b"))", R"(image("a)b"))");
	Parse(R"(image( a\\b ))", R"(image( a\b ))");
	Parse(R"(image( a\\\b ))", R"(image( a\\b ))");
	Parse(R"(image( a\\\\b ))", R"(image( a\\b ))");

	Rml::Shutdown();
}

TEST_CASE("PropertyParser.Keyword")
{
	TestsSystemInterface system_interface;
	TestsRenderInterface render_interface;
	SetRenderInterface(&render_interface);
	SetSystemInterface(&system_interface);
	Rml::Initialise();

	PropertySpecification specification(20, 0);

	auto Parse = [&](const PropertyId id, const String& test_value, int expected_value) {
		PropertyDictionary properties;
		const bool parse_success = specification.ParsePropertyDeclaration(properties, id, test_value);
		if (expected_value == INT_MAX)
		{
			CHECK(!parse_success);
		}
		else
		{
			CHECK(parse_success);
			CHECK(properties.GetProperties().size() == 1);
			const int parsed_value = properties.GetProperty(id)->Get<int>();
			CHECK_MESSAGE(parsed_value == expected_value, "Test value: ", test_value);

			const String parsed_value_str = properties.GetProperty(id)->ToString();
			CHECK(parsed_value_str == test_value);
		}
	};

	const PropertyId simple = specification.RegisterProperty("simple", "", false, false).AddParser("keyword", "a, b, c").GetId();
	Parse(simple, "a", 0);
	Parse(simple, "b", 1);
	Parse(simple, "c", 2);
	Parse(simple, "d", INT_MAX);
	Parse(simple, "0", INT_MAX);
	Parse(simple, "2", INT_MAX);

	const PropertyId values = specification.RegisterProperty("values", "", false, false).AddParser("keyword", "a=50, b, c=-200").GetId();
	Parse(values, "a", 50);
	Parse(values, "b", 51);
	Parse(values, "c", -200);
	Parse(values, "d", INT_MAX);
	Parse(values, "0", INT_MAX);
	Parse(values, "2", INT_MAX);

	const PropertyId numbers =
		specification.RegisterProperty("numbers", "", false, false).AddParser("keyword", "a=10, b=20, c=30").AddParser("number").GetId();
	Parse(numbers, "a", 10);
	Parse(numbers, "b", 20);
	Parse(numbers, "c", 30);
	Parse(numbers, "d", INT_MAX);
	Parse(numbers, "0", 0);
	Parse(numbers, "2", 2);
	Parse(numbers, "20", 20);

	Rml::Shutdown();
}
