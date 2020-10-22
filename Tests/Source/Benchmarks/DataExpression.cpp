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


#include "../../../Source/Core/DataExpression.cpp"

#include <RmlUi/Core/DataModelHandle.h>
#include <doctest.h>
#include <nanobench.h>

using namespace Rml;
using namespace ankerl;

static DataTypeRegister type_register;
static DataModel model(type_register.GetTransformFuncRegister());
static DataExpressionInterface interface(&model, nullptr);


TEST_CASE("data_expressions")
{
	float radius = 6.0f;
	String color_name = "color";
	Colourb color_value = Colourb(180, 100, 255);

	DataModelConstructor constructor(&model, &type_register);
	constructor.Bind("radius", &radius);
	constructor.Bind("color_name", &color_name);
	constructor.BindFunc("color_value", [&](Variant& variant) {
		variant = ToString(color_value);
	});

	nanobench::Bench bench;
	bench.title("Data expression");
	bench.relative(true);

	auto bench_expression = [&](const String& expression, const char* parse_name, const char* execute_name) {
		DataParser parser(expression, interface);

		bool result = true;
		bench.run(parse_name, [&] {
			result &= parser.Parse(false);
			});

		REQUIRE(result);

		Program program = parser.ReleaseProgram();
		AddressList addresses = parser.ReleaseAddresses();
		DataInterpreter interpreter(program, addresses, interface);

		bench.run(execute_name, [&] {
			result &= interpreter.Run();
		});

		REQUIRE(result);
	};

	bench_expression(
		"2 * 2",
		"Simple (parse)",
		"Simple (execute)"
	);

	bench_expression(
		"true || false ? true && radius==1+2 ? 'Absolutely!' : color_value : 'no'",
		"Complex (parse)",
		"Complex (execute)"
	);

	auto bench_assignment = [&](const String& expression, const char* parse_name, const char* execute_name) {
		DataParser parser(expression, interface); 
		
		bool result = true;
		bench.run(parse_name, [&] {
			result &= parser.Parse(true);
			});

		REQUIRE(result);

		Program program = parser.ReleaseProgram();
		AddressList addresses = parser.ReleaseAddresses();
		DataInterpreter interpreter(program, addresses, interface);

		bench.run(execute_name, [&] {
			result &= interpreter.Run();
		});

		REQUIRE(result);
	};

	bench_assignment(
		"radius = 15",
		"Simple assign (parse)",
		"Simple assign (execute)"
	);

	bench_assignment(
		"radius = radius*radius*3.14; color_name = 'image-color'",
		"Complex assign (parse)",
		"Complex assign (execute)"
	);
}
