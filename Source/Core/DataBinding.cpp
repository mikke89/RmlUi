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
#include "../../Include/RmlUi/Core/DataBinding.h"
#include "../../Include/RmlUi/Core/Element.h"

namespace Rml {
namespace Core {


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
		entry.name = (String)StringUtilities::StripWhitespace(StringView(in_text.data() + begin_name, in_text.data() + end_name));
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
		bool result = model.GetValue(entry.name, value);

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
			Log::Message(Log::LT_WARNING, "Could not update data view text, parent element no longer valid. Was it destroyed?");
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


DataViewAttribute::DataViewAttribute(const DataModel& model, Element* element, const String& attribute_name, const String& value_name)
	: element(element->GetObserverPtr()), attribute_name(attribute_name), value_name(value_name)
{
	Update(model);
}

bool DataViewAttribute::Update(const DataModel& model)
{
	bool result = false;
	String value;
	if (model.GetValue(value_name, value))
	{
		Variant* variant = element->GetAttribute(attribute_name);

		if (!variant || (variant && *variant != value))
		{
			element->SetAttribute(attribute_name, value);
			result = true;
		}
	}
	return result;
}


bool DataModel::GetValue(const String& name, String& out_value) const
{
	auto it = bindings.find(name);
	if (it != bindings.end())
	{
		const Binding& binding = it->second;

		bool result = true;

		if (binding.type == Type::STRING)
			out_value = *static_cast<const String*>(binding.ptr);
		else if (binding.type == Type::INT)
			TypeConverter<int, String>::Convert(*static_cast<const int*>(binding.ptr), out_value);
		else
		{
			RMLUI_ERRORMSG("TODO: Implementation for the provided binding type has not been made yet.");
			result = false;
		}

		return result;
	}
	else
	{
		Log::Message(Log::LT_WARNING, "Could not find value named '%s' in data model.", name.c_str());
	}

	return false;
}

}
}
