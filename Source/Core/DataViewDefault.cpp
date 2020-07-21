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

#include "DataViewDefault.h"
#include "DataExpression.h"
#include "../../Include/RmlUi/Core/DataModel.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/ElementText.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/Variant.h"

namespace Rml {

DataViewCommon::DataViewCommon(Element* element, String override_modifier) : DataView(element), modifier(override_modifier)
{}

bool DataViewCommon::Initialize(DataModel& model, Element* element, const String& expression_str, const String& in_modifier)
{
	// The modifier can be overriden in the constructor
	if (modifier.empty())
		modifier = in_modifier;

	expression = MakeUnique<DataExpression>(expression_str);
	DataExpressionInterface interface(&model, element);

	bool result = expression->Parse(interface, false);
	return result;
}

StringList DataViewCommon::GetVariableNameList() const {
	RMLUI_ASSERT(expression);
	return expression->GetVariableNameList();
}

const String& DataViewCommon::GetModifier() const {
	return modifier;
}

DataExpression& DataViewCommon::GetExpression() {
	RMLUI_ASSERT(expression);
	return *expression;
}

void DataViewCommon::Release()
{
	delete this;
}


DataViewAttribute::DataViewAttribute(Element* element) : DataViewCommon(element)
{}

DataViewAttribute::DataViewAttribute(Element * element, String override_attribute) : DataViewCommon(element, std::move(override_attribute))
{}

bool DataViewAttribute::Update(DataModel& model)
{
	const String& attribute_name = GetModifier();
	bool result = false;
	Variant variant;
	Element* element = GetElement();
	DataExpressionInterface interface(&model, element);

	if (element && GetExpression().Run(interface, variant))
	{
		const String value = variant.Get<String>();
		const Variant* attribute = element->GetAttribute(attribute_name);
		
		if (!attribute || (attribute && attribute->Get<String>() != value))
		{
			element->SetAttribute(attribute_name, value);
			result = true;
		}
	}
	return result;
}


DataViewValue::DataViewValue(Element* element) : DataViewAttribute(element, "value")
{}


DataViewStyle::DataViewStyle(Element* element) : DataViewCommon(element)
{}

bool DataViewStyle::Update(DataModel& model)
{
	const String& property_name = GetModifier();
	bool result = false;
	Variant variant;
	Element* element = GetElement();
	DataExpressionInterface interface(&model, element);
	
	if (element && GetExpression().Run(interface, variant))
	{
		const String value = variant.Get<String>();
		const Property* p = element->GetLocalProperty(property_name);
		if (!p || p->Get<String>() != value)
		{
			element->SetProperty(property_name, value);
			result = true;
		}
	}
	return result;
}


DataViewClass::DataViewClass(Element* element) : DataViewCommon(element)
{}

bool DataViewClass::Update(DataModel& model)
{
	const String& class_name = GetModifier();
	bool result = false;
	Variant variant;
	Element* element = GetElement();
	DataExpressionInterface interface(&model, element);

	if (element && GetExpression().Run(interface, variant))
	{
		const bool activate = variant.Get<bool>();
		const bool is_set = element->IsClassSet(class_name);
		if (activate != is_set)
		{
			element->SetClass(class_name, activate);
			result = true;
		}
	}
	return result;
}


DataViewRml::DataViewRml(Element* element) : DataViewCommon(element)
{}

bool DataViewRml::Update(DataModel & model)
{
	bool result = false;
	Variant variant;
	Element* element = GetElement();
	DataExpressionInterface interface(&model, element);

	if (element && GetExpression().Run(interface, variant))
	{
		String new_rml = variant.Get<String>();
		if (new_rml != previous_rml)
		{
			element->SetInnerRML(new_rml);
			previous_rml = std::move(new_rml);
			result = true;
		}
	}
	return result;
}


DataViewIf::DataViewIf(Element* element) : DataViewCommon(element)
{}

bool DataViewIf::Update(DataModel& model)
{
	bool result = false;
	Variant variant;
	Element* element = GetElement();
	DataExpressionInterface interface(&model, element);

	if (element && GetExpression().Run(interface, variant))
	{
		const bool value = variant.Get<bool>();
		const bool is_visible = (element->GetLocalStyleProperties().count(PropertyId::Display) == 0);
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


DataViewVisible::DataViewVisible(Element* element) : DataViewCommon(element)
{}

bool DataViewVisible::Update(DataModel& model)
{
	bool result = false;
	Variant variant;
	Element* element = GetElement();
	DataExpressionInterface interface(&model, element);

	if (element && GetExpression().Run(interface, variant))
	{
		const bool value = variant.Get<bool>();
		const bool is_visible = (element->GetLocalStyleProperties().count(PropertyId::Visibility) == 0);
		if (is_visible != value)
		{
			if (value)
				element->RemoveProperty(PropertyId::Visibility);
			else
				element->SetProperty(PropertyId::Visibility, Property(Style::Visibility::Hidden));
			result = true;
		}
	}
	return result;
}


DataViewText::DataViewText(Element* element) : DataView(element)
{}

bool DataViewText::Initialize(DataModel& model, Element* element, const String& RMLUI_UNUSED_PARAMETER(expression), const String& RMLUI_UNUSED_PARAMETER(modifier))
{
	RMLUI_UNUSED(expression);
	RMLUI_UNUSED(modifier);

	ElementText* element_text = rmlui_dynamic_cast<ElementText*>(element);
	if (!element_text)
		return false;

	const String& in_text = element_text->GetText();
	
	text.reserve(in_text.size());

	DataExpressionInterface expression_interface(&model, element);

	size_t previous_close_brackets = 0;
	size_t begin_brackets = 0;
	while ((begin_brackets = in_text.find("{{", begin_brackets)) != String::npos)
	{
		text.insert(text.end(), in_text.begin() + previous_close_brackets, in_text.begin() + begin_brackets);

		const size_t begin_name = begin_brackets + 2;
		const size_t end_name = in_text.find("}}", begin_name);

		if (end_name == String::npos)
			return false;

		DataEntry entry;
		entry.index = text.size();
		entry.data_expression = MakeUnique<DataExpression>(String(in_text.begin() + begin_name, in_text.begin() + end_name));

		if (entry.data_expression->Parse(expression_interface, false))
			data_entries.push_back(std::move(entry));

		previous_close_brackets = end_name + 2;
		begin_brackets = previous_close_brackets;
	}

	if (data_entries.empty())
		return false;

	if (previous_close_brackets < in_text.size())
		text.insert(text.end(), in_text.begin() + previous_close_brackets, in_text.end());

	return true;
}

bool DataViewText::Update(DataModel& model)
{
	bool entries_modified = false;
	{
		Element* element = GetElement();
		DataExpressionInterface expression_interface(&model, element);

		for (DataEntry& entry : data_entries)
		{
			RMLUI_ASSERT(entry.data_expression);
			Variant variant;
			bool result = entry.data_expression->Run(expression_interface, variant);
			const String value = variant.Get<String>();
			if (result && entry.value != value)
			{
				entry.value = value;
				entries_modified = true;
			}
		}
	}

	if (entries_modified)
	{
		if (Element* element = GetElement())
		{
			RMLUI_ASSERTMSG(rmlui_dynamic_cast<ElementText*>(element), "Somehow the element type was changed from ElementText since construction of the view. Should not be possible?");

			if (ElementText* text_element = static_cast<ElementText*>(element))
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

StringList DataViewText::GetVariableNameList() const
{
	StringList full_list;
	full_list.reserve(data_entries.size());

	for (const DataEntry& entry : data_entries)
	{
		RMLUI_ASSERT(entry.data_expression);

		StringList entry_list = entry.data_expression->GetVariableNameList();
		full_list.insert(full_list.end(),
			MakeMoveIterator(entry_list.begin()),
			MakeMoveIterator(entry_list.end())
		);
	}

	return full_list;
}

void DataViewText::Release()
{
	delete this;
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



DataViewFor::DataViewFor(Element* element) : DataView(element)
{}

bool DataViewFor::Initialize(DataModel& model, Element* element, const String& in_expression, const String& in_rml_content)
{
	rml_contents = in_rml_content;

	StringList iterator_container_pair;
	StringUtilities::ExpandString(iterator_container_pair, in_expression, ':');

	if (iterator_container_pair.empty() || iterator_container_pair.size() > 2 || iterator_container_pair.front().empty() || iterator_container_pair.back().empty())
	{
		Log::Message(Log::LT_WARNING, "Invalid syntax in data-for '%s'", in_expression.c_str());
		return false;
	}

	if (iterator_container_pair.size() == 2)
	{
		StringList iterator_index_pair;
		StringUtilities::ExpandString(iterator_index_pair, iterator_container_pair.front(), ',');

		if (iterator_index_pair.empty())
		{
			Log::Message(Log::LT_WARNING, "Invalid syntax in data-for '%s'", in_expression.c_str());
			return false;
		}
		else if (iterator_index_pair.size() == 1)
		{
			iterator_name = iterator_index_pair.front();
		}
		else if (iterator_index_pair.size() == 2)
		{
			iterator_name = iterator_index_pair.front();
			iterator_index_name = iterator_index_pair.back();
		}
	}

	if (iterator_name.empty())
		iterator_name = "it";

	if (iterator_index_name.empty())
		iterator_index_name = "it_index";

	const String& container_name = iterator_container_pair.back();

	container_address = model.ResolveAddress(container_name, element);
	if (container_address.empty())
		return false;

	element->SetProperty(PropertyId::Display, Property(Style::Display::None));

	return true;
}


bool DataViewFor::Update(DataModel& model)
{
	DataVariable variable = model.GetVariable(container_address);
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

			DataAddress iterator_address;
			iterator_address.reserve(container_address.size() + 1);
			iterator_address = container_address;
			iterator_address.push_back(DataAddressEntry(i));

			DataAddress iterator_index_address = {
				{"literal"}, {"int"}, {i}
			};

			model.InsertAlias(new_element_ptr.get(), iterator_name, std::move(iterator_address));
			model.InsertAlias(new_element_ptr.get(), iterator_index_name, std::move(iterator_index_address));

			Element* new_element = element->GetParentNode()->InsertBefore(std::move(new_element_ptr), element);
			elements.push_back(new_element);

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

StringList DataViewFor::GetVariableNameList() const {
	RMLUI_ASSERT(!container_address.empty());
	return StringList{ container_address.front().name };
}

void DataViewFor::Release()
{
	delete this;
}

} // namespace Rml
