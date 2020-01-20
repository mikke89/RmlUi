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

Element* DataView::GetElement() const
{
	Element* result = attached_element.get();
	if (!result)
		Log::Message(Log::LT_WARNING, "Could not retrieve element in view, was it destroyed?");
	return result;
}

DataView::DataView(Element* element) : attached_element(element->GetObserverPtr()), element_depth(0) {
	if (element)
	{
		for (Element* parent = element->GetParentNode(); parent; parent = parent->GetParentNode())
			element_depth += 1;
	}
}


DataViewText::DataViewText(const DataModel& model, ElementText* in_parent_element, const String& in_text, const size_t index_begin_search) : DataView(in_parent_element)
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
		InvalidateView();
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
		if (Element* element = GetElement())
		{
			RMLUI_ASSERTMSG(rmlui_dynamic_cast<ElementText*>(element), "Somehow the element type was changed from ElementText since construction of the view. Should not be possible?");

			if(auto text_element = static_cast<ElementText*>(element))
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
	: DataView(element), attribute_name(attribute_name)
{
	variable_address = model.ResolveAddress(binding_name, parent);

	if (attribute_name.empty())
		InvalidateView();
	else
		Update(model);
}

bool DataViewAttribute::Update(const DataModel& model)
{
	bool result = false;
	String value;
	Element* element = GetElement();

	if (model.GetValue(variable_address, value) && element)
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
	: DataView(element), property_name(property_name)
{
	variable_address = model.ResolveAddress(binding_name, parent);
	
	if (variable_address.empty() || property_name.empty())
		InvalidateView();
	else
		Update(model);
}


bool DataViewStyle::Update(const DataModel& model)
{
	bool result = false;
	String value;
	Element* element = GetElement();

	if (model.GetValue(variable_address, value) && element)
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




DataViewIf::DataViewIf(const DataModel& model, Element* element, Element* parent, const String& binding_name) : DataView(element)
{
	variable_address = model.ResolveAddress(binding_name, element);
	if (variable_address.empty())
		InvalidateView();
	else
		Update(model);
}


bool DataViewIf::Update(const DataModel& model)
{
	bool result = false;
	bool value = false;
	Element* element = GetElement();

	if (model.GetValue(variable_address, value) && element)
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
	: DataView(element), rml_contents(in_rml_content)
{
	StringList binding_list;
	StringUtilities::ExpandString(binding_list, in_binding_name, ':');

	if (binding_list.empty() || binding_list.size() > 2 || binding_list.front().empty() || binding_list.back().empty())
	{
		Log::Message(Log::LT_WARNING, "Invalid syntax in data-for '%s'", in_binding_name.c_str());
		InvalidateView();
		return;
	}

	if (binding_list.size() == 2)
		alias_name = binding_list.front();
	else
		alias_name = "it";

	const String& binding_name = binding_list.back();

	variable_address = model.ResolveAddress(binding_name, element);
	if (variable_address.empty())
	{
		InvalidateView();
		return;
	}

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
	Element* element = GetElement();

	for (int i = 0; i < Math::Max(size, num_elements); i++)
	{
		if (i >= num_elements)
		{
			ElementPtr new_element_ptr = Factory::InstanceElement(nullptr, element->GetTagName(), element->GetTagName(), attributes);
			new_element_ptr->SetDataModel((DataModel*)(&model));
			Element* new_element = element->GetParentNode()->InsertBefore(std::move(new_element_ptr), element);
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
			model.EraseAliases(elements[i]);
			elements[i]->GetParentNode()->RemoveChild(elements[i]).reset();
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

void DataViews::Add(UniquePtr<DataView> view) {
	views_to_add.push_back(std::move(view));
}

void DataViews::OnElementRemove(Element* element) 
{
	for (auto it = views.begin(); it != views.end();)
	{
		auto& view = *it;
		if (view && view->GetElement() == element)
		{
			views_to_remove.push_back(std::move(view));
			it = views.erase(it);
		}
		else
			++it;
	}
}

bool DataViews::Update(const DataModel & model, const SmallUnorderedSet< String >& dirty_variables)
{
	bool result = false;

	std::vector<DataView*> dirty_views;

	if(!views_to_add.empty())
	{
		views.reserve(views.size() + views_to_add.size());
		for (auto&& view : views_to_add)
		{
			dirty_views.push_back(view.get());
			for(const String& variable_name : view->GetVariableNameList())
				name_view_map.emplace(variable_name, view.get());

			views.push_back(std::move(view));
		}
		views_to_add.clear();
	}

	for(const String& variable_name : dirty_variables)
	{
		auto pair = name_view_map.equal_range(variable_name);
		for (auto it = pair.first; it != pair.second; ++it)
			dirty_views.push_back(it->second);
	}

	// Remove duplicate entries
	std::sort(dirty_views.begin(), dirty_views.end());
	auto it_remove = std::unique(dirty_views.begin(), dirty_views.end());
	dirty_views.erase(it_remove, dirty_views.end());

	// Sort by the element's depth in the document tree so that any structural changes due to a changed variable are reflected in the element's children.
	// Eg. the 'data-for' view will remove children if any of its data variable array size is reduced.
	std::sort(dirty_views.begin(), dirty_views.end(), [](auto&& left, auto&& right) { return left->GetElementDepth() < right->GetElementDepth(); });

	
	// Todo: Newly created views won't actually be updated until next Update loop.
	for (DataView* view : dirty_views)
	{
		RMLUI_ASSERT(view);
		if (!view)
			continue;

		if (view->IsValid())
			result |= view->Update(model);
		else
			Log::Message(Log::LT_DEBUG, "Invalid view");
	}

	// Destroy views marked for destruction
	// @performance: Horrible...
	if (!views_to_remove.empty())
	{
		for (const auto& view : views_to_remove)
		{
			for (auto it = name_view_map.begin(); it != name_view_map.end(); )
			{
				if (it->second == view.get())
					it = name_view_map.erase(it);
				else
					++it;
			}
		}

		views_to_remove.clear();
	}


	return result;
}

}
}
