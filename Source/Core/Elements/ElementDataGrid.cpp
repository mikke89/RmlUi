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

#include "../../../Include/RmlUi/Core/Elements/ElementDataGrid.h"
#include "../../../Include/RmlUi/Core/Elements/DataSource.h"
#include "../../../Include/RmlUi/Core/Math.h"
#include "../../../Include/RmlUi/Core/XMLParser.h"
#include "../../../Include/RmlUi/Core/Event.h"
#include "../../../Include/RmlUi/Core/ElementDocument.h"
#include "../../../Include/RmlUi/Core/Factory.h"
#include "../../../Include/RmlUi/Core/Property.h"
#include "../../../Include/RmlUi/Core/Elements/DataFormatter.h"
#include "../../../Include/RmlUi/Core/Elements/ElementDataGridRow.h"

namespace Rml {

ElementDataGrid::ElementDataGrid(const String& tag) : Element(tag)
{
	XMLAttributes attributes;

	// Create the row for the column headers:
	ElementPtr element = Factory::InstanceElement(this, "#rmlctl_datagridrow", "datagridheader", attributes);
	header = static_cast<ElementDataGridRow*>(element.get());
	header->SetProperty(PropertyId::Display, Property(Style::Display::Block));
	header->Initialise(this);
	AppendChild(std::move(element));

	element = Factory::InstanceElement(this, "*", "datagridbody", attributes);
	body = element.get();
	body->SetProperty(PropertyId::Display, Property(Style::Display::Block));
	body->SetProperty(PropertyId::Width, Property(Style::Width::Auto));
	AppendChild(std::move(element));

	element = Factory::InstanceElement(this, "#rmlctl_datagridrow", "datagridroot", attributes);
	root = static_cast<ElementDataGridRow*>(element.get());
	root->SetProperty(PropertyId::Display, Property(Style::Display::None));
	root->Initialise(this);
	AppendChild(std::move(element), false);

	SetProperty(PropertyId::OverflowX, Property(Style::Overflow::Auto));
	SetProperty(PropertyId::OverflowY, Property(Style::Overflow::Auto));

	new_data_source = "";
}

ElementDataGrid::~ElementDataGrid()
{
}

void ElementDataGrid::SetDataSource(const String& data_source_name)
{
	new_data_source = data_source_name;
}

// Adds a column to the table.
bool ElementDataGrid::AddColumn(const String& fields, const String& formatter, float initial_width, const String& header_rml)
{
	ElementPtr header_element = Factory::InstanceElement(this, "datagridcolumn", "datagridcolumn", XMLAttributes());
	if (!header_element)
		return false;

	if (!Factory::InstanceElementText(header_element.get(), header_rml))
	{
		return false;
	}

	AddColumn(fields, formatter, initial_width, std::move(header_element));
	return true;
}

// Adds a column to the table.
void ElementDataGrid::AddColumn(const String& fields, const String& formatter, float initial_width, ElementPtr header_element)
{
	Column column;
	StringUtilities::ExpandString(column.fields, fields);
	column.formatter = DataFormatter::GetDataFormatter(formatter);
	column.header = header;
	column.current_width = initial_width;
	column.refresh_on_child_change = false;

	// The header elements are added to the header row at the top of the table.
	if (header_element)
	{
		header_element->SetProperty(PropertyId::Display, Property(Style::Display::InlineBlock));

		// Push all the width properties from the column onto the header element.
		String width = header_element->GetAttribute<String>("width", "100%");
		header_element->SetProperty("width", width);

		header->AppendChild(std::move(header_element));
	}

	// Add the fields that this column requires to the concatenated string of all column fields.
	for (size_t i = 0; i < column.fields.size(); i++)
	{
		// Don't append the depth or num_children fields, as they're handled by the row.
		if (column.fields[i] == DataSource::NUM_CHILDREN)
		{
			column.refresh_on_child_change = true;
		}
		else if (column.fields[i] != DataSource::DEPTH)
		{
			if (!column_fields.empty())
			{
				column_fields += ",";
			}
			column_fields += column.fields[i];
		}
	}

	columns.push_back(column);

	Dictionary parameters;
	parameters["index"] = (int)(columns.size() - 1);
	if (DispatchEvent(EventId::Columnadd, parameters))
	{
		root->RefreshRows();
		DirtyLayout();
	}
}

// Returns the number of columns in this table
int ElementDataGrid::GetNumColumns()
{
	return (int)columns.size();
}

// Returns the column at the specified index.
const ElementDataGrid::Column* ElementDataGrid::GetColumn(int column_index)
{
	if (column_index < 0 || column_index >= (int)columns.size())
	{
		RMLUI_ERROR;
		return nullptr;
	}

	return &columns[column_index];
}

/// Returns a CSV string containing all the fields that each column requires, in order.
const String& ElementDataGrid::GetAllColumnFields()
{
	return column_fields;
}

// Adds a new row to the table - usually only called by child rows.
ElementDataGridRow* ElementDataGrid::AddRow(ElementDataGridRow* parent, int index)
{
	// Now we make a new row at the right place then return it.
	XMLAttributes attributes;
	ElementPtr element = Factory::InstanceElement(this, "#rmlctl_datagridrow", "datagridrow", attributes);
	ElementDataGridRow* new_row = rmlui_dynamic_cast< ElementDataGridRow* >(element.get());

	new_row->Initialise(this, parent, index, header, parent->GetDepth() + 1);

	// We need to work out the table-specific row.
	int table_relative_index = parent->GetChildTableRelativeIndex(index);

	Element* child_to_insert_before = nullptr;
	if (table_relative_index < body->GetNumChildren())
	{
		child_to_insert_before = body->GetChild(table_relative_index);
	}
	body->InsertBefore(std::move(element), child_to_insert_before);

	// As the rows have changed, we need to lay out the document again.
	DirtyLayout();

	return new_row;
}

// Removes a row from the table.
void ElementDataGrid::RemoveRows(int index, int num_rows)
{
	for (int i = 0; i < num_rows; i++)
	{
		ElementDataGridRow* row = GetRow(index);
		row->SetDataSource("");
		body->RemoveChild(row);
	}

	// As the rows have changed, we need to lay out the document again.
	DirtyLayout();
}

// Returns the number of rows in the table
int ElementDataGrid::GetNumRows() const
{
	return body->GetNumChildren();
}

// Returns the row at the given index in the table.
ElementDataGridRow* ElementDataGrid::GetRow(int index) const
{
	// We need to add two to the index, to skip the header row.
	ElementDataGridRow* row = rmlui_dynamic_cast< ElementDataGridRow* >(body->GetChild(index));
	return row;
}

void ElementDataGrid::OnUpdate()
{
	if (!new_data_source.empty())
	{
		root->SetDataSource(new_data_source);
		new_data_source = "";
	}

	bool any_new_children = root->UpdateChildren();
	if (any_new_children)
	{
		DispatchEvent(EventId::Rowupdate, Dictionary());
	}
}


void ElementDataGrid::OnResize()
{
	SetScrollTop(GetScrollHeight() - GetClientHeight());

	for (int i = 0; i < header->GetNumChildren(); i++)
	{
		Element* child = header->GetChild(i);
		columns[i].current_width = child->GetBox().GetSize(Box::MARGIN).x;
	}
}

// Gets the markup and content of the element.
void ElementDataGrid::GetInnerRML(String& content) const
{
	// The only content we have is the columns, and inside them the header elements.
	for (size_t i = 0; i < columns.size(); i++)
	{
		Element* header_element = header->GetChild((int)i);

		String column_fields;
		for (size_t j = 0; j < columns[i].fields.size(); j++)
		{
			if (j != columns[i].fields.size() - 1)
			{
				column_fields += ",";
			}
			column_fields += columns[i].fields[j];
		}
		String width_attribute = header_element->GetAttribute<String>("width", "");

		content += CreateString(column_fields.size() + 32, "<col fields=\"%s\"", column_fields.c_str());
		if (!width_attribute.empty())
			content += CreateString(width_attribute.size() + 32, " width=\"%s\"", width_attribute.c_str());
		content += ">";
		header_element->GetInnerRML(content);
		content += "</col>";
	}
}

} // namespace Rml
