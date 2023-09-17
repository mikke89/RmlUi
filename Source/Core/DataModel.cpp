/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
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

#include "DataModel.h"
#include "../../Include/RmlUi/Core/DataTypeRegister.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "DataController.h"
#include "DataView.h"

namespace Rml {

static DataAddress ParseAddress(const String& address_str)
{
	StringList list;
	StringUtilities::ExpandString(list, address_str, '.');

	DataAddress address;
	address.reserve(list.size() * 2);

	for (const auto& item : list)
	{
		if (item.empty())
			return DataAddress();

		size_t i_open = item.find('[', 0);
		if (i_open == 0)
			return DataAddress();

		address.emplace_back(item.substr(0, i_open));

		while (i_open != String::npos)
		{
			size_t i_close = item.find(']', i_open + 1);
			if (i_close == String::npos)
				return DataAddress();

			int index = FromString<int>(item.substr(i_open + 1, i_close - i_open), -1);
			if (index < 0)
				return DataAddress();

			address.emplace_back(index);

			i_open = item.find('[', i_close + 1);
		}
		// TODO: Abort on invalid characters among [ ] and after the last found bracket?
	}

	RMLUI_ASSERT(!address.empty() && !address[0].name.empty());

	return address;
}

// Returns an error string on error, or nullptr on success.
static const char* LegalVariableName(const String& name)
{
	static SmallUnorderedSet<String> reserved_names{"it", "it_index", "ev", "true", "false", "size", "literal"};

	if (name.empty())
		return "Name cannot be empty.";

	const String name_lower = StringUtilities::ToLower(name);

	const char first = name_lower.front();
	if (!(first >= 'a' && first <= 'z'))
		return "First character must be 'a-z' or 'A-Z'.";

	for (const char c : name_lower)
	{
		if (!(c == '_' || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')))
			return "Name must strictly contain characters a-z, A-Z, 0-9 and under_score.";
	}

	if (reserved_names.count(name_lower) == 1)
		return "Name is reserved.";

	return nullptr;
}

static String DataAddressToString(const DataAddress& address)
{
	String result;
	bool is_first = true;
	for (auto& entry : address)
	{
		if (entry.index >= 0)
			result += '[' + ToString(entry.index) + ']';
		else
		{
			if (!is_first)
				result += ".";
			result += entry.name;
		}
		is_first = false;
	}
	return result;
}

DataModel::DataModel(DataTypeRegister* data_type_register) : data_type_register(data_type_register)
{
	views = MakeUnique<DataViews>();
	controllers = MakeUnique<DataControllers>();
}

DataModel::~DataModel()
{
	RMLUI_ASSERT(attached_elements.empty());
}

void DataModel::AddView(DataViewPtr view)
{
	views->Add(std::move(view));
}

void DataModel::AddController(DataControllerPtr controller)
{
	controllers->Add(std::move(controller));
}

bool DataModel::BindVariable(const String& name, DataVariable variable)
{
	const char* name_error_str = LegalVariableName(name);
	if (name_error_str)
	{
		Log::Message(Log::LT_WARNING, "Could not bind data variable '%s'. %s", name.c_str(), name_error_str);
		return false;
	}

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

bool DataModel::BindFunc(const String& name, DataGetFunc get_func, DataSetFunc set_func)
{
	auto result = function_variable_definitions.emplace(name, nullptr);
	auto& it = result.first;
	bool inserted = result.second;
	if (!inserted)
	{
		Log::Message(Log::LT_ERROR, "Data get/set function with name %s already exists in model", name.c_str());
		return false;
	}
	auto& func_definition_ptr = it->second;
	func_definition_ptr = MakeUnique<FuncDefinition>(std::move(get_func), std::move(set_func));

	return BindVariable(name, DataVariable(func_definition_ptr.get(), nullptr));
}

bool DataModel::BindEventCallback(const String& name, DataEventFunc event_func)
{
	const char* name_error_str = LegalVariableName(name);
	if (name_error_str)
	{
		Log::Message(Log::LT_WARNING, "Could not bind data event callback '%s'. %s", name.c_str(), name_error_str);
		return false;
	}

	if (!event_func)
	{
		Log::Message(Log::LT_WARNING, "Could not bind data event callback '%s' to data model, empty function provided.", name.c_str());
		return false;
	}

	bool inserted = event_callbacks.emplace(name, std::move(event_func)).second;
	if (!inserted)
	{
		Log::Message(Log::LT_WARNING, "Data event callback with name '%s' already exists.", name.c_str());
		return false;
	}

	return true;
}

bool DataModel::InsertAlias(Element* element, const String& alias_name, DataAddress replace_with_address)
{
	if (replace_with_address.empty() || replace_with_address.front().name.empty())
	{
		Log::Message(Log::LT_WARNING, "Could not add alias variable '%s' to data model, replacement address invalid.", alias_name.c_str());
		return false;
	}

	if (variables.count(alias_name) == 1)
		Log::Message(Log::LT_WARNING, "Alias variable '%s' is shadowed by a global variable.", alias_name.c_str());

	auto& map = aliases.emplace(element, SmallUnorderedMap<String, DataAddress>()).first->second;

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

void DataModel::CopyAliases(Element* from_element, Element* to_element)
{
	if (from_element == to_element)
		return;
	auto existing_map = aliases.find(from_element);

	if (existing_map != aliases.end())
	{
		// Need to create a copy to prevent errors during concurrent modification for 3rd party containers
		auto copy = existing_map->second;
		for (auto const& it : copy)
			aliases[to_element][it.first] = std::move(it.second);
	}
}

DataAddress DataModel::ResolveAddress(const String& address_str, Element* element) const
{
	DataAddress address = ParseAddress(address_str);

	if (address.empty())
		return address;

	const String& first_name = address.front().name;

	auto it = variables.find(first_name);
	if (it != variables.end())
		return address;

	// Look for a variable alias for the first name.

	Element* ancestor = element;
	while (ancestor && ancestor->GetDataModel() == this)
	{
		auto it_element = aliases.find(ancestor);
		if (it_element != aliases.end())
		{
			const auto& alias_names = it_element->second;
			auto it_alias_name = alias_names.find(first_name);
			if (it_alias_name != alias_names.end())
			{
				const DataAddress& replace_address = it_alias_name->second;
				if (replace_address.empty() || replace_address.front().name.empty())
				{
					// Variable alias is invalid
					return DataAddress();
				}

				// Insert the full alias address, replacing the first element.
				address[0] = replace_address[0];
				address.insert(address.begin() + 1, replace_address.begin() + 1, replace_address.end());
				return address;
			}
		}

		ancestor = ancestor->GetParentNode();
	}

	Log::Message(Log::LT_WARNING, "Could not find variable name '%s' in data model.", address_str.c_str());

	return DataAddress();
}

DataVariable DataModel::GetVariable(const DataAddress& address) const
{
	if (address.empty())
		return DataVariable();

	auto it = variables.find(address.front().name);
	if (it != variables.end())
	{
		DataVariable variable = it->second;

		for (int i = 1; i < (int)address.size() && variable; i++)
		{
			variable = variable.Child(address[i]);
			if (!variable)
				return DataVariable();
		}

		return variable;
	}

	if (address[0].name == "literal")
	{
		if (address.size() > 2 && address[1].name == "int")
			return MakeLiteralIntVariable(address[2].index);
	}

	return DataVariable();
}

const DataEventFunc* DataModel::GetEventCallback(const String& name)
{
	auto it = event_callbacks.find(name);
	if (it == event_callbacks.end())
	{
		Log::Message(Log::LT_WARNING, "Could not find data event callback '%s' in data model.", name.c_str());
		return nullptr;
	}

	return &it->second;
}

bool DataModel::GetVariableInto(const DataAddress& address, Variant& out_value) const
{
	DataVariable variable = GetVariable(address);
	bool result = (variable && variable.Get(out_value));
	if (!result)
		Log::Message(Log::LT_WARNING, "Could not get value from data variable '%s'.", DataAddressToString(address).c_str());
	return result;
}

void DataModel::DirtyVariable(const String& variable_name)
{
	RMLUI_ASSERTMSG(LegalVariableName(variable_name) == nullptr, "Illegal variable name provided. Only top-level variables can be dirtied.");
	RMLUI_ASSERTMSG(variables.count(variable_name) == 1, "In DirtyVariable: Variable name not found among added variables.");
	dirty_variables.emplace(variable_name);
}

bool DataModel::IsVariableDirty(const String& variable_name) const
{
	RMLUI_ASSERTMSG(LegalVariableName(variable_name) == nullptr, "Illegal variable name provided. Only top-level variables can be dirtied.");
	return dirty_variables.count(variable_name) == 1;
}

void DataModel::DirtyAllVariables()
{
	dirty_variables.reserve(variables.size());
	for (const auto& variable : variables)
	{
		dirty_variables.emplace(variable.first);
	}
}

bool DataModel::CallTransform(const String& name, const VariantList& arguments, Variant& out_result) const
{
	if (const auto transform_register = data_type_register->GetTransformFuncRegister())
		return transform_register->Call(name, arguments, out_result);
	return false;
}

void DataModel::AttachModelRootElement(Element* element)
{
	attached_elements.insert(element);
}

ElementList DataModel::GetAttachedModelRootElements() const
{
	return ElementList(attached_elements.begin(), attached_elements.end());
}

void DataModel::OnElementRemove(Element* element)
{
	EraseAliases(element);
	views->OnElementRemove(element);
	controllers->OnElementRemove(element);
	attached_elements.erase(element);
}

bool DataModel::Update(bool clear_dirty_variables)
{
	const bool result = views->Update(*this, dirty_variables);

	if (clear_dirty_variables)
		dirty_variables.clear();

	return result;
}

} // namespace Rml
