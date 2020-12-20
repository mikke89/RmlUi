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

#include "../../../Source/Core/DataModel.cpp"
#include <RmlUi/Core/Types.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <doctest.h>

using namespace Rml;

TEST_CASE("Data variables")
{
	using IntVector = Vector<int>;

	struct FunData {
		int i = 99;
		String x = "hello";
		IntVector magic = { 3, 5, 7, 11, 13 };
	};

	using FunArray = Array<FunData, 3>;

	struct SmartData {
		bool valid = true;
		FunData fun;
		FunArray more_fun;
	};

	DataModel model;
	DataTypeRegister types;

	DataModelConstructor handle(&model, &types);

	// Setup data type register
	{
		handle.RegisterArray<IntVector>();

		if (auto fun_handle = handle.RegisterStruct<FunData>())
		{
			fun_handle.RegisterMember("i", &FunData::i);
			fun_handle.RegisterMember("x", &FunData::x);
			fun_handle.RegisterMember("magic", &FunData::magic);
		}

		handle.RegisterArray<FunArray>();

		if (auto smart_handle = handle.RegisterStruct<SmartData>())
		{
			smart_handle.RegisterMember("valid", &SmartData::valid);
			smart_handle.RegisterMember("fun", &SmartData::fun);
			smart_handle.RegisterMember("more_fun", &SmartData::more_fun);
		}
	}

	SmartData data;
	data.fun.x = "Hello, we're in SmartData!";

	handle.Bind("data", &data);

	// Test data addresses, setters, and assignments
	{
		Vector<String> test_addresses = { "data.more_fun[1].magic[3]", "data.more_fun[1].magic.size", "data.fun.x", "data.valid" };
		Vector<String> expected_results = { ToString(data.more_fun[1].magic[3]), ToString(int(data.more_fun[1].magic.size())), ToString(data.fun.x), ToString(data.valid) };

		Vector<String> results;

		for (auto& str_address : test_addresses)
		{
			DataAddress address = ParseAddress(str_address);

			Variant result;
			if (model.GetVariableInto(address, result))
				results.push_back(result.Get<String>());
		}

		CHECK(results == expected_results);

		REQUIRE(model.GetVariable(ParseAddress("data.more_fun[1].magic[1]")).Set(Variant(String("199"))));
		CHECK(data.more_fun[1].magic[1] == 199);

		data.fun.magic = { 99, 190, 55, 2000, 50, 60, 70, 80, 90 };

		Variant get_result;

		const int magic_size = int(data.fun.magic.size());
		REQUIRE(model.GetVariable(ParseAddress("data.fun.magic.size")).Get(get_result));
		CHECK(get_result.Get<String>() == ToString(magic_size));
		CHECK(model.GetVariable(ParseAddress("data.fun.magic")).Size() == magic_size);

		REQUIRE(model.GetVariable(ParseAddress("data.fun.magic[8]")).Get(get_result));
		CHECK(get_result.Get<String>() == "90");
	}
}
