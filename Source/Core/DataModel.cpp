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

bool DataModel::BindVariable(const String& name, Variable variable)
{
	if (!variable)
	{
		Log::Message(Log::LT_WARNING, "Could not bind variable '%s' to data model, data type not registered.", name.c_str());
		return false;
	}

	bool inserted = variables.emplace(name, variable).second;
	if (!inserted)
	{
		Log::Message(Log::LT_WARNING, "Data model variable with name '%s' already exists.", name.c_str());
		return false;
	}

	return true;
}

bool DataModel::InsertAlias(Element* element, const String& alias_name, Address replace_with_address)
{
	if (replace_with_address.empty() || replace_with_address.front().name.empty())
	{
		Log::Message(Log::LT_WARNING, "Could not add alias variable '%s' to data model, replacement address invalid.", alias_name.c_str());
		return false;
	}

	if (variables.count(alias_name) == 1)
		Log::Message(Log::LT_WARNING, "Alias variable '%s' is shadowed by a global variable.", alias_name.c_str());

	auto& map = aliases.emplace(element, SmallUnorderedMap<String, Address>()).first->second;
	
	auto it = map.find(alias_name);
	if (it != map.end())
		Log::Message(Log::LT_WARNING, "Alias name '%s' in data model already exists, replaced.", alias_name.c_str());

	map[alias_name] = std::move(replace_with_address);

	return true;
}

bool DataModel::EraseAliases(Element* element)
{
	return aliases.erase(element) == 1;
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

void DataModel::DirtyVariable(const String& variable_name)
{
	RMLUI_ASSERTMSG(variables.count(variable_name) == 1, "Variable name not found among added variables.");
	dirty_variables.insert(variable_name);
}

bool DataModel::IsVariableDirty(const String& variable_name) const {
	return (dirty_variables.count(variable_name) == 1);
}

void DataModel::OnElementRemove(Element* element)
{
	EraseAliases(element);
	views.OnElementRemove(element);
}

void DataModel::DirtyController(Element* element) 
{
	controllers.DirtyElement(*this, element);
}

bool DataModel::Update() 
{
	bool result = views.Update(*this, dirty_variables);
	dirty_variables.clear();
	return result;
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

		DataModel model;
		DataTypeRegister types;

		DataModelHandle handle(&model, &types);

		{
			handle.RegisterArray<IntVector>();

			if (auto fun_handle = handle.RegisterStruct<FunData>())
			{
				fun_handle.AddMember("i", &FunData::i);
				fun_handle.AddMember("x", &FunData::x);
				fun_handle.AddMember("magic", &FunData::magic);
			}

			handle.RegisterArray<FunArray>();

			if (auto smart_handle = handle.RegisterStruct<SmartData>())
			{
				smart_handle.AddMember("valid", &SmartData::valid);
				smart_handle.AddMember("fun", &SmartData::fun);
				smart_handle.AddMember("more_fun", &SmartData::more_fun);
			}
		}

		SmartData data;
		data.fun.x = "Hello, we're in SmartData!";
		
		handle.Bind("data", &data);

		{
			std::vector<String> test_addresses = { "data.more_fun[1].magic[3]", "data.fun.x", "data.valid" };
			std::vector<String> expected_results = { ToString(data.more_fun[1].magic[3]), ToString(data.fun.x), ToString(data.valid) };

			std::vector<String> results;

			for (auto& str_address : test_addresses)
			{
				Address address = ParseAddress(str_address);

				String result;
				if(model.GetValue<String>(address, result))
					results.push_back(result);
			}

			RMLUI_ASSERT(results == expected_results);

			bool success = model.GetVariable(ParseAddress("data.more_fun[1].magic[1]")).Set(Variant(String("199")));
			RMLUI_ASSERT(success && data.more_fun[1].magic[1] == 199);

			data.fun.magic = { 99, 190, 55, 2000, 50, 60, 70, 80, 90 };

			Variant test_get_result;
			bool test_get_success = model.GetVariable(ParseAddress("data.fun.magic[8]")).Get(test_get_result);
			RMLUI_ASSERT(test_get_success && test_get_result.Get<String>() == "90");
		}
	}
} test_data_variables;


#endif

}
}
