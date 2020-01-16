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
#include "../../Include/RmlUi/Core/DataView.h"
#include "../../Include/RmlUi/Core/DataModel.h"


namespace Rml {
namespace Core {


DataView::~DataView() {}


DataViewText::DataViewText(const DataModel& model, ElementText* in_parent_element, const String& in_text, const size_t index_begin_search) : element(in_parent_element->GetObserverPtr())
{
	text.reserve(in_text.size());

	bool success = true;

	size_t previous_close_brackets = 0;
	size_t begin_brackets = index_begin_search;
	while ((begin_brackets = in_text.find("{{", begin_brackets)) != String::npos)
	{
		text.insert(text.end(), in_text.begin() + previous_close_brackets, in_text.begin() + begin_brackets);

		const size_t begin_name = begin_brackets + 2;
		const size_t end_name = in_text.find("}}", begin_name);

		if (end_name == String::npos)
		{
			success = false;
			break;
		}

		DataEntry entry;
		entry.index = text.size();
		String address_str = StringUtilities::StripWhitespace(StringView(in_text.data() + begin_name, in_text.data() + end_name));
		entry.variable_address = model.ResolveAddress(address_str, in_parent_element);

		data_entries.push_back(std::move(entry));

		previous_close_brackets = end_name + 2;
		begin_brackets = previous_close_brackets;
	}

	if (data_entries.empty())
		success = false;

	if (success && previous_close_brackets < in_text.size())
		text.insert(text.end(), in_text.begin() + previous_close_brackets, in_text.end());

	if (success)
	{
		Update(model);
	}
	else
	{
		text.clear();
		data_entries.clear();
	}
}

bool DataViewText::Update(const DataModel& model)
{
	bool entries_modified = false;

	for (DataEntry& entry : data_entries)
	{
		String value;
		bool result = model.GetValue(entry.variable_address, value);
		if (result && entry.value != value)
		{
			entry.value = value;
			entries_modified = true;
		}
	}

	if (entries_modified)
	{
		if (element)
		{
			RMLUI_ASSERTMSG(rmlui_dynamic_cast<ElementText*>(element.get()), "Somehow the element type was changed from ElementText since construction of the view. Should not be possible?");

			if(auto text_element = static_cast<ElementText*>(element.get()))
			{
				String new_text = BuildText();
				text_element->SetText(new_text);
			}
		}
		else
		{
			Log::Message(Log::LT_WARNING, "Could not update data view text, element no longer valid. Was it destroyed?");
		}
	}

	return entries_modified;
}

String DataViewText::BuildText() const
{
	size_t reserve_size = text.size();

	for (const DataEntry& entry : data_entries)
		reserve_size += entry.value.size();

	String result;
	result.reserve(reserve_size);

	size_t previous_index = 0;
	for (const DataEntry& entry : data_entries)
	{
		result += text.substr(previous_index, entry.index - previous_index);
		result += entry.value;
		previous_index = entry.index;
	}

	if (previous_index < text.size())
		result += text.substr(previous_index);

	return result;
}


DataViewAttribute::DataViewAttribute(const DataModel& model, Element* element, Element* parent, const String& binding_name, const String& attribute_name)
	: element(element->GetObserverPtr()), attribute_name(attribute_name)
{
	variable_address = model.ResolveAddress(binding_name, parent);
	Update(model);
}

bool DataViewAttribute::Update(const DataModel& model)
{
	bool result = false;
	String value;
	if (model.GetValue(variable_address, value))
	{
		Variant* attribute = element->GetAttribute(attribute_name);

		if (!attribute || (attribute && attribute->Get<String>() != value))
		{
			element->SetAttribute(attribute_name, value);
			result = true;
		}
	}
	return result;
}



DataViewStyle::DataViewStyle(const DataModel& model, Element* element, Element* parent, const String& binding_name, const String& property_name)
	: element(element->GetObserverPtr()), property_name(property_name)
{
	variable_address = model.ResolveAddress(binding_name, parent);
	Update(model);
}


bool DataViewStyle::Update(const DataModel& model)
{
	bool result = false;
	String value;
	if (model.GetValue(variable_address, value))
	{
		const Property* p = element->GetLocalProperty(property_name);
		if (!p || p->Get<String>() != value)
		{
			element->SetProperty(property_name, value);
			result = true;
		}
	}
	return result;
}




DataViewIf::DataViewIf(const DataModel& model, Element* element, Element* parent, const String& binding_name) : element(element->GetObserverPtr())
{
	variable_address = model.ResolveAddress(binding_name, element);
	Update(model);
}


bool DataViewIf::Update(const DataModel& model)
{
	bool result = false;
	bool value = false;
	if (model.GetValue(variable_address, value))
	{
		bool is_visible = (element->GetLocalStyleProperties().count(PropertyId::Display) == 0);
		if(is_visible != value)
		{
			if (value)
				element->RemoveProperty(PropertyId::Display);
			else
				element->SetProperty(PropertyId::Display, Property(Style::Display::None));
			result = true;
		}
	}
	return result;
}



DataViewFor::DataViewFor(const DataModel& model, Element* element, const String& in_binding_name, const String& in_rml_content)
	: element(element->GetObserverPtr()), rml_contents(in_rml_content)
{
	StringList binding_list;
	StringUtilities::ExpandString(binding_list, in_binding_name, ':');

	if (binding_list.empty() || binding_list.size() > 2 || binding_list.front().empty() || binding_list.back().empty())
	{
		Log::Message(Log::LT_WARNING, "Invalid syntax for data-for '%s'", in_binding_name.c_str());
		return;
	}

	if (binding_list.size() == 2)
		alias_name = binding_list.front();
	else
		alias_name = "it";

	const String& binding_name = binding_list.back();

	variable_address = model.ResolveAddress(binding_name, element);
	attributes = element->GetAttributes();
	attributes.erase("data-for");
	element->SetProperty(PropertyId::Display, Property(Style::Display::None));
	Update(model);
}



bool DataViewFor::Update(const DataModel& model)
{
	Variable variable = model.GetVariable(variable_address);
	if (!variable)
		return false;

	bool result = false;
	const int size = variable.Size();
	const int num_elements = (int)elements.size();

	for (int i = 0; i < Math::Max(size, num_elements); i++)
	{
		if (i >= num_elements)
		{
			ElementPtr new_element_ptr = Factory::InstanceElement(nullptr, element->GetTagName(), element->GetTagName(), attributes);
			new_element_ptr->SetDataModel((DataModel*)(&model));
			Element* new_element = element->GetParentNode()->InsertBefore(std::move(new_element_ptr), element.get());
			elements.push_back(new_element);

			Address replacement_address;
			replacement_address.reserve(variable_address.size() + 1);
			replacement_address = variable_address;
			replacement_address.push_back(AddressEntry(i));

			model.InsertAlias(new_element, alias_name, replacement_address);

			elements[i]->SetInnerRML(rml_contents);

			RMLUI_ASSERT(i < (int)elements.size());
		}
		if (i >= size)
		{
			elements[i]->GetParentNode()->RemoveChild(elements[i]).reset();
			model.EraseAliases(elements[i]);
			elements[i] = nullptr;
		}
	}

	if (num_elements > size)
		elements.resize(size);

	return result;
}

DataViews::DataViews()
{}

DataViews::~DataViews()
{}

}
}
