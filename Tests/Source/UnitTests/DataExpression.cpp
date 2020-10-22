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
#include <RmlUi/Core/Types.h>

#include <doctest.h>

using namespace Rml;

static DataTypeRegister type_register;
static DataModel model(type_register.GetTransformFuncRegister());
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
			FAIL_CHECK("Could not execute expression: " << expression << "\n\n  Parsed program: \n" << interpreter.DumpProgram());
	}
	else
	{
		FAIL_CHECK("Could not parse expression: " << expression);
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
			FAIL_CHECK("Could not execute assignment expression: " << expression << "\n\n  Parsed program: \n" << interpreter.DumpProgram());
	}
	else
	{
		FAIL_CHECK("Could not parse assignment expression: " << expression);
	}
	return result;
}



TEST_CASE("Data expressions")
{
	float radius = 8.7f;
	String color_name = "color";
	Colourb color_value = Colourb(180, 100, 255);

	DataModelConstructor handle(&model, &type_register);
	handle.Bind("radius", &radius);
	handle.Bind("color_name", &color_name);
	handle.BindFunc("color_value", [&](Variant& variant) {
		variant = ToString(color_value);
	});

	CHECK(TestExpression("!!10 - 1 ? 'hello' : 'world' | to_upper") == "WORLD");
	CHECK(TestExpression("(color_name) + (': rgba(' + color_value + ')')") == "color: rgba(180, 100, 255, 255)");
	CHECK(TestExpression("'hello world' | to_upper(5 + 12 == 17 ? 'yes' : 'no', 9*2)") == "HELLO WORLD");
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

	CHECK(TestExpression("0.2 + 3.42345 | round") == "4");
	CHECK(TestExpression("(3.42345 | round) + 0.2") == "3.2");
	CHECK(TestExpression("(3.42345 | format(0)) + 0.2") == "30.2"); // Here, format(0) returns a string, so the + means string concatenation.
}


