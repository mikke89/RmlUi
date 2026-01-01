#include "../../../Source/Core/DataExpression.cpp"
#include <RmlUi/Core/DataModelHandle.h>
#include <doctest.h>
#include <nanobench.h>

using namespace Rml;
using namespace ankerl;

static DataTypeRegister type_register;
static DataModel model(&type_register);
static DataExpressionInterface interface(&model, nullptr);

TEST_CASE("data_expressions")
{
	float radius = 6.0f;
	String color_name = "color";
	Colourb color_value = Colourb(180, 100, 255);

	DataModelConstructor constructor(&model);
	constructor.Bind("radius", &radius);
	constructor.Bind("color_name", &color_name);
	constructor.BindFunc("color_value", [&](Variant& variant) { variant = ToString(color_value); });

	nanobench::Bench bench;
	bench.title("Data expression");
	bench.relative(true);

	auto bench_expression = [&](const String& expression, const char* parse_name, const char* execute_name) {
		DataParser parser(expression, interface);

		bool result = true;
		bench.run(parse_name, [&] { result &= parser.Parse(false); });

		REQUIRE(result);

		Program program = parser.ReleaseProgram();
		AddressList addresses = parser.ReleaseAddresses();
		DataInterpreter interpreter(program, addresses, interface);

		bench.run(execute_name, [&] { result &= interpreter.Run(); });

		REQUIRE(result);
	};

	bench_expression("2 * 2", "Simple (parse)", "Simple (execute)");

	bench_expression("true || false ? true && radius==1+2 ? 'Absolutely!' : color_value : 'no'", "Complex (parse)", "Complex (execute)");

	auto bench_assignment = [&](const String& expression, const char* parse_name, const char* execute_name) {
		DataParser parser(expression, interface);

		bool result = true;
		bench.run(parse_name, [&] { result &= parser.Parse(true); });

		REQUIRE(result);

		Program program = parser.ReleaseProgram();
		AddressList addresses = parser.ReleaseAddresses();
		DataInterpreter interpreter(program, addresses, interface);

		bench.run(execute_name, [&] { result &= interpreter.Run(); });

		REQUIRE(result);
	};

	bench_assignment("radius = 15", "Simple assign (parse)", "Simple assign (execute)");

	bench_assignment("radius = radius*radius*3.14; color_name = 'image-color'", "Complex assign (parse)", "Complex assign (execute)");
}
