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

#include "../../../Include/RmlUi/Core/Elements/DataSourceListener.h"
#include "../../../Include/RmlUi/Core/Elements/DataSource.h"
#include "../../../Include/RmlUi/Core/StringUtilities.h"
#include "../../../Include/RmlUi/Core/Log.h"

namespace Rml {

DataSourceListener::DataSourceListener()
{
}

DataSourceListener::~DataSourceListener()
{
}

// Notification of the destruction of an observed data source.
void DataSourceListener::OnDataSourceDestroy(DataSource* RMLUI_UNUSED_PARAMETER(data_source))
{
	RMLUI_UNUSED(data_source);
}

// Notification of the addition of one or more rows to an observed data source's table.
void DataSourceListener::OnRowAdd(DataSource* RMLUI_UNUSED_PARAMETER(data_source), const String& RMLUI_UNUSED_PARAMETER(table), int RMLUI_UNUSED_PARAMETER(first_row_added), int RMLUI_UNUSED_PARAMETER(num_rows_added))
{
	RMLUI_UNUSED(data_source);
	RMLUI_UNUSED(table);
	RMLUI_UNUSED(first_row_added);
	RMLUI_UNUSED(num_rows_added);
}

// Notification of the removal of one or more rows from an observed data source's table.
void DataSourceListener::OnRowRemove(DataSource* RMLUI_UNUSED_PARAMETER(data_source), const String& RMLUI_UNUSED_PARAMETER(table), int RMLUI_UNUSED_PARAMETER(first_row_removed), int RMLUI_UNUSED_PARAMETER(num_rows_removed))
{
	RMLUI_UNUSED(data_source);
	RMLUI_UNUSED(table);
	RMLUI_UNUSED(first_row_removed);
	RMLUI_UNUSED(num_rows_removed);
}

// Notification of the changing of one or more rows from an observed data source's table.
void DataSourceListener::OnRowChange(DataSource* RMLUI_UNUSED_PARAMETER(data_source), const String& RMLUI_UNUSED_PARAMETER(table), int RMLUI_UNUSED_PARAMETER(first_row_changed), int RMLUI_UNUSED_PARAMETER(num_rows_changed))
{
	RMLUI_UNUSED(data_source);
	RMLUI_UNUSED(table);
	RMLUI_UNUSED(first_row_changed);
	RMLUI_UNUSED(num_rows_changed);
}

// Notification of the change of all of the data of an observed data source's table.
void DataSourceListener::OnRowChange(DataSource* RMLUI_UNUSED_PARAMETER(data_source), const String& RMLUI_UNUSED_PARAMETER(table))
{
	RMLUI_UNUSED(data_source);
	RMLUI_UNUSED(table);
}

// Sets up data source and table from a given string.
bool DataSourceListener::ParseDataSource(DataSource*& data_source, String& table_name, const String& data_source_name)
{
	if (data_source_name.size() == 0)
	{
		data_source = nullptr;
		table_name = "";
		return false;
	}

	StringList data_source_parts;
	StringUtilities::ExpandString(data_source_parts, data_source_name, '.');

	DataSource* new_data_source = DataSource::GetDataSource(data_source_parts[0]);

	if (data_source_parts.size() != 2 || !new_data_source)
	{
		Log::Message(Log::LT_ERROR, "Bad data source name %s", data_source_name.c_str());
		data_source = nullptr;
		table_name = "";
		return false;
	}

	data_source = new_data_source;
	table_name = data_source_parts[1];
	return true;
}

} // namespace Rml
