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

#include "../../../Include/RmlUi/Core/Elements/ElementDataGridRow.h"
#include "../../../Include/RmlUi/Core/Elements/DataSource.h"
#include "../../../Include/RmlUi/Core/Elements/DataFormatter.h"
#include "../../../Include/RmlUi/Core/Elements/ElementDataGrid.h"
#include "../../../Include/RmlUi/Core/Elements/ElementDataGridCell.h"
#include "../../../Include/RmlUi/Core/Factory.h"
#include "../../../Include/RmlUi/Core/Types.h"
#include "../Clock.h"

namespace Rml {

static const float MAX_UPDATE_TIME = 0.001f;

ElementDataGridRow::ElementDataGridRow(const String& tag) : Element(tag)
{
	parent_grid = nullptr;
	parent_row = nullptr;
	child_index = -1;
	depth = -1;

	data_source = nullptr;

	table_relative_index = -1;
	table_relative_index_dirty = true;
	dirty_cells = true;
	dirty_children = false;
	row_expanded = true;

	SetProperty(PropertyId::WhiteSpace, Property(Style::WhiteSpace::Nowrap));
	SetProperty(PropertyId::Display, Property(Style::Display::InlineBlock));
}

ElementDataGridRow::~ElementDataGridRow()
{
	if (data_source)
	{
		data_source->DetachListener(this);
		data_source = nullptr;
	}
}

void ElementDataGridRow::Initialise(ElementDataGrid* _parent_grid, ElementDataGridRow* _parent_row, int _child_index, ElementDataGridRow* header_row, int _depth)
{
	parent_grid = _parent_grid;
	parent_row = _parent_row;
	child_index = _child_index;
	depth = _depth;

	// We start all the rows collapsed, except for the root row.
	if (child_index != -1)
	{
		row_expanded = false;
	}

	int num_columns = parent_grid->GetNumColumns();
	XMLAttributes cell_attributes;
	for (int i = 0; i < num_columns; i++)
	{
		ElementPtr element = Factory::InstanceElement(this, "#rmlctl_datagridcell", "datagridcell", cell_attributes);
		ElementDataGridCell* cell = rmlui_dynamic_cast< ElementDataGridCell* >(element.get());
		cell->Initialise(i, header_row->GetChild(i));
		cell->SetProperty(PropertyId::Display, Property(Style::Display::InlineBlock));
		AppendChild(std::move(element));
	}
}

void ElementDataGridRow::SetChildIndex(int _child_index)
{
	if (child_index != _child_index)
	{	
		child_index = _child_index;

		if (parent_row)
		{
			parent_row->ChildChanged(child_index);
		}
	}
}

int ElementDataGridRow::GetDepth()
{
	return depth;
}

void ElementDataGridRow::SetDataSource(const String& data_source_name)
{
	if (data_source != nullptr)
	{
		data_source->DetachListener(this);
		data_source = nullptr;
	}

	if (ParseDataSource(data_source, data_table, data_source_name))
	{
		data_source->AttachListener(this);
		RefreshRows();
	}
}

bool ElementDataGridRow::UpdateChildren()
{
	if (dirty_children)
	{
		double start_time = Clock::GetElapsedTime();
		
		RowQueue dirty_rows;
		dirty_rows.push(this);

		while (!dirty_rows.empty())
		{
			ElementDataGridRow* dirty_row = dirty_rows.front();
			dirty_rows.pop();
			
			float time_slice = MAX_UPDATE_TIME - float(Clock::GetElapsedTime() - start_time);
			if (time_slice <= 0.0f)
				break;

			dirty_row->LoadChildren(time_slice);
			for (size_t i = 0; i < dirty_row->children.size(); i++)
			{
				if (dirty_row->children[i]->dirty_cells || dirty_row->children[i]->dirty_children)
				{
					dirty_rows.push(dirty_row->children[i]);
				}
			}
		}

		return true;
	}
	
	return false;
}

// Returns the number of children that aren't dirty (have been loaded)
int ElementDataGridRow::GetNumLoadedChildren()
{
	int num_loaded_children = 0;
	for (size_t i = 0; i < children.size(); i++)
	{
		if (!children[i]->dirty_cells)
		{
			num_loaded_children++;
		}
		num_loaded_children += children[i]->GetNumLoadedChildren();
	}

	return num_loaded_children;
}

bool ElementDataGridRow::IsRowExpanded()
{
	return row_expanded;
}

void ElementDataGridRow::ExpandRow()
{
	row_expanded = true;

	for (size_t i = 0; i < children.size(); i++)
	{
		children[i]->Show();
	}

	DirtyLayout();
}

void ElementDataGridRow::CollapseRow()
{
	row_expanded = false;

	for (size_t i = 0; i < children.size(); i++)
	{
		children[i]->Hide();
	}

	DirtyLayout();
}

void ElementDataGridRow::ToggleRow()
{
	if (row_expanded)
	{
		CollapseRow();
	}
	else
	{
		ExpandRow();
	}
}

// Returns the index of this row, relative to its parent.
int ElementDataGridRow::GetParentRelativeIndex()
{
	return child_index;
}

// Returns the index of this row, relative to the table rather than its parent.
int ElementDataGridRow::GetTableRelativeIndex()
{
	if (!parent_row)
	{
		return -1;
	}

	if (table_relative_index_dirty)
	{
		table_relative_index = parent_row->GetChildTableRelativeIndex(child_index);
		table_relative_index_dirty = false;
	}

	return table_relative_index;
}

// Returns the parent row of this row.
ElementDataGridRow* ElementDataGridRow::GetParentRow()
{
	return parent_row;
}

// Returns the grid that this row belongs to.
ElementDataGrid* ElementDataGridRow::GetParentGrid()
{
	return parent_grid;
}

void ElementDataGridRow::OnDataSourceDestroy(DataSource* /*data_source*/)
{
	if(data_source != nullptr)
	{
		data_source->DetachListener(this);
		data_source = nullptr;
	}
	RemoveChildren();
}

void ElementDataGridRow::OnRowAdd(DataSource* _data_source, const String& _data_table, int first_row_added, int num_rows_added)
{
	if (_data_source == data_source && _data_table == data_table)
		AddChildren(first_row_added, num_rows_added);
}

void ElementDataGridRow::OnRowRemove(DataSource* _data_source, const String& _data_table, int first_row_removed, int num_rows_removed)
{
	if (_data_source == data_source && _data_table == data_table)
		RemoveChildren(first_row_removed, num_rows_removed);
}

void ElementDataGridRow::OnRowChange(DataSource* _data_source, const String& _data_table, int first_row_changed, int num_rows_changed)
{
	if (_data_source == data_source && _data_table == data_table)
		ChangeChildren(first_row_changed, num_rows_changed);
}

void ElementDataGridRow::OnRowChange(DataSource* _data_source, const String& _data_table)
{
	if (_data_source == data_source && _data_table == data_table)
		RefreshRows();
}

// Removes all the child cells and fetches them again from the data source.
void ElementDataGridRow::RefreshRows()
{
	// Remove all our child rows from the table.
	RemoveChildren();

	// Load the children from the data source.
	if (data_source != nullptr)
	{
		int num_rows = data_source->GetNumRows(data_table);
		if (num_rows > 0)
		{
			AddChildren(0, num_rows);
		}
	}
}

// Called when a row change (addition or removal) occurs in one of our
// children.
void ElementDataGridRow::ChildChanged(int child_row_index)
{
	for (int i = child_row_index + 1; i < (int)children.size(); i++)
	{
		children[i]->DirtyTableRelativeIndex();
	}

	if (parent_row)
	{
		parent_row->ChildChanged(child_index);
	}
}

// Checks if any columns are dependent on the number of children present, and
// refreshes them from the data source if they are.
void ElementDataGridRow::RefreshChildDependentCells()
{
	if (child_index != -1)
	{
		for (int i = 0; i < parent_grid->GetNumColumns(); i++)
		{
			const ElementDataGrid::Column* column = parent_grid->GetColumn(i);
			if (column->refresh_on_child_change)
			{
				DirtyCells();
			}
		}
	}
}

// Called whenever a row is added or removed above ours.
void ElementDataGridRow::DirtyTableRelativeIndex()
{
	if (table_relative_index_dirty)
		return;

	for (size_t i = 0; i < children.size(); i++)
	{
		children[i]->DirtyTableRelativeIndex();
	}

	table_relative_index_dirty = true;
}

int ElementDataGridRow::GetChildTableRelativeIndex(int child_index)
{
	// We start with our index, then count down each of the children until we
	// reach child_index. For each child we skip by add one (for the child
	// itself) and all of its descendants.
	int child_table_index = GetTableRelativeIndex() + 1;

	for (int i = 0; i < child_index; i++)
	{
		child_table_index++;
		child_table_index += children[i]->GetNumDescendants();
	}

	return child_table_index;
}

// Adds children underneath this row, and fetches their contents (and possible
// children) from the row's data source.
void ElementDataGridRow::AddChildren(int first_row_added, int num_rows_added)
{
	if (first_row_added == -1)
	{
		first_row_added = (int)children.size();
	}

	// We need to make a row for each new child, then pass through the cell
	// information and the child's data source (if one exists.)
	if (data_source != nullptr)
	{
		for (int i = 0; i < num_rows_added; i++)
		{
			int row_index = first_row_added + i;

			// Make a new row:
			ElementDataGridRow* new_row = parent_grid->AddRow(this, row_index);
			children.insert(children.begin() + row_index, new_row);

			if (!row_expanded)
			{
				new_row->SetProperty(PropertyId::Display, Property(Style::Display::None));
			}
		}

		for (int i = first_row_added + num_rows_added; i < (int)children.size(); i++)
		{
			children[i]->SetChildIndex(i);
			children[i]->DirtyTableRelativeIndex();
		}

		if (parent_row)
		{
			parent_row->ChildChanged(child_index);
		}
	}

	RefreshChildDependentCells();
	DirtyRow();

	Dictionary parameters;
	parameters["first_row_added"] = GetChildTableRelativeIndex(first_row_added);
	parameters["num_rows_added"] = num_rows_added;
	
	parent_grid->DispatchEvent(EventId::Rowadd, parameters);
}

void ElementDataGridRow::RemoveChildren(int first_row_removed, int num_rows_removed)
{
	if (num_rows_removed == -1)
		num_rows_removed = (int)children.size() - first_row_removed;

	for (int i = num_rows_removed - 1; i >= 0; i--)
	{
		children[first_row_removed + i]->RemoveChildren();
		parent_grid->RemoveRows(children[first_row_removed + i]->GetTableRelativeIndex());
	}

	children.erase(children.begin() + first_row_removed, children.begin() + (first_row_removed + num_rows_removed));
    for (int i = first_row_removed; i < (int) children.size(); i++)
	{
		children[i]->SetChildIndex(i);
		children[i]->DirtyTableRelativeIndex();
	}

	Dictionary parameters;
	parameters["first_row_removed"] = GetChildTableRelativeIndex(first_row_removed);
	parameters["num_rows_removed"] = num_rows_removed;
	
	parent_grid->DispatchEvent(EventId::Rowremove, parameters);
}

void ElementDataGridRow::ChangeChildren(int first_row_changed, int num_rows_changed)
{
	for (int i = first_row_changed; i < first_row_changed + num_rows_changed; i++)
		children[i]->DirtyCells();

	Dictionary parameters;
	parameters["first_row_changed"] = GetChildTableRelativeIndex(first_row_changed);
	parameters["num_rows_changed"] = num_rows_changed;
	
	parent_grid->DispatchEvent(EventId::Rowchange, parameters);
}

// Returns the number of rows under this row (children, grandchildren, etc)
int ElementDataGridRow::GetNumDescendants()
{
	int num_descendants = (int)children.size();

	for (size_t i = 0; i < children.size(); i++)
		num_descendants += children[i]->GetNumDescendants();

	return num_descendants;
}

// Adds the cell contents, and marks the row as loaded.
void ElementDataGridRow::Load(const DataQuery& row_information)
{
	// Check for a data source. If they're both set then we set
	// ourselves up with it.
	if (row_information.IsFieldSet(DataSource::CHILD_SOURCE))
	{
		String data_source = row_information.Get< String >(DataSource::CHILD_SOURCE, "");
		if (!data_source.empty())
		{
			SetDataSource(data_source);
		}
		else
		{
			// If we've no data source, then we should remove any children.
			RemoveChildren();
		}
	}

	// Now load our cells.
	for (int i = 0; i < parent_grid->GetNumColumns(); i++)
	{
		Element* cell = GetChild(i);

		if (cell)
		{
			// Fetch the column:
			const ElementDataGrid::Column* column = parent_grid->GetColumn(i);

			// Now we use the column's formatter to process the raw data into the
			// XML string, and parse that into the actual Elements. If there is
			// no formatter, then we just send through the raw text, in CVS form.
			StringList raw_data;
			raw_data.reserve(column->fields.size());
			size_t raw_data_total_len = 0;
			for (size_t j = 0; j < column->fields.size(); j++)
			{
				if (column->fields[j] == DataSource::DEPTH)
				{
					raw_data.push_back(CreateString(8, "%d", depth));
					raw_data_total_len += raw_data.back().length();
				}
				else if (column->fields[j] == DataSource::NUM_CHILDREN)
				{
					raw_data.push_back(CreateString(8, "%zu", children.size()));
					raw_data_total_len += raw_data.back().length();
				}
				else
				{
					raw_data.push_back(row_information.Get< String >(column->fields[j], ""));
					raw_data_total_len += raw_data.back().length();
				}
			}

			String cell_string;
			if (column->formatter)
			{
				column->formatter->FormatData(cell_string, raw_data);
			}
			else
			{
				cell_string.reserve(raw_data_total_len + raw_data.size() + 1);
				for (size_t j = 0; j < raw_data.size(); j++)
				{
					if (j > 0)
					{
						cell_string += ",";
					}
					cell_string += raw_data[j];
				}
			}

			// Remove all the cell's current contents.
			while (cell->GetNumChildren(true) > 0)
			{
				cell->RemoveChild(cell->GetChild(0));
			}

			// Add the new contents to the cell.
			Factory::InstanceElementText(cell, cell_string);
		}
		else
		{
			RMLUI_ERROR;
		}
	}

	dirty_cells = false;
}

// Instantiates the children that haven't been fully loaded yet.
void ElementDataGridRow::LoadChildren(float time_slice)
{
	double start_time = Clock::GetElapsedTime();

	int data_query_offset = -1;
	int data_query_limit = -1;

	// Iterate through the list of children and find the holes of unloaded
	// rows.
	//  - If we find an unloaded element, and we haven't set the offset
	//    (beginning of the hole), then we've hit a new hole. We set the offset
	//    to the current index and the limit (size of the hole) to 1. If we've
	//    found a hole already and we find an unloaded element, then we
	//    increase the size of the hole.
	//  - The end of a hole is either a loaded element or the end of the list.
	//    In either case, we check if we have a hole that's unfilled, and if
	//    so, we fill it.
	bool any_dirty_children = false;
	for (size_t i = 0; i < children.size() && float(Clock::GetElapsedTime() - start_time) < time_slice; i++)
	{
		if (children[i]->dirty_cells)
		{
			any_dirty_children = true;
			if (data_query_offset == -1)
			{
				data_query_offset = (int)i;
				data_query_limit = 1;
			}
			else
			{
				data_query_limit++;
			}
		}
		else if (children[i]->dirty_children)
		{
			any_dirty_children = true;
		}

		bool end_of_list = i == children.size() - 1;
		bool unfilled_hole = data_query_offset != -1;
		bool end_of_hole_found = !children[i]->dirty_cells;

		// If this is the last element and we've found no holes (or filled them
		// all in) then all our children are loaded.
		if (end_of_list && !unfilled_hole)
		{
			if (!any_dirty_children)
			{
				dirty_children = false;
			}
		}
		// Otherwise, if we have a hole outstanding and we've either hit the
		// end of the list or the end of the hole, fill the hole.
		else if (unfilled_hole && (end_of_list || end_of_hole_found))
		{
			float load_time_slice = time_slice - float(Clock::GetElapsedTime() - start_time);
			LoadChildren(data_query_offset, data_query_limit, load_time_slice);
			data_query_offset =  -1;
			data_query_limit = -1;
		}
	}

	if (children.empty())
	{
		dirty_children = false;
	}
}

void ElementDataGridRow::LoadChildren(int first_row_to_load, int num_rows_to_load, double time_slice)
{
	double start_time = Clock::GetElapsedTime();

	// Now fetch these new children from the data source, pass them
	// through each column's data formatter, and add them as our new
	// child rows.
	String column_query = parent_grid->GetAllColumnFields() + "," + DataSource::CHILD_SOURCE;
	DataQuery query(data_source, data_table, column_query, first_row_to_load, num_rows_to_load);

	for (int i = 0; i < num_rows_to_load; i++)
	{
		int index = first_row_to_load + i;

		if (!query.NextRow())
		{
			Log::Message(Log::LT_WARNING, "Failed to load row %d from data source %s", i, data_table.c_str());
		}

		// Now load the child with the row in the query.
		children[index]->Load(query);

		if (float(Clock::GetElapsedTime() - start_time) > time_slice)
		{
			break;
		}
	}
}

void ElementDataGridRow::DirtyCells()
{
	dirty_cells = true;
	if (parent_row)
	{
		parent_row->DirtyRow();
	}
}

void ElementDataGridRow::DirtyRow()
{
	dirty_children = true;
	if (parent_row)
	{
		parent_row->DirtyRow();
	}
}

// Sets this row's child rows to be visible.
void ElementDataGridRow::Show()
{
	SetProperty(PropertyId::Display, Property(Style::Display::InlineBlock));

	if (row_expanded)
	{
		for (size_t i = 0; i < children.size(); i++)
		{
			children[i]->Show();
		}
	}
}

// Sets this row's children to be invisible.
void ElementDataGridRow::Hide()
{
	SetProperty(PropertyId::Display, Property(Style::Display::None));

	for (size_t i = 0; i < children.size(); i++)
	{
		children[i]->Hide();
	}
}

} // namespace Rml
