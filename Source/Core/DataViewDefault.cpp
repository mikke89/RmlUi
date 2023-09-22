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

#include "DataViewDefault.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/DataVariable.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/ElementText.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/SystemInterface.h"
#include "../../Include/RmlUi/Core/Variant.h"
#include "DataExpression.h"
#include "DataModel.h"
#include "XMLParseTools.h"

namespace Rml {

// Some data views need to offset the update order for proper behavior.
//  'data-value' may need other attributes applied first, eg. min/max attributes.
static constexpr int SortOffset_DataValue = 100;
//  'data-checked' may need a value attribute already set.
static constexpr int SortOffset_DataChecked = 110;

DataViewCommon::DataViewCommon(Element* element, String override_modifier, int sort_offset) :
	DataView(element, sort_offset), modifier(std::move(override_modifier))
{}

bool DataViewCommon::Initialize(DataModel& model, Element* element, const String& expression_str, const String& in_modifier)
{
	// The modifier can be overriden in the constructor
	if (modifier.empty())
		modifier = in_modifier;

	expression = MakeUnique<DataExpression>(expression_str);
	DataExpressionInterface expr_interface(&model, element);

	bool result = expression->Parse(expr_interface, false);
	return result;
}

StringList DataViewCommon::GetVariableNameList() const
{
	RMLUI_ASSERT(expression);
	return expression->GetVariableNameList();
}

const String& DataViewCommon::GetModifier() const
{
	return modifier;
}

DataExpression& DataViewCommon::GetExpression()
{
	RMLUI_ASSERT(expression);
	return *expression;
}

void DataViewCommon::Release()
{
	delete this;
}

DataViewAttribute::DataViewAttribute(Element* element) : DataViewCommon(element) {}

DataViewAttribute::DataViewAttribute(Element* element, String override_attribute, int sort_offset) :
	DataViewCommon(element, std::move(override_attribute), sort_offset)
{}

bool DataViewAttribute::Update(DataModel& model)
{
	const String& attribute_name = GetModifier();
	bool result = false;
	Variant variant;
	Element* element = GetElement();
	DataExpressionInterface expr_interface(&model, element);

	if (element && GetExpression().Run(expr_interface, variant))
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

DataViewAttributeIf::DataViewAttributeIf(Element* element) : DataViewCommon(element) {}

bool DataViewAttributeIf::Update(DataModel& model)
{
	const String& attribute_name = GetModifier();
	bool result = false;
	Variant variant;
	Element* element = GetElement();
	DataExpressionInterface expr_interface(&model, element);

	if (element && GetExpression().Run(expr_interface, variant))
	{
		const bool value = variant.Get<bool>();
		const bool is_set = static_cast<bool>(element->GetAttribute(attribute_name));
		if (is_set != value)
		{
			if (value)
				element->SetAttribute(attribute_name, String());
			else
				element->RemoveAttribute(attribute_name);
			result = true;
		}
	}
	return result;
}

DataViewValue::DataViewValue(Element* element) : DataViewAttribute(element, "value", SortOffset_DataValue) {}

DataViewChecked::DataViewChecked(Element* element) : DataViewCommon(element, String(), SortOffset_DataChecked) {}

bool DataViewChecked::Update(DataModel& model)
{
	bool result = false;
	Variant variant;
	Element* element = GetElement();
	DataExpressionInterface expr_interface(&model, element);

	if (element && GetExpression().Run(expr_interface, variant))
	{
		bool new_checked_state = false;

		if (variant.GetType() == Variant::BOOL)
		{
			new_checked_state = variant.Get<bool>();
		}
		else
		{
			const String value = variant.Get<String>();
			new_checked_state = (!value.empty() && value == element->GetAttribute<String>("value", ""));
		}

		const bool current_checked_state = element->HasAttribute("checked");

		if (new_checked_state != current_checked_state)
		{
			result = true;
			if (new_checked_state)
				element->SetAttribute("checked", String());
			else
				element->RemoveAttribute("checked");
		}
	}

	return result;
}

DataViewStyle::DataViewStyle(Element* element) : DataViewCommon(element) {}

bool DataViewStyle::Update(DataModel& model)
{
	const String& property_name = GetModifier();
	bool result = false;
	Variant variant;
	Element* element = GetElement();
	DataExpressionInterface expr_interface(&model, element);

	if (element && GetExpression().Run(expr_interface, variant))
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

DataViewClass::DataViewClass(Element* element) : DataViewCommon(element) {}

bool DataViewClass::Update(DataModel& model)
{
	const String& class_name = GetModifier();
	bool result = false;
	Variant variant;
	Element* element = GetElement();
	DataExpressionInterface expr_interface(&model, element);

	if (element && GetExpression().Run(expr_interface, variant))
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

DataViewRml::DataViewRml(Element* element) : DataViewCommon(element) {}

bool DataViewRml::Update(DataModel& model)
{
	bool result = false;
	Variant variant;
	Element* element = GetElement();
	DataExpressionInterface expr_interface(&model, element);

	if (element && GetExpression().Run(expr_interface, variant))
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

DataViewIf::DataViewIf(Element* element) : DataViewCommon(element) {}

bool DataViewIf::Update(DataModel& model)
{
	bool result = false;
	Variant variant;
	Element* element = GetElement();
	DataExpressionInterface expr_interface(&model, element);

	if (element && GetExpression().Run(expr_interface, variant))
	{
		const bool value = variant.Get<bool>();
		const bool is_visible = (element->GetLocalStyleProperties().count(PropertyId::Display) == 0);
		if (is_visible != value)
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

DataViewVisible::DataViewVisible(Element* element) : DataViewCommon(element) {}

bool DataViewVisible::Update(DataModel& model)
{
	bool result = false;
	Variant variant;
	Element* element = GetElement();
	DataExpressionInterface expr_interface(&model, element);

	if (element && GetExpression().Run(expr_interface, variant))
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

DataViewText::DataViewText(Element* element) : DataView(element, 0) {}

bool DataViewText::Initialize(DataModel& model, Element* element, const String& /*expression*/, const String& /*modifier*/)
{
	ElementText* element_text = rmlui_dynamic_cast<ElementText*>(element);
	if (!element_text)
		return false;

	const String& in_text = element_text->GetText();

	text.reserve(in_text.size());

	DataExpressionInterface expression_interface(&model, element);

	size_t begin_brackets = 0;
	size_t cur = 0;
	char previous = 0;
	bool was_in_brackets = false;
	bool in_brackets = false;
	bool in_string = false;

	for (char c : in_text)
	{
		was_in_brackets = in_brackets;

		const char* error_str = XMLParseTools::ParseDataBrackets(in_brackets, in_string, c, previous);
		if (error_str)
		{
			Log::Message(Log::LT_WARNING, "Failed to parse data view text '%s'. %s", in_text.c_str(), error_str);
			return false;
		}

		if (!was_in_brackets && in_brackets)
		{
			begin_brackets = cur;
		}
		else if (was_in_brackets && !in_brackets)
		{
			DataEntry entry;
			entry.index = text.size();
			entry.data_expression = MakeUnique<DataExpression>(String(in_text.begin() + begin_brackets + 1, in_text.begin() + cur - 1));
			entry.value = "#rmlui#"; // A random value that the user string will not be initialized with.

			if (entry.data_expression->Parse(expression_interface, false))
				data_entries.push_back(std::move(entry));

			// Reset char so that it won't be appended to the output
			c = 0;
		}
		else if (!in_brackets && previous)
		{
			text.push_back(previous);
		}

		cur++;
		previous = c;
	}

	if (!in_brackets && previous)
	{
		text.push_back(previous);
	}

	if (data_entries.empty())
		return false;

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
			String new_text = BuildText();
			String text;
			if (SystemInterface* system_interface = GetSystemInterface())
				system_interface->TranslateString(text, new_text);

			rmlui_static_cast<ElementText*>(element)->SetText(text);
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
		full_list.insert(full_list.end(), MakeMoveIterator(entry_list.begin()), MakeMoveIterator(entry_list.end()));
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

DataViewFor::DataViewFor(Element* element) : DataView(element, 0) {}

bool DataViewFor::Initialize(DataModel& model, Element* element, const String& in_expression, const String& in_rml_content)
{
	rml_contents = in_rml_content;

	StringList iterator_container_pair;
	StringUtilities::ExpandString(iterator_container_pair, in_expression, ':');

	if (iterator_container_pair.empty() || iterator_container_pair.size() > 2 || iterator_container_pair.front().empty() ||
		iterator_container_pair.back().empty())
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

	// Copy over the attributes, but remove the 'data-for' which would otherwise recreate the data-for loop on all constructed children recursively.
	attributes = element->GetAttributes();
	for (auto it = attributes.begin(); it != attributes.end(); ++it)
	{
		if (it->first == "data-for")
		{
			attributes.erase(it);
			break;
		}
	}

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

			DataAddress iterator_index_address = {{"literal"}, {"int"}, {i}};

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

StringList DataViewFor::GetVariableNameList() const
{
	RMLUI_ASSERT(!container_address.empty());
	return StringList{container_address.front().name};
}

void DataViewFor::Release()
{
	delete this;
}

DataViewAlias::DataViewAlias(Element* element) : DataView(element, 0) {}

StringList DataViewAlias::GetVariableNameList() const
{
	return variables;
}

bool DataViewAlias::Update(DataModel&)
{
	return false;
}

bool DataViewAlias::Initialize(DataModel& model, Element* element, const String& expression, const String& modifier)
{
	auto address = model.ResolveAddress(expression, element);
	if (address.empty())
		return false;

	variables.push_back(modifier);
	model.InsertAlias(element, modifier, address);
	return true;
}

void DataViewAlias::Release()
{
	delete this;
}

} // namespace Rml
