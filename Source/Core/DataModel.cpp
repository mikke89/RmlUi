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

#include "precompiled.h"
#include "../../Include/RmlUi/Core/DataModel.h"

namespace Rml {
namespace Core {


static Address ParseAddress(const String& address_str)
{
	StringList list;
	StringUtilities::ExpandString(list, address_str, '.');

	Address address;
	address.reserve(list.size() * 2);

	for (const auto& item : list)
	{
		if (item.empty())
			return Address();

		size_t i_open = item.find('[', 0);
		if (i_open == 0)
			return Address();

		address.emplace_back(item.substr(0, i_open));

		while (i_open != String::npos)
		{
			size_t i_close = item.find(']', i_open + 1);
			if (i_close == String::npos)
				return Address();

			int index = FromString<int>(item.substr(i_open + 1, i_close - i_open), -1);
			if (index < 0)
				return Address();

			address.emplace_back(index);

			i_open = item.find('[', i_close + 1);
		}
		// TODO: Abort on invalid characters among [ ] and after the last found bracket?
	}

	return address;
};


Variant DataModel::GetValue(const Rml::Core::String& address_str) const
{
	Variable variable = GetVariable(address_str);

	Variant result;
	if (!variable)
		return result;

	if (variable.Type() != VariableType::Scalar)
	{
		Log::Message(Log::LT_WARNING, "Error retrieving data variable '%s': Only the values of scalar variables can be parsed.", address_str.c_str());
		return result;
	}
	if (!variable.Get(result))
		Log::Message(Log::LT_WARNING, "Could not parse data value '%s'", address_str.c_str());

	return result;
}


bool DataModel::SetValue(const String& address_str, const Variant& variant) const
{
	Variable variable = GetVariable(address_str);

	if (!variable)
		return false;

	if (variable.Type() != VariableType::Scalar)
	{
		Log::Message(Log::LT_WARNING, "Could not assign data value '%s', variable is not a scalar type.", address_str.c_str());
		return false;
	}

	if (!variable.Set(variant))
	{
		Log::Message(Log::LT_WARNING, "Could not assign data value '%s'", address_str.c_str());
		return false;
	}

	return true;
}

bool DataModel::Bind(String name, void* ptr, VariableDefinition* variable, VariableType type)
{
	RMLUI_ASSERT(ptr);
	if (!variable)
	{
		Log::Message(Log::LT_WARNING, "No registered type could be found for the data variable '%s'.", name.c_str());
		return false;
	}

	if (variable->Type() != type)
	{
		Log::Message(Log::LT_WARNING, "The registered type does not match the given type for the data variable '%s'.", name.c_str());
		return false;
	}

	bool inserted = variables.emplace(name, Variable(variable, ptr)).second;
	if (!inserted)
	{
		Log::Message(Log::LT_WARNING, "Data model variable with name '%s' already exists.", name.c_str());
		return false;
	}

	return true;
}

Variable DataModel::GetVariable(const String& address_str) const
{
	Address address = ParseAddress(address_str);

	if (address.empty() || address.front().name.empty())
	{
		Log::Message(Log::LT_WARNING, "Invalid data address '%s'.", address_str.c_str());
		return Variable();
	}

	Variable instance = GetVariable(address);
	if (!instance)
	{
		Log::Message(Log::LT_WARNING, "Could not find the data variable '%s'.", address_str.c_str());
		return Variable();
	}

	return instance;
}

Variable DataModel::GetVariable(const Address& address) const
{
	if (address.empty() || address.front().name.empty())
		return Variable();

	auto it = variables.find(address.front().name);
	if (it == variables.end())
		return Variable();

	Variable variable = it->second;

	for (int i = 1; i < (int)address.size() && variable; i++)
	{
		variable = variable.GetChild(address[i]);
		if (!variable)
			return Variable();
	}

	return variable;
}

Address DataModel::ResolveAddress(const String& address_str, Element* parent) const
{
	Address address = ParseAddress(address_str);

	if (address.empty() || address.front().name.empty())
		return Address();

	const String& first_name = address.front().name;

	auto it = variables.find(first_name);
	if (it != variables.end())
		return address;

	// Look for a variable alias for the first name.
	
	Element* ancestor = parent;
	while (ancestor && ancestor->GetDataModel() == this)
	{
		auto it_element = aliases.find(ancestor);
		if (it_element != aliases.end())
		{
			auto& alias_names = it_element->second;
			auto it_alias_name = alias_names.find(first_name);
			if (it_alias_name != alias_names.end())
			{
				const Address& replace_address = it_alias_name->second;
				if (replace_address.empty() || replace_address.front().name.empty())
				{
					// Variable alias is invalid
					return Address();
				}

				// Insert the full alias address, replacing the first element.
				address[0] = std::move(replace_address[0]);
				address.insert(address.begin() + 1, replace_address.begin() + 1, replace_address.end());
				return address;
			}
		}

		ancestor = ancestor->GetParentNode();
	}

	Log::Message(Log::LT_WARNING, "Could not find variable name '%s' in data model.", address_str.c_str());

	return Address();
}

bool DataModel::InsertAlias(Element* element, const String& alias_name, Address replace_with_address) const
{
	if (replace_with_address.empty() || replace_with_address.front().name.empty())
	{
		Log::Message(Log::LT_WARNING, "Could not add alias variable '%s' to data model, replacement address invalid.", alias_name.c_str());
		return false;
	}

	auto& map = aliases.emplace(element, SmallUnorderedMap<String, Address>()).first->second;
	
	auto it = map.find(alias_name);
	if(it != map.end())
		Log::Message(Log::LT_WARNING, "Alias name '%s' in data model already exists, replaced.", alias_name.c_str());

	map[alias_name] = std::move(replace_with_address);

	return true;
}

bool DataModel::EraseAliases(Element* element) const
{
	return aliases.erase(element) == 1;
}



#ifdef RMLUI_DEBUG

static struct TestDataVariables {
	TestDataVariables() 
	{
		using IntVector = std::vector<int>;

		struct FunData {
			int i = 99;
			String x = "hello";
			IntVector magic = { 3, 5, 7, 11, 13 };
		};

		using FunArray = std::array<FunData, 3>;

		struct SmartData {
			bool valid = true;
			FunData fun;
			FunArray more_fun;
		};

		DataTypeRegister types;

		{
			auto int_vector_handle = types.RegisterArray<IntVector>();

			auto fun_handle = types.RegisterStruct<FunData>();
			if (fun_handle)
			{
				fun_handle.RegisterMember("i", &FunData::i);
				fun_handle.RegisterMember("x", &FunData::x);
				fun_handle.RegisterMember("magic", &FunData::magic, int_vector_handle);
			}

			auto fun_array_handle = types.RegisterArray<FunArray>(fun_handle);

			auto smart_handle = types.RegisterStruct<SmartData>();
			if (smart_handle)
			{
				smart_handle.RegisterMember("valid", &SmartData::valid);
				smart_handle.RegisterMember("fun", &SmartData::fun, fun_handle);
				smart_handle.RegisterMember("more_fun", &SmartData::more_fun, fun_array_handle);
			}
		}

		DataModel model(&types);

		SmartData data;
		data.fun.x = "Hello, we're in SmartData!";

		model.BindStruct("data", &data);

		{
			std::vector<String> test_addresses = { "data.more_fun[1].magic[3]", "data.fun.x", "data.valid" };
			std::vector<String> expected_results = { ToString(data.more_fun[1].magic[3]), ToString(data.fun.x), ToString(data.valid) };

			std::vector<String> results;

			for (auto& address : test_addresses)
			{
				auto the_address = ParseAddress(address);

				Variant variant = model.GetValue(address);
				results.push_back(variant.Get<String>());
			}

			RMLUI_ASSERT(results == expected_results);

			bool success = model.SetValue("data.more_fun[1].magic[1]", Variant(String("199")));
			RMLUI_ASSERT(success && data.more_fun[1].magic[1] == 199);

			data.fun.magic = { 99, 190, 55, 2000, 50, 60, 70, 80, 90 };

			String result = model.GetValue("data.fun.magic[8]").Get<String>();
			RMLUI_ASSERT(result == "90");
		}
	}
} test_data_variables;


#endif

}
}
