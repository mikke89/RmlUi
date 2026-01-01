#include "../Common/TestsInterface.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Factory.h>
#include <RmlUi/Core/PropertyDefinition.h>
#include <RmlUi/Core/PropertyDictionary.h>
#include <RmlUi/Core/PropertySpecification.h>
#include <RmlUi/Core/StyleSheetSpecification.h>
#include <doctest.h>
#include <limits.h>

namespace Rml {
class TestPropertySpecification {
public:
	using SplitOption = PropertySpecification::SplitOption;
	TestPropertySpecification(const PropertySpecification& specification) : specification(specification) {}

	void ParsePropertyValues(StringList& values_list, const String& values, SplitOption split_option) const
	{
		specification.ParsePropertyValues(values_list, values, split_option);
	}

private:
	const PropertySpecification& specification;
};
} // namespace Rml

using namespace Rml;

static String Stringify(const StringList& list)
{
	String result = "[";
	for (int i = 0; i < (int)list.size(); i++)
	{
		if (i != 0)
			result += "; ";
		result += list[i];
	}
	result += ']';
	return result;
}

TEST_CASE("PropertySpecification.ParsePropertyValues")
{
	TestsSystemInterface system_interface;
	TestsRenderInterface render_interface;
	SetRenderInterface(&render_interface);
	SetSystemInterface(&system_interface);
	Rml::Initialise();

	using SplitOption = TestPropertySpecification::SplitOption;
	const TestPropertySpecification& specification = TestPropertySpecification(StyleSheetSpecification::GetPropertySpecification());

	struct Expected {
		Expected(const char* value) : values{String(value)} {}
		Expected(std::initializer_list<String> list) : values(list) {}
		StringList values;
	};

	auto Parse = [&](const String& test_value, const Expected& expected, SplitOption split = SplitOption::Whitespace) {
		StringList parsed_values;
		specification.ParsePropertyValues(parsed_values, test_value, split);
		const String split_str[] = {"none", "whitespace", "comma"};

		INFO("\n\tSplit:     ", split_str[(int)split], "\n\tInput:     ", test_value, "\n\tExpected: ", Stringify(expected.values),
			"\n\tResult:   ", Stringify(parsed_values));
		CHECK(parsed_values == expected.values);
	};

	Parse("red", "red");
	Parse(" red ", "red");
	Parse("inline-block", "inline-block");

	Parse("none red", {"none", "red"});
	Parse("none    red", {"none", "red"});
	Parse("none\t \r \nred", {"none", "red"});

	Parse("none red", "none red", SplitOption::None);
	Parse(" none red ", "none red", SplitOption::None);
	Parse("none    red", "none    red", SplitOption::None);
	Parse("none\t \r \nred", "none\t \r \nred", SplitOption::None);
	Parse("none,red", "none,red", SplitOption::None);
	Parse(" \"none,red\" ", "none,red", SplitOption::None);

	Parse("none,red", {"none,red"});
	Parse("none, red", {"none,", "red"});
	Parse("none , red", {"none", ",", "red"});
	Parse("none   ,   red", {"none", ",", "red"});
	Parse("none,,red", "none,,red");
	Parse("none,,,red", "none,,,red");

	Parse("none,red", {"none", "red"}, SplitOption::Comma);
	Parse("none, red", {"none", "red"}, SplitOption::Comma);
	Parse("none , red", {"none", "red"}, SplitOption::Comma);
	Parse("none   ,   red", {"none", "red"}, SplitOption::Comma);
	Parse("none,,red", {"none", "red"}, SplitOption::Comma);
	Parse("none,,,red", {"none", "red"}, SplitOption::Comma);
	Parse("none, ,  ,red", {"none", "red"}, SplitOption::Comma);

	Parse(R"("name")", {R"("name")"}, SplitOption::Comma);
	Parse(R"("name" none)", {R"("name" none)"}, SplitOption::Comma);
	Parse(R"(none "name")", {R"(none "name")"}, SplitOption::Comma);
	Parse(R"( none "name" )", {R"(none "name")"}, SplitOption::Comma);
	Parse(R"( "name", red )", {R"("name")", R"(red)"}, SplitOption::Comma);
	Parse(R"( "name", "red" )", {R"("name")", R"("red")"}, SplitOption::Comma);
	Parse(R"( "name", none "red" )", {R"("name")", R"(none "red")"}, SplitOption::Comma);

	Parse("\"string with spaces\"", "string with spaces");
	Parse("\"string with spaces\" two", {"string with spaces", "two"});
	Parse("\"string with spaces\"two", {"string with spaces", "two"});

	Parse("\"string\"two", {}, SplitOption::None);
	Parse("one\"string\"", {}, SplitOption::None);
	Parse("one \"string\"", {}, SplitOption::None);

	Parse("\"string (with) ((parenthesis\" two", {"string (with) ((parenthesis", "two"});
	Parse("\"none,,red\" two", {"none,,red", "two"});

	Parse("aa(bb( cc ) dd) ee", {"aa(bb( cc ) dd)", "ee"});
	Parse("aa(\"bb cc ) dd\") ee", {"aa(\"bb cc ) dd\")", "ee"});
	Parse("aa(\"bb cc \\) dd\") ee", {"aa(\"bb cc \\) dd\")", "ee"});
	Parse("aa(\"bb cc \\) dd\") ee", "aa(\"bb cc \\) dd\") ee", SplitOption::Comma);

	Parse("none(\"long string\"), aa, \"bb() cc\"", {"none(\"long string\"),", "aa,", "bb() cc"});
	Parse("none(\"long string\"), aa, \"bb() cc\"", {"none(\"long string\")", "aa", "\"bb() cc\""}, SplitOption::Comma);
	Parse("none(\"long string\"), aa, bb() cc", {"none(\"long string\")", "aa", "bb() cc"}, SplitOption::Comma);

	Parse("tiled-horizontal( title-bar-l, title-bar-c, title-bar-r )", "tiled-horizontal( title-bar-l, title-bar-c, title-bar-r )");
	Parse("tiled-horizontal( title-bar-l, title-bar-c,\n\ttitle-bar-r )", "tiled-horizontal( title-bar-l, title-bar-c,\n\ttitle-bar-r )");
	Parse("tiled-horizontal( title-bar-l, title-bar-c )", "tiled-horizontal( title-bar-l, title-bar-c )", SplitOption::Comma);

	Parse("linear-gradient(110deg, #fff, #000 10%) border-box, image(invader.png)",
		{"linear-gradient(110deg, #fff, #000 10%)", "border-box,", "image(invader.png)"});
	Parse("linear-gradient(110deg, rgba( 255, 255, 255, 255 ), #000 10%) border-box, image(invader.png)",
		{"linear-gradient(110deg, rgba( 255, 255, 255, 255 ), #000 10%)", "border-box,", "image(invader.png)"});
	Parse("linear-gradient(110deg, #fff, #000 10%) border-box, image(invader.png)",
		{"linear-gradient(110deg, #fff, #000 10%) border-box", "image(invader.png)"}, SplitOption::Comma);
	Parse("linear-gradient(110deg, rgba( 255, 255, 255, 255 ), #000 10%) border-box, image(invader.png)",
		{"linear-gradient(110deg, rgba( 255, 255, 255, 255 ), #000 10%) border-box", "image(invader.png)"}, SplitOption::Comma);

	Parse(R"(image( a\) b ))", {R"(image( a\))", "b", ")"});
	Parse(R"(image( a\) b ))", R"(image( a\) b ))", SplitOption::Comma);

	Parse(R"(image( ))", R"(image( ))");
	Parse(R"(image( a\\b ))", R"(image( a\\b ))");
	Parse(R"(image( a\\\b ))", R"(image( a\\\b ))");
	Parse(R"(image( a\\\\b ))", R"(image( a\\\\b ))");
	Parse(R"(image("a\)b"))", R"(image("a\)b"))");
	Parse(R"(image("a\\)b"))", R"(image("a\)b"))");
	Parse(R"(image("a\\b"))", R"(image("a\b"))");
	Parse(R"(image("a\\\b"))", R"(image("a\\b"))");
	Parse(R"(image("a\\\\b"))", R"(image("a\\b"))");

	Parse(R"()", {});
	Parse(R"("")", R"()");
	Parse(R"(" ")", R"( )");
	Parse(R"("abc")", R"(abc)");
	Parse(R"( "abc" )", R"(abc)");
	Parse(R"(" abc")", R"( abc)");
	Parse(R"("abc ")", R"(abc )");
	Parse(R"(" abc ")", R"( abc )");
	Parse(R"("test"  none)", {R"(test)", R"(none)"});

	Parse(R"('')", R"()");
	Parse(R"(' ')", R"( )");
	Parse(R"('abc')", R"(abc)");
	Parse(R"( 'abc' )", R"(abc)");
	Parse(R"(' abc')", R"( abc)");
	Parse(R"('abc ')", R"(abc )");
	Parse(R"(' abc ')", R"( abc )");
	Parse(R"('test'  none)", {R"(test)", R"(none)"});

	Parse(R"()", {}, SplitOption::None);
	Parse(R"("")", R"()", SplitOption::None);
	Parse(R"(" ")", R"( )", SplitOption::None);
	Parse(R"("abc")", R"(abc)", SplitOption::None);
	Parse(R"( "abc" )", R"(abc)", SplitOption::None);
	Parse(R"(" abc")", R"( abc)", SplitOption::None);
	Parse(R"("abc ")", R"(abc )", SplitOption::None);
	Parse(R"(" abc ")", R"( abc )", SplitOption::None);
	Parse(R"("test"  none)", {}, SplitOption::None);

	Parse(R"("","")", {R"("")", R"("")"}, SplitOption::Comma);
	Parse(R"(" "," ")", {R"(" ")", R"(" ")"}, SplitOption::Comma);
	Parse(R"(" " , " ")", {R"(" ")", R"(" ")"}, SplitOption::Comma);
	Parse(R"( " " none, yes)", {R"(" " none)", R"(yes)"}, SplitOption::Comma);

	Parse(R"('','')", {R"('')", R"('')"}, SplitOption::Comma);
	Parse(R"(' ',' ')", {R"(' ')", R"(' ')"}, SplitOption::Comma);
	Parse(R"(' ' , ' ')", {R"(' ')", R"(' ')"}, SplitOption::Comma);
	Parse(R"( ' ' none, yes)", {R"(' ' none)", R"(yes)"}, SplitOption::Comma);

	Rml::Shutdown();
}

TEST_CASE("PropertySpecification.string")
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

	Rml::Shutdown();
}

TEST_CASE("PropertyParser.Keyword")
{
	TestsSystemInterface system_interface;
	TestsRenderInterface render_interface;
	SetRenderInterface(&render_interface);
	SetSystemInterface(&system_interface);
	Rml::Initialise();

	// Test keyword parser. Ensure that the keyword values are correct.
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

TEST_CASE("PropertyParser.InvalidShorthands")
{
	TestsSystemInterface system_interface;
	TestsRenderInterface render_interface;
	SetRenderInterface(&render_interface);
	SetSystemInterface(&system_interface);
	Rml::Initialise();

	ElementPtr element = Factory::InstanceElement(nullptr, "*", "div", {});

	struct TestCase {
		bool expected_result;
		String name;
		String value;
	};
	Vector<TestCase> tests = {
		{true, "margin", "10px "},                                  //
		{true, "margin", "10px 20px "},                             //
		{true, "margin", "10px 20px 30px "},                        //
		{true, "margin", "10px 20px 30px 40px"},                    //
		{false, "margin", ""},                                      // Too few values
		{false, "margin", "10px 20px 30px 40px 50px"},              // Too many values

		{true, "flex", "1 2 3px"},                                  //
		{false, "flex", "1 2 3px 4"},                               // Too many values
		{false, "flex", "1px 2 3 4"},                               // Wrong order

		{true, "perspective-origin", "center center"},              //
		{true, "perspective-origin", "left top"},                   //
		{false, "perspective-origin", "center center center"},      // Too many values
		{false, "perspective-origin", "left top 50px"},             // Too many values
		{false, "perspective-origin", "50px 50px left"},            // Too many values
		{false, "perspective-origin", "top left"},                  // Wrong order
		{false, "perspective-origin", "50px left"},                 // Wrong order

		{true, "font", "20px arial"},                               //
		{false, "font", "arial 20px"},                              // Wrong order

		{true, "decorator", "gradient(vertical blue red)"},         //
		{false, "decorator", "gradient(blue red vertical)"},        // Wrong order
		{false, "decorator", "gradient(blue vertical red)"},        // Wrong order
		{false, "decorator", "gradient(vertical blue red green)"},  // Too many values

		{true, "filter", "drop-shadow(blue 10px 20px 30px)"},       //
		{false, "filter", "drop-shadow(10px 20px 30px blue)"},      // Wrong order
		{false, "filter", "drop-shadow(10px blue 20px 30px)"},      // Wrong order
		{false, "filter", "drop-shadow(blue 10px 20px 30px 40px)"}, // Too many values

		{true, "overflow", "hidden"},                               //
		{true, "overflow", "scroll hidden"},                        //
		{false, "overflow", ""},                                    // Too few values
		{false, "overflow", "scroll hidden scroll"},                // Too many values
	};

	for (const TestCase& test : tests)
	{
		PropertyDictionary properties;
		INFO(test.name, ": ", test.value);
		CHECK(StyleSheetSpecification::ParsePropertyDeclaration(properties, test.name, test.value) == test.expected_result);

		// Ensure we get a warning when trying to set the invalid property on an element.
		system_interface.SetNumExpectedWarnings(test.expected_result ? 0 : 1);
		CHECK(element->SetProperty(test.name, test.value) == test.expected_result);
	}

	element.reset();
	Rml::Shutdown();
}
