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
#include "RmlUi/Core/Core.h"
#include "RmlUi/Core/SystemInterface.h"
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Types.h>
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

struct YesStr {
	int scalar;
	int* scalarptr;
	const int* scalarptr2;
	Vector<int> yesvector;
};
struct FooStr {
	String scalar_string = "yes";
	int scalar;
	int* scalarptr;
	YesStr* yesptr;
	SharedPtr<YesStr> yesshr;
	const int* scalarptr2;
	Vector<int> foovector;
	Vector<int>* foovectorptr;
};
struct BarStr {
	int scalar;
	int* scalarptr;
	const int* scalarptr2;
	FooStr foo;
	FooStr* fooptr;
	SharedPtr<FooStr> fooshr;
	Vector<int> barvector;
	Vector<int>* barvectorptr;
};

void registerStructs(DataModelConstructor& handle)
{
	handle.RegisterArray<Vector<int>>();

	if (auto fun_handle = handle.RegisterStruct<YesStr>())
	{
		fun_handle.RegisterMember("scalar", &YesStr::scalar);
		fun_handle.RegisterMember("scalarptr", &YesStr::scalarptr);
		// requires constness support - fun_handle.RegisterMember("scalarptr2", &YesStr::scalarptr2);
		fun_handle.RegisterMember("yesvector", &YesStr::yesvector);
	}

	if (auto fun_handle = handle.RegisterStruct<FooStr>())
	{
		fun_handle.RegisterMember("scalar", &FooStr::scalar);
		fun_handle.RegisterMember("scalarptr", &FooStr::scalarptr);
		// requires constness support - fun_handle.RegisterMember("scalarptr2", &FooStr::scalarptr2);
		fun_handle.RegisterMember("string", &FooStr::scalar_string);
		fun_handle.RegisterMember("yesptr", &FooStr::yesptr);
		fun_handle.RegisterMember("yesshr", &FooStr::yesshr);
		fun_handle.RegisterMember("foovector", &FooStr::foovector);
		fun_handle.RegisterMember("foovectorptr", &FooStr::foovectorptr);
	}

	if (auto smart_handle = handle.RegisterStruct<BarStr>())
	{
		smart_handle.RegisterMember("scalar", &BarStr::scalar);
		smart_handle.RegisterMember("scalarptr", &BarStr::scalarptr);
		// requires constness support - smart_handle.RegisterMember("scalarptr2", &BarStr::scalarptr2);
		smart_handle.RegisterMember("foo", &BarStr::foo);
		smart_handle.RegisterMember("fooptr", &BarStr::fooptr);
		smart_handle.RegisterMember("fooshr", &BarStr::fooshr);
		smart_handle.RegisterMember("barvector", &BarStr::barvector);
		smart_handle.RegisterMember("barvectorptr", &BarStr::barvectorptr);
	}
}

TEST_CASE("Data variables pointers")
{
	DataModel model;
	DataTypeRegister types;

	DataModelConstructor handle(&model, &types);

	// Setup data type register
	registerStructs(handle);

	BarStr example;
	handle.Bind("example", &example);
	{
		int for_scalars_ptrsa = 13;
		int for_scalars_ptrsb = 69;
		example.scalar = 666;
		example.scalarptr = &for_scalars_ptrsa;
		example.scalarptr2 = &for_scalars_ptrsa;
		example.foo.scalar = 1337;
		example.foo.scalarptr = &for_scalars_ptrsb;
		example.foo.scalarptr2 = &for_scalars_ptrsb;
		example.fooptr = new FooStr();
		example.fooptr->scalar = 1337;
		example.fooptr->scalarptr = &for_scalars_ptrsb;
		example.fooptr->scalarptr2 = &for_scalars_ptrsb;
		example.fooshr = std::make_shared<FooStr>();
		*example.fooshr = example.foo;

		Vector<String> test_addresses = {"example.scalar", "example.scalarptr", /*"example.scalarptr2", */
			"example.foo.scalar", "example.foo.scalarptr", /*const "example.foo.scalarptr2", */ "example.foo.string", "example.fooptr.scalar",
			"example.fooptr.scalarptr", /*const "example.fooptr.scalarptr2" ,*/ "example.fooptr.string", "example.fooshr.scalar",
			"example.fooshr.scalarptr", /*const "example.fooshr.scalarptr2", */ "example.fooshr.string"};
		Vector<String> expected_results = {ToString(example.scalar), ToString(*example.scalarptr), /*const ToString(*example.scalarptr2), */
			ToString(example.foo.scalar), ToString(*example.foo.scalarptr),
			/*const ToString(*example.foo.scalarptr2) ,*/ ToString(example.foo.scalar_string), ToString(example.fooptr->scalar),
			ToString(*example.fooptr->scalarptr), /*const ToString(*example.fooptr->scalarptr2),*/ ToString(example.fooptr->scalar_string),
			ToString(example.fooshr->scalar), ToString(*example.fooshr->scalarptr),
			/*const ToString(*example.fooshr->scalarptr2) ,*/ ToString(example.fooshr->scalar_string)};

		Vector<String> results;

		for (auto& str_address : test_addresses)
		{
			DataAddress address = ParseAddress(str_address);

			Variant result;
			if (model.GetVariableInto(address, result))
				results.push_back(result.Get<String>());
		}
		CHECK(results == expected_results);
	}
	{
		delete example.fooptr;
		int new_int_value = 88;
		example.fooptr = new FooStr();
		example.fooptr->scalar = 2137;
		example.fooptr->scalarptr = &new_int_value;
		example.fooptr->scalarptr2 = &new_int_value;
		example.fooshr = std::make_shared<FooStr>();
		*example.fooshr = example.foo;

		Vector<String> test_addresses = {"example.fooptr.scalar", "example.fooptr.scalarptr",
			/*const "example.fooptr.scalarptr2" , */ "example.fooptr.string", "example.fooshr.scalar", "example.fooshr.scalarptr",
			/*const "example.fooshr.scalarptr2" , */ "example.fooshr.string"};
		Vector<String> expected_results = {ToString(example.fooptr->scalar), ToString(*example.fooptr->scalarptr),
			/*const ToString(*example.fooptr->scalarptr2),*/ ToString(example.fooptr->scalar_string), ToString(example.fooshr->scalar),
			ToString(*example.fooshr->scalarptr), /*const ToString(*example.fooshr->scalarptr2),*/ ToString(example.fooshr->scalar_string)};

		Vector<String> results;

		for (auto& str_address : test_addresses)
		{
			DataAddress address = ParseAddress(str_address);

			Variant result;
			if (model.GetVariableInto(address, result))
				results.push_back(result.Get<String>());
		}
		CHECK(results == expected_results);
	}
}

TEST_CASE("Data variables pointers - nulls")
{
	class SystemInterfaceImpl : public SystemInterface {
	public:
		double GetElapsedTime() override { return 0; }
	};
	SystemInterface* obj = new SystemInterfaceImpl();
	SetSystemInterface(obj);
	// Setup data type register

	DataModel model;
	DataTypeRegister types;

	DataModelConstructor handle(&model, &types);

	// Setup data type register
	registerStructs(handle);

	BarStr example;
	BarStr* exampleptr;
	UniquePtr<BarStr> exampleunq;
	UniquePtr<BarStr> exampleunq2;
	handle.Bind("example", &example);
	handle.Bind("exampleptr", &exampleptr);
	handle.Bind("exampleunq", &exampleunq);
	handle.Bind("exampleunq2", &exampleunq2);
	{
		example.fooptr = nullptr;
		example.fooshr = std::shared_ptr<FooStr>();
		example.barvectorptr = nullptr;
		exampleptr = nullptr;
		exampleunq = MakeUnique<BarStr>();
		exampleunq->fooptr = nullptr;
		exampleunq2 = MakeUnique<BarStr>();
		exampleunq2->fooptr = new FooStr();
		exampleunq2->fooptr->scalarptr = nullptr;
		exampleunq2->fooptr->yesptr = nullptr;

		Vector<String> test_addresses = {
			"example.fooptr.scalarptr",
			"example.fooptr",
			"example.fooshr",
			"example.fooshr.scalarptr",
			"example.fooptr.yesptr",
			"example.fooptr.yesptr.scalar",
			"example.fooptr.yesshr",
			"example.fooptr.yesshr.scalar",
			"example.barvectorptr",
			"example.barvectorptr[0]",
			"example.fooptr.foovectorptr",
			"example.fooptr.foovectorptr[0]",
			"exampleptr",
			"exampleptr.barvectorptr",
			"exampleptr.barvectorptr[0]",
			"exampleptr.fooptr",
			"exampleptr.fooptr.foovectorptr",
			"exampleptr.fooptr.foovectorptr[0]",
			/*"exampleunq", - this is not null*/ "exampleunq.fooptr",
			"exampleunq.fooptr.yesptr",
			/*"exampleunq2", - this is not null*/ /*"exampleunq2.fooptr", - this is not null*/ "exampleunq2.fooptr.scalarptr",
			"exampleunq2.fooptr.yesptr",

		};
		Vector<void*> expected_results = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
			nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

		Vector<void*> results;

		for (auto& str_address : test_addresses)
		{
			DataAddress address = ParseAddress(str_address);

			Variant result;
			model.GetVariableInto(address, result);
			results.push_back(result.Get<void*>());
		}
		CHECK(results == expected_results);
	}
}
