#include "../../../Source/Core/DataModel.cpp"
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
		IntVector magic = {3, 5, 7, 11, 13};
	};

	using FunArray = Array<FunData, 3>;

	struct SmartData {
		bool valid = true;
		FunData fun;
		FunArray more_fun;
	};

	DataTypeRegister types;
	DataModel model(&types);

	DataModelConstructor handle(&model);

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
		Vector<String> test_addresses = {"data.more_fun[1].magic[3]", "data.more_fun[1].magic.size", "data.fun.x", "data.valid"};
		Vector<String> expected_results = {ToString(data.more_fun[1].magic[3]), ToString(int(data.more_fun[1].magic.size())), ToString(data.fun.x),
			ToString(data.valid)};

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

		data.fun.magic = {99, 190, 55, 2000, 50, 60, 70, 80, 90};

		Variant get_result;

		const int magic_size = int(data.fun.magic.size());
		REQUIRE(model.GetVariable(ParseAddress("data.fun.magic.size")).Get(get_result));
		CHECK(get_result.Get<String>() == ToString(magic_size));
		CHECK(model.GetVariable(ParseAddress("data.fun.magic")).Size() == magic_size);

		REQUIRE(model.GetVariable(ParseAddress("data.fun.magic[8]")).Get(get_result));
		CHECK(get_result.Get<String>() == "90");
	}
}
