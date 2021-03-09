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

#include "LayoutTable.h"
#include "LayoutTableDetails.h"
#include "LayoutDetails.h"
#include "LayoutEngine.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Types.h"
#include <algorithm>
#include <numeric>

namespace Rml {

Vector2f LayoutTable::FormatTable(Box& box, Vector2f min_size, Vector2f max_size, Element* element_table)
{
	const ComputedValues& computed_table = element_table->GetComputedValues();

	// Scrollbars are illegal in the table element.
	if (!(computed_table.overflow_x == Style::Overflow::Visible || computed_table.overflow_x == Style::Overflow::Hidden) ||
		!(computed_table.overflow_y == Style::Overflow::Visible || computed_table.overflow_y == Style::Overflow::Hidden))
	{
		Log::Message(Log::LT_WARNING, "Table elements can only have 'overflow' property values of 'visible' or 'hidden'. Table will not be formatted: %s.", element_table->GetAddress().c_str());
		return Vector2f(0);
	}

	const Vector2f box_content_size = box.GetSize();
	const bool table_auto_height = (box_content_size.y < 0.0f);

	Vector2f table_content_offset = box.GetPosition();
	Vector2f table_initial_content_size = Vector2f(box_content_size.x, Math::Max(0.0f, box_content_size.y));

	Math::SnapToPixelGrid(table_content_offset, table_initial_content_size);

	// When width or height is set, they act as minimum width or height, just as in CSS.
	if (computed_table.width.type != Style::Width::Auto)
		min_size.x = Math::Max(min_size.x, table_initial_content_size.x);
	if (computed_table.height.type != Style::Height::Auto)
		min_size.y = Math::Max(min_size.y, table_initial_content_size.y);

	const Vector2f table_gap = Vector2f(
		ResolveValue(computed_table.column_gap, table_initial_content_size.x), 
		ResolveValue(computed_table.row_gap, table_initial_content_size.y)
	);

	TableGrid grid;
	grid.Build(element_table);

	// Construct the layout object and format the table.
	LayoutTable layout_table(element_table, grid, table_gap, table_content_offset, table_initial_content_size, table_auto_height, min_size, max_size);

	layout_table.FormatTable();

	// Update the box size based on the new table size.
	box.SetContent(layout_table.table_resulting_content_size);

	return layout_table.table_content_overflow_size;
}


LayoutTable::LayoutTable(Element* element_table, const TableGrid& grid, Vector2f table_gap, Vector2f table_content_offset,
	Vector2f table_initial_content_size, bool table_auto_height, Vector2f table_min_size, Vector2f table_max_size)
	: element_table(element_table), grid(grid), table_auto_height(table_auto_height), table_min_size(table_min_size), table_max_size(table_max_size),
	table_gap(table_gap), table_content_offset(table_content_offset), table_initial_content_size(table_initial_content_size)
{
	table_resulting_content_size = table_initial_content_size;
}

void LayoutTable::FormatTable()
{
	DetermineColumnWidths();

	InitializeCellBoxes();

	DetermineRowHeights();

	FormatRows();

	FormatColumns();

	FormatCells();
}

void LayoutTable::DetermineColumnWidths()
{
	// The column widths are determined entirely by any <col> elements preceding the first row, and <td> elements in the first row.
	// If <col> has a fixed width, that is used. Otherwise, if <td> has a fixed width, that is used. Otherwise the column is 'flexible' width.
	// All flexible widths are then sized to evenly fill the width of the table.
	
	// Both <col> and <colgroup> can have border/padding which extend beyond the size of <td> and <col>, respectively.
	// Margins for <td>, <col>, <colgroup> are merged to produce a single left/right margin for each column, located outside <colgroup>.

	TrackMetricList column_metrics(grid.columns.size());
	TracksSizing sizing(column_metrics, table_initial_content_size.x, table_gap.x);

	// Use the <col> and <colgroup> elements for initializing the respective columns.
	for (int i = 0; i < (int)column_metrics.size(); i++)
	{
		if (Element* element_group = grid.columns[i].element_group)
		{
			const ComputedTrackSize computed = BuildComputedColumnSize(element_group->GetComputedValues());
			const int span = grid.columns[i].group_span;

			sizing.ApplyGroupElement(i, span, computed);
		}

		if (Element* element_column = grid.columns[i].element_column)
		{
			const ComputedTrackSize computed = BuildComputedColumnSize(element_column->GetComputedValues());
			const int span = grid.columns[i].column_span;

			sizing.ApplyTrackElement(i, span, computed);
		}
	}

	// Next, walk through the cells in the first table row, use their size if the column is still auto-sized.
	for (int i = 0; i < (int)grid.columns.size(); i++)
	{
		if (Element* element_cell = grid.columns[i].element_cell)
		{
			const ComputedTrackSize computed = BuildComputedColumnSize(element_cell->GetComputedValues());
			const int colspan = grid.columns[i].cell_span;

			sizing.ApplyCellElement(i, colspan, computed);
		}
	}

	// Convert any auto widths to flexible width.
	for (TrackMetric& metric : column_metrics)
	{
		if (metric.sizing_mode == TrackSizingMode::Auto)
		{
			metric.sizing_mode = TrackSizingMode::Flexible;
			metric.fixed_size = 0.0f;
			metric.flex_size = 1.0f;
		}
	}

	// Now all widths should be either fixed or flexible, resolve all flexible widths to fixed.
	sizing.ResolveFlexibleSize();

	// Generate the column results based on the metrics.
	const float columns_full_width = BuildColumnBoxes(columns, column_metrics, grid.columns, table_gap.x);

	// Adjust the table content width based on the accumulated column widths and spacing.
	table_resulting_content_size.x = Math::Clamp(columns_full_width, table_min_size.x, table_max_size.x);
}

void LayoutTable::InitializeCellBoxes()
{
	// Requires that column boxes are already generated.
	RMLUI_ASSERT(columns.size() == grid.columns.size());

	cells.resize(grid.cells.size());

	for (int i = 0; i < (int)cells.size(); i++)
	{
		Box& box = cells[i];

		// Determine the cell's box for formatting later, we may get an indefinite (-1) vertical content size.
		LayoutDetails::BuildBox(box, table_initial_content_size, grid.cells[i].element_cell, false, 0.f);

		// Determine the cell's content width. Include any spanning columns in the cell width.
		const float cell_border_width = GetSpanningCellBorderSize(columns, grid.cells[i].column_begin, grid.cells[i].column_last);
		const float content_width = Math::Max(0.0f, cell_border_width - box.GetSizeAcross(Box::HORIZONTAL, Box::BORDER, Box::PADDING));
		box.SetContent(Vector2f(content_width, box.GetSize().y));
	}
}

void LayoutTable::DetermineRowHeights()
{
	/*
		The table height algorithm works similar to the table width algorithm. The major difference is that 'auto' row height
		will use the height of the largest formatted cell in the row.
		
		Table row height: 
		  auto: Height of largest cell in row.
		  length: Fixed size.
		  percentage < 100%: Fixed size, resolved against table initial height.
		  percentage >= 100%: Flexible size.

		Table height:
		  auto: Height is sum of all rows.
		  length/percentage: Fixed minimum size. If row height sum is larger, increase table size. If row sum is smaller, try to increase
		                     row heights, but respect max-heights. If table is still larger than row-sum, leave empty space.
	*/

	// Requires that cell boxes have been initialized.
	RMLUI_ASSERT(cells.size() == grid.cells.size());

	TrackMetricList row_metrics(grid.rows.size());
	TracksSizing sizing(row_metrics, table_initial_content_size.y, table_gap.y);
	bool percentage_size_used = false;

	// First look for any <col> and <colgroup> elements preceding any <tr> elements, use them for initializing the respective columns.
	for (int i = 0; i < (int)grid.rows.size(); i++)
	{
		if (Element* element_group = grid.rows[i].element_group)
		{
			// The padding/border/margin of column groups are used, but their widths are ignored.
			const ComputedTrackSize computed = BuildComputedRowSize(element_group->GetComputedValues());
			const int span = grid.rows[i].group_span;

			sizing.ApplyGroupElement(i, span, computed);
		}

		if (Element* element_row = grid.rows[i].element_row)
		{
			// The padding/border/margin and widths of columns are used.
			const ComputedTrackSize computed = BuildComputedRowSize(element_row->GetComputedValues());
			
			if (computed.size.type == Style::LengthPercentageAuto::Percentage)
				percentage_size_used = true;

			sizing.ApplyTrackElement(i, 1, computed);
		}
	}

	if (table_auto_height && percentage_size_used)
	{
		Log::Message(Log::LT_WARNING, 
			"Table has one or more rows that use percentages for height. However, initial table height is undefined, thus "
			"these rows will become flattened. Set a fixed height on the table, or use fixed or 'auto' row heights. In element: %s.",
			element_table->GetAddress().c_str()
		);
	}

	// Next, find the height of rows that use auto height.
	// Auto height rows set their height according to the largest formatted cell size. This differs from the column width algorithm.
	for (int i = 0; i < (int)row_metrics.size(); i++)
	{
		TrackMetric& row_metric = row_metrics[i];

		if (row_metric.sizing_mode == TrackSizingMode::Auto)
		{
			struct CellLastRowComp {
				bool operator() (const TableGrid::Cell& cell, int i) const { return cell.row_last < i; }
				bool operator() (int i, const TableGrid::Cell& cell) const { return i < cell.row_last; }
			};

			// Determine which cells end at this row.
			const auto it_pair = std::equal_range(grid.cells.begin(), grid.cells.end(), i, CellLastRowComp{});
			const int cell_begin = int(it_pair.first - grid.cells.begin());
			const int cell_end = int(it_pair.second - grid.cells.begin());

			for (int cell_index = cell_begin; cell_index < cell_end; cell_index++)
			{
				const TableGrid::Cell& grid_cell = grid.cells[cell_index];
				Element* element_cell = grid_cell.element_cell;
				Box& box = cells[cell_index];

				// If both the row and the cell heights are 'auto', we need to format the cell to get its height.
				if (box.GetSize().y < 0)
				{
					LayoutEngine::FormatElement(element_cell, table_initial_content_size, &box);
					box.SetContent(element_cell->GetBox().GetSize());
				}

				// Find the height of the cell which applies only to this row. 
				// In case it spans multiple rows, we must first subtract the height of any previous rows it spans. It is
				// unsupported if any spanning rows are flexibly sized, in which case we consider their size to be zero.
				const float gap_from_spanning_rows = table_gap.y * float(grid_cell.row_last - grid_cell.row_begin);

				const float height_from_spanning_rows = std::accumulate(row_metrics.begin() + grid_cell.row_begin, row_metrics.begin() + grid_cell.row_last, gap_from_spanning_rows, [](float acc_height, const TrackMetric& metric) {
					return acc_height + metric.fixed_size + metric.column_padding_border_a + metric.column_padding_border_b
						+ metric.group_padding_border_a + metric.group_padding_border_b + metric.sum_margin_a + metric.sum_margin_b;
				});

				const float cell_inrow_height = box.GetSizeAcross(Box::VERTICAL, Box::BORDER) - height_from_spanning_rows;

				// Now we have the height of the cell, increase the row height to accompany the cell.
				row_metric.fixed_size = Math::Max(row_metric.fixed_size, cell_inrow_height);
			}

			row_metric.sizing_mode = TrackSizingMode::Fixed;
			row_metric.flex_size = 0.0f;
			row_metric.fixed_size = Math::Clamp(row_metric.fixed_size, row_metric.min_size, row_metric.max_size);
		}
	}

	// Now all heights should be either fixed or flexible, resolve all flexible heights to fixed.
	sizing.ResolveFlexibleSize();

	// Generate the column results based on the metrics.
	const float rows_full_height = BuildRowBoxes(rows, row_metrics, grid.rows, table_gap.y);

	// Adjust the table content width based on the accumulated column widths and spacing.
	table_resulting_content_size.y = Math::Clamp(Math::Max(rows_full_height, table_initial_content_size.y), table_min_size.y, table_max_size.y);
}

void LayoutTable::FormatRows()
{
	RMLUI_ASSERT(rows.size() == grid.rows.size());

	// Size and position the row and row group elements.
	//   @performance: Maybe build the box using a simpler algorithm. Eg. we don't do anything for auto
	//                 margins. Some of the information is already gathered in the TrackMetric.
	auto FormatRow = [this](Element* element, float content_height, float offset_y) {
		Box box;
		LayoutDetails::BuildBox(box, table_initial_content_size, element, false, 0.0f);
		const Vector2f content_size(
			table_resulting_content_size.x - box.GetSizeAcross(Box::HORIZONTAL, Box::MARGIN, Box::PADDING),
			content_height
		);
		box.SetContent(content_size);
		element->SetBox(box);

		element->SetOffset(table_content_offset + Vector2f(box.GetEdge(Box::MARGIN, Box::LEFT), offset_y), element_table);
	};

	for (int i = 0; i < (int)rows.size(); i++)
	{
		const TableGrid::Row& grid_row = grid.rows[i];
		const TrackBox& box = rows[i];

		if (grid_row.element_row)
			FormatRow(grid_row.element_row, box.track_size, box.track_offset);

		if (grid_row.element_group)
			FormatRow(grid_row.element_group, box.group_size, box.group_offset);
	}
}

void LayoutTable::FormatColumns()
{
	RMLUI_ASSERT(columns.size() == grid.columns.size());

	// Size and position the column and column group elements.
	auto FormatColumn = [this](Element* element, float content_width, float offset_x) {
		Box box;
		LayoutDetails::BuildBox(box, table_initial_content_size, element, false, 0.0f);
		const Vector2f content_size(
			content_width,
			table_resulting_content_size.y - box.GetSizeAcross(Box::VERTICAL, Box::MARGIN, Box::PADDING)
		);
		box.SetContent(content_size);
		element->SetBox(box);

		element->SetOffset(table_content_offset + Vector2f(offset_x, box.GetEdge(Box::MARGIN, Box::TOP)), element_table);
	};

	for (int i = 0; i < (int)columns.size(); i++)
	{
		const TableGrid::Column& grid_column = grid.columns[i];
		const TrackBox& box = columns[i];

		if (grid_column.element_column)
			FormatColumn(grid_column.element_column, box.track_size, box.track_offset);

		if (grid_column.element_group)
			FormatColumn(grid_column.element_group, box.group_size, box.group_offset);
	}
}

void LayoutTable::FormatCells()
{
	RMLUI_ASSERT(cells.size() == grid.cells.size());

	for (int cell_index = 0; cell_index < (int)cells.size(); cell_index++)
	{
		const TableGrid::Cell& grid_cell = grid.cells[cell_index];
		Element* element_cell = grid_cell.element_cell;

		Box& box = cells[cell_index];
		Style::VerticalAlign vertical_align = element_cell->GetComputedValues().vertical_align;

		const float cell_border_height = GetSpanningCellBorderSize(rows, grid_cell.row_begin, grid_cell.row_last);
		const Vector2f cell_offset = table_content_offset + Vector2f(
			columns[grid_cell.column_begin].cell_offset,
			rows[grid_cell.row_begin].cell_offset
		);
		
		// Determine the height of the cell.
		if (box.GetSize().y < 0)
		{
			const bool is_aligned = (vertical_align.type == Style::VerticalAlign::Middle || vertical_align.type == Style::VerticalAlign::Bottom);
			if (is_aligned)
			{
				// We need to format the cell to know how much padding to add.
				LayoutEngine::FormatElement(element_cell, table_initial_content_size, &box);
				box.SetContent(element_cell->GetBox().GetSize());
			}
			else
			{
				// We don't need to add any padding and can thus avoid formatting, just set the height to the row height.
				box.SetContent(Vector2f(box.GetSize().x, Math::Max(0.0f, cell_border_height - box.GetSizeAcross(Box::VERTICAL, Box::BORDER, Box::PADDING))));
			}
		}

		const float available_height = cell_border_height - box.GetSizeAcross(Box::VERTICAL, Box::BORDER);

		if (available_height > 0)
		{
			// Pad the cell for vertical alignment
			float add_padding_top;
			float add_padding_bottom;

			switch (vertical_align.type)
			{
			case Style::VerticalAlign::Bottom:
				add_padding_top = available_height;
				add_padding_bottom = 0;
				break;
			case Style::VerticalAlign::Middle:
				add_padding_top = 0.5f * available_height;
				add_padding_bottom = 0.5f * available_height;
				break;
			case Style::VerticalAlign::Top:
			default:
				add_padding_top = 0.0f;
				add_padding_bottom = available_height;
			}

			box.SetEdge(Box::PADDING, Box::TOP, box.GetEdge(Box::PADDING, Box::TOP) + add_padding_top);
			box.SetEdge(Box::PADDING, Box::BOTTOM, box.GetEdge(Box::PADDING, Box::BOTTOM) + add_padding_bottom);
		}

		// Format the cell in a new block formatting context.
		// @performance: We may have already formatted the element during the above procedures without the extra padding. In that case, we may
		//   instead set the new box and offset all descending elements whose offset parent is the cell, to account for the new padding box.
		//   That should be faster than formatting the element again, but there may be edge-cases not accounted for.
		Vector2f cell_visible_overflow_size;
		LayoutEngine::FormatElement(element_cell, table_initial_content_size, &box, &cell_visible_overflow_size);

		// Set the position of the element within the the table container
		element_cell->SetOffset(cell_offset, element_table);

		// The cell contents may overflow, propagate this to the table.
		table_content_overflow_size.x = Math::Max(table_content_overflow_size.x, cell_offset.x - table_content_offset.x + cell_visible_overflow_size.x);
		table_content_overflow_size.y = Math::Max(table_content_overflow_size.y, cell_offset.y - table_content_offset.y + cell_visible_overflow_size.y);
	}
}


} // namespace Rml
