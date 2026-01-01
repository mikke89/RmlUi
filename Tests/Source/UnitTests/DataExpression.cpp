#include "../../../Source/Core/DataExpression.cpp"
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Types.h>
#include <doctest.h>

using namespace Rml;

static DataTypeRegister type_register;
static DataModel model(&type_register);
static DataExpressionInterface interface(&model, nullptr);

static String TestExpression(const String& expression)
{
	String result;

	DataParser parser(expression, interface);

	if (parser.Parse(false))
	{
		Program program = parser.ReleaseProgram();
		AddressList addresses = parser.ReleaseAddresses();

		DataInterpreter interpreter(program, addresses, interface);

		if (interpreter.Run())
			result = interpreter.Result().Get<String>();
		else
			FAIL_CHECK("Could not execute expression: " << expression << "\n\n  Parsed program: \n" << DumpProgram(program));
	}
	else
	{
		Program program = parser.ReleaseProgram();
		FAIL_CHECK("Could not parse expression: " << expression << "\n\n  Parsed result: \n" << DumpProgram(program));
	}

	return result;
}

static bool TestAssignment(const String& expression)
{
	bool result = false;
	DataParser parser(expression, interface);
	if (parser.Parse(true))
	{
		Program program = parser.ReleaseProgram();
		AddressList addresses = parser.ReleaseAddresses();

		DataInterpreter interpreter(program, addresses, interface);
		if (interpreter.Run())
			result = true;
		else
			FAIL_CHECK("Could not execute assignment expression: " << expression << "\n\n  Parsed program: \n" << DumpProgram(program));
	}
	else
	{
		Program program = parser.ReleaseProgram();
		FAIL_CHECK("Could not parse assignment expression: " << expression << "\n\n  Parsed result: \n" << DumpProgram(program));
	}
	return result;
}

TEST_CASE("Data expressions")
{
	float radius = 8.7f;
	int num_trolls = 1;
	String color_name = "color";
	Colourb color_value = Colourb(180, 100, 255);
	std::vector<String> num_multi = {"left", "right"};

	DataModelConstructor constructor(&model);
	constructor.RegisterArray<std::vector<String>>();

	constructor.Bind("radius", &radius);
	constructor.Bind("color_name", &color_name);
	constructor.Bind("num_trolls", &num_trolls);
	constructor.Bind("num_multi", &num_multi);
	constructor.BindFunc("color_value", [&](Variant& variant) { variant = ToString(color_value); });

	constructor.RegisterTransformFunc("concatenate", [](const VariantList& arguments) -> Variant {
		StringList list;
		list.reserve(arguments.size());
		for (const Variant& argument : arguments)
			list.push_back(argument.Get<String>());
		String result;
		StringUtilities::JoinString(result, list);
		return Variant(std::move(result));
	});
	constructor.RegisterTransformFunc("number_suffix", [](const VariantList& arguments) -> Variant {
		if (arguments.size() != 3)
			return {};
		String suffix = (arguments[0].Get<double>() == 1.0 ? arguments[1] : arguments[2]).Get<String>();
		return Variant(arguments[0].Get<String>() + ' ' + suffix);
	});
	DataModelHandle handle = constructor.GetModelHandle();

	CHECK(TestExpression("3.62345 | round | format(2)") == "4.00");

	CHECK(TestExpression("'a' | to_upper") == "A");
	CHECK(TestExpression("!!10 - 1 ? 'hello' : 'world' | to_upper") == "WORLD");
	CHECK(TestExpression("(color_name) + (': ' + color_value)") == "color: #b464ff");
	CHECK(TestExpression("'hello world' | to_upper | concatenate(5 + 12 == 17 ? 'yes' : 'no', 9*2)") == "HELLO WORLD,yes,18");
	CHECK(TestExpression("true == false") == "0");
	CHECK(TestExpression("true != false") == "1");
	CHECK(TestExpression("true") == "1");

	CHECK(TestExpression("true || false ? true && 3==1+2 ? 'Absolutely!' : 'well..' : 'no'") == "Absolutely!");
	CHECK(TestExpression(R"('It\'s a fit')") == R"(It's a fit)");
	CHECK(TestExpression("2 * 2") == "4");
	CHECK(TestExpression("50000 / 1500") == "33.333");
	CHECK(TestExpression("5*1+2") == "7");
	CHECK(TestExpression("5*(1+2)") == "15");
	CHECK(TestExpression("2*(-2)/4") == "-1");
	CHECK(TestExpression("5.2 + 19 + 'px'") == "24.2px");

	CHECK(TestExpression("(radius | format(2)) + 'm'") == "8.70m");
	CHECK(TestExpression("radius < 10.5 ? 'smaller' : 'larger'") == "smaller");
	CHECK(TestAssignment("radius = 15"));
	CHECK(radius == doctest::Approx(15.f));
	CHECK(TestExpression("radius < 10.5 ? 'smaller' : 'larger'") == "larger");
	CHECK(TestAssignment("radius = 4; color_name = 'image-color'"));
	CHECK(radius == doctest::Approx(4.f));
	CHECK(color_name == "image-color");
	CHECK(TestExpression("radius == 4 && color_name == 'image-color'") == "1");

	CHECK(TestAssignment("color_name = 'a' | concatenate('b')"));
	CHECK(color_name == "a,b");
	CHECK(TestAssignment("color_name = concatenate('c','d')"));
	CHECK(color_name == "c,d");

	CHECK(TestExpression("5 == 1 + 2*2 || 8 == 1 + 4  ? 'yes' : 'no'") == "yes");
	CHECK(TestExpression("!!('fa' + 'lse')") == "0");
	CHECK(TestExpression("!!('tr' + 'ue')") == "1");
	CHECK(TestExpression("'fox' + 'dog' ? 'FoxyDog' : 'hot' + 'dog' | to_upper") == "HOTDOG");

	CHECK(TestExpression("3.62345 | round") == "4");
	CHECK(TestExpression("3.62345 | format(0)") == "4");
	CHECK(TestExpression("3.62345 | format(2)") == "3.62");
	CHECK(TestExpression("3.62345 | format(10)") == "3.6234500000");
	CHECK(TestExpression("3.62345 | format(10, true)") == "3.62345");
	CHECK(TestExpression("3.62345 | round | format(2)") == "4.00");
	CHECK(TestExpression("3.0001 | format(2, false)") == "3.00");
	CHECK(TestExpression("3.0001 | format(2, true)") == "3");
	CHECK(TestExpression("format(3.0001, 2, true)") == "3");

	CHECK(TestExpression("0.2 + 3.42345 | round") == "4");
	CHECK(TestExpression("(3.42345 | round) + 0.2") == "3.2");
	CHECK(TestExpression("(3.42345 | format(0)) + 0.2") == "30.2"); // Here, format(0) returns a string, so the + means string concatenation.

	CHECK(TestExpression("'Hi' | concatenate") == "Hi");
	CHECK(TestExpression("'Hi' | concatenate('there')") == "Hi,there");
	CHECK(TestExpression("'A' | concatenate('b','c', 'd','e')") == "A,b,c,d,e");
	CHECK(TestExpression("3.6 | concatenate") == "3.6");

	CHECK(TestExpression("concatenate()") == "");
	CHECK(TestExpression("concatenate('Hi')") == "Hi");
	CHECK(TestExpression("concatenate( 'Hi',  'there' )") == "Hi,there");
	CHECK(TestExpression("concatenate('A', 'b'+'c', 'd','e')") == "A,bc,d,e");
	CHECK(TestExpression("concatenate('r', 2*radius)") == "r,8");
	CHECK(TestExpression("concatenate(3.6)") == "3.6");
	CHECK(TestExpression("concatenate(3.6+1)") == "4.6");
	CHECK(TestExpression("concatenate(3.6+1) | round | format(3, false)") == "5.000");
	CHECK(TestExpression("concatenate(3.6+1 | round, 'x')") == "5,x");

	CHECK(TestExpression("num_trolls | number_suffix('troll','trolls')") == "1 troll");
	CHECK(TestExpression("concatenate('It takes', num_trolls*3 + ' goats', 'to outsmart', num_trolls | number_suffix('troll','trolls'))") ==
		"It takes,3 goats,to outsmart,1 troll");
	num_trolls = 3;
	handle.DirtyVariable("num_trolls");
	CHECK(TestExpression("concatenate('It takes', num_trolls*3 + ' goats', 'to outsmart', num_trolls | number_suffix('troll','trolls'))") ==
		"It takes,9 goats,to outsmart,3 trolls");

	// Test that only one side of ternary is evaluated
	CHECK(TestExpression("true ? num_multi[0] : num_multi[999]") == "left");
	CHECK(TestExpression("false ? num_multi[999] : num_multi[1]") == "right");
}
