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

#include "../../../Include/RmlUi/Core/Elements/ElementFormControlDataSelect.h"
#include "../../../Include/RmlUi/Core/Elements/DataQuery.h"
#include "../../../Include/RmlUi/Core/Elements/DataSource.h"
#include "../../../Include/RmlUi/Core/Elements/DataFormatter.h"
#include "WidgetDropDown.h"

namespace Rml {

// Constructs a new ElementFormControlDataSelect.
ElementFormControlDataSelect::ElementFormControlDataSelect(const String& tag) : ElementFormControlSelect(tag)
{
	data_source = nullptr;
	initialised = false;
}

ElementFormControlDataSelect::~ElementFormControlDataSelect()
{
	if (data_source != nullptr) {
		data_source->DetachListener(this);
		data_source = nullptr;
	}
}

// Sets the data source the control's options are driven from.
void ElementFormControlDataSelect::SetDataSource(const String& _data_source)
{
	SetAttribute("source", _data_source);
}

// If a new data source has been set on the control, this will attach to it and build the initial
// options.
void ElementFormControlDataSelect::OnUpdate()
{
	if (!initialised)
	{
		initialised = true;

		if (ParseDataSource(data_source, data_table, GetAttribute< String >("source", "")))
		{
			data_source->AttachListener(this);
			BuildOptions();
		}
	}
}

// Checks for changes to the data source or formatting attributes.
void ElementFormControlDataSelect::OnAttributeChange(const ElementAttributes& changed_attributes)
{
	ElementFormControlSelect::OnAttributeChange(changed_attributes);

	if (changed_attributes.find("source") != changed_attributes.end())
	{
		if (data_source != nullptr) {
			data_source->DetachListener(this);
			data_source = nullptr;
		}

		initialised = false;
	}
	else if (changed_attributes.find("fields") != changed_attributes.end() ||
			 changed_attributes.find("valuefield") != changed_attributes.end() ||
			 changed_attributes.find("formatter") != changed_attributes.end())
	{
		BuildOptions();
	}
}

// Detaches from the data source and rebuilds the options.
void ElementFormControlDataSelect::OnDataSourceDestroy(DataSource* _data_source)
{
	if (data_source == _data_source)
	{
		data_source->DetachListener(this);
		data_source = nullptr;
		data_table = "";

		BuildOptions();
	}
}

// Rebuilds the available options from the data source.
void ElementFormControlDataSelect::OnRowAdd(DataSource* RMLUI_UNUSED_PARAMETER(data_source), const String& table, int RMLUI_UNUSED_PARAMETER(first_row_added), int RMLUI_UNUSED_PARAMETER(num_rows_added))
{
	RMLUI_UNUSED(data_source);
	RMLUI_UNUSED(first_row_added);
	RMLUI_UNUSED(num_rows_added);

	if (table == data_table)
		BuildOptions();
}

// Rebuilds the available options from the data source.
void ElementFormControlDataSelect::OnRowRemove(DataSource* RMLUI_UNUSED_PARAMETER(data_source), const String& table, int RMLUI_UNUSED_PARAMETER(first_row_removed), int RMLUI_UNUSED_PARAMETER(num_rows_removed))
{
	RMLUI_UNUSED(data_source);
	RMLUI_UNUSED(first_row_removed);
	RMLUI_UNUSED(num_rows_removed);
	
	if (table == data_table)
		BuildOptions();
}

// Rebuilds the available options from the data source.
void ElementFormControlDataSelect::OnRowChange(DataSource* RMLUI_UNUSED_PARAMETER(data_source), const String& table, int RMLUI_UNUSED_PARAMETER(first_row_changed), int RMLUI_UNUSED_PARAMETER(num_rows_changed))
{
	RMLUI_UNUSED(data_source);
	RMLUI_UNUSED(first_row_changed);
	RMLUI_UNUSED(num_rows_changed);
	
	if (table == data_table)
		BuildOptions();
}

// Rebuilds the available options from the data source.
void ElementFormControlDataSelect::OnRowChange(DataSource* RMLUI_UNUSED_PARAMETER(data_source), const String& table)
{
	RMLUI_UNUSED(data_source);

	if (table == data_table)
		BuildOptions();
}

// Builds the option list from the data source.
void ElementFormControlDataSelect::BuildOptions()
{
	widget->ClearOptions();

	if (data_source == nullptr)
		return;

	// Store the old selection value and index. These will be used to restore the selection to the
	// most appropriate option after the options have been rebuilt.
	String old_value = GetValue();
	int old_selection = GetSelection();

	String fields_attribute = GetAttribute<String>("fields", "");
	String valuefield_attribute = GetAttribute<String>("valuefield", "");
	String data_formatter_attribute = GetAttribute<String>("formatter", "");
	DataFormatter* data_formatter = nullptr;

	// Process the attributes.
	if (fields_attribute.empty())
	{
		Log::Message(Log::LT_ERROR, "DataQuery failed, no fields specified for %s.", GetTagName().c_str());
		return;
	}

	if (valuefield_attribute.empty())
	{
		valuefield_attribute = fields_attribute.substr(0, fields_attribute.find(','));
	}

	if (!data_formatter_attribute.empty())
	{
		data_formatter = DataFormatter::GetDataFormatter(data_formatter_attribute);
		if (!data_formatter)
			Log::Message(Log::LT_WARNING, "Unable to find data formatter named '%s', formatting skipped.", data_formatter_attribute.c_str());
	}

	// Build a list of attributes
	String fields(valuefield_attribute);
	fields += ",";
	fields += fields_attribute;

	DataQuery query(data_source, data_table, fields);
	while (query.NextRow())
	{
		StringList fields_list;
		String value = query.Get<String>(0, "");

		for (size_t i = 1; i < query.GetNumFields(); ++i)
			fields_list.push_back(query.Get< String>(i, ""));

		String formatted;
		if (fields_list.size() > 0)
			formatted = fields_list[0];

		if (data_formatter)
			data_formatter->FormatData(formatted, fields_list);

		// Add the data as an option.
		widget->AddOption(formatted, value, -1, false);
	}

	// If an option was selected before, attempt to restore the selection to it.
	if (old_selection > -1)
	{
		// Try to find a selection with the same value as the previous one.
		for (int i = 0; i < GetNumOptions(); ++i)
		{
			Element* option = GetOption(i);
			Variant* variant = option->GetAttribute("value");
			if (variant && variant->Get<String>() == old_value)
			{
				widget->SetSelection(option, true);
				return;
			}
		}

		// Failed to find an option with the same value. Attempt to at least set the same index.
		int new_selection = Math::Clamp(old_selection, 0, GetNumOptions() - 1);
		if (GetNumOptions() == 0)
			new_selection = -1;

		widget->SetSelection(GetOption(new_selection), true);
	}
}

} // namespace Rml
