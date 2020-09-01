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
#include "LayoutDetails.h"
#include "LayoutEngine.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/Types.h"
#include <cstddef>
#include <algorithm>

namespace Rml {


LayoutTable::CloseResult LayoutTable::FormatTable(LayoutBlockBox* table_block_context_box, Element* element_table)
{
	const Vector2f content_top_left = table_block_context_box->GetBox().GetPosition();
	const Vector2f content_containing_block = Vector2f(table_block_context_box->GetBox().GetSize().x, Math::Max(0.0f, table_block_context_box->GetBox().GetSize().y));

	const ComputedValues& computed_table = element_table->GetComputedValues();
	const Vector2f table_gap(
		ResolveValue(computed_table.column_gap, content_containing_block.x), 
		ResolveValue(computed_table.row_gap, content_containing_block.y)
	);

	Vector<Column> columns = DetermineColumnWidths(element_table, content_containing_block.x, table_gap.x);

	// Now that we have the size of each column, we can move on to formatting the elements.
	// After we format and size an element, we record its height as well, and keep the maximum_height over all cells in the current row.
	// At the end of a row, we then know the height of the row, and we can proceed by positioning the cells.

	Vector2f table_content_overflow_size;

	Vector2f table_cursor = content_top_left;
	table_cursor.y -= table_gap.y;

	struct Cell {
		Element* element_cell;
		int row_last; // The last row the cell spans
		int column_begin, column_last;
		Box box;
		Vector2f table_offset;
		float rows_accumulated_height;
	};
	Vector<Cell> cells;
	cells.reserve(columns.size());

	const int num_table_children = element_table->GetNumChildren();
	for (int i = 0, row = -1; i < num_table_children; i++)
	{
		Element* element_row = element_table->GetChild(i);

		const ComputedValues& computed_row = element_row->GetComputedValues();
		if (computed_row.display != Style::Display::TableRow)
			continue;

		row += 1;

		Box row_box;
		float row_min_height, row_max_height;
		LayoutDetails::BuildBox(row_box, content_containing_block, element_row, false, 0.f);
		LayoutDetails::GetMinMaxHeight(row_min_height, row_max_height, &computed_row, row_box, content_containing_block.y);

		// Prepare the cursor for this row
		table_cursor.x = content_top_left.x;

		const Vector2f row_element_offset = table_cursor + Vector2f(0.0f, table_gap.y) + Vector2f(row_box.GetEdge(Box::MARGIN, Box::LEFT), row_box.GetEdge(Box::MARGIN, Box::TOP));

		{
			const float row_top_offset = table_gap.y + row_box.GetEdge(Box::MARGIN, Box::TOP) + row_box.GetEdge(Box::BORDER, Box::TOP) + row_box.GetEdge(Box::PADDING, Box::TOP);
			table_cursor.y += row_top_offset;

			for (Cell& cell : cells)
				cell.rows_accumulated_height += row_top_offset;
		}

		const int num_cells_spanning_this_row = (int)cells.size();
		const int num_row_children = element_row->GetNumChildren();

		// For all child cell elements of this row, add them to the list of cells and determine their position.
		for (int j = 0, column = 0; j < num_row_children; j++)
		{
			Element* element_cell = element_row->GetChild(j);

			const ComputedValues& computed_cell = element_cell->GetComputedValues();
			if (computed_cell.display != Style::Display::TableCell)
				continue;

			const int row_span = Math::Max(1, element_cell->GetAttribute("rowspan", 1));
			const int col_span = Math::Max(1, element_cell->GetAttribute("colspan", 1));

			float spanning_column_border_width = 0;
			int column_last = -1;

			{
				// Offset the column if we have any rowspan elements from previous rows overlapping with the current column.
				for (bool continue_offset_column = true; continue_offset_column; )
				{
					continue_offset_column = false;
					for (int k = 0; k < num_cells_spanning_this_row; k++)
					{
						if (column >= cells[k].column_begin && column <= cells[k].column_last)
						{
							column = cells[k].column_last + 1;
							continue_offset_column = true;
							break;
						}
					}
				}

				column_last = column + col_span - 1;

				if (column_last >= (int)columns.size())
				{
					Log::Message(Log::LT_WARNING, "Too many columns in table row %d: %s\nThe number of columns is %d, as determined by the table columns or first table row.",
						row + 1, element_row->GetAddress().c_str(), (int)columns.size());
					break;
				}

				spanning_column_border_width = columns[column_last].cell_width + (columns[column_last].cell_offset - columns[column].cell_offset);
			}

			table_cursor.x = content_top_left.x + columns[column].cell_offset;

			// Add the new cell to our list.
			cells.emplace_back();
			Cell& cell = cells.back();

			cell.row_last = row + row_span - 1;
			cell.column_begin = column;
			cell.column_last = column_last;
			cell.element_cell = element_cell;
			cell.table_offset = table_cursor;
			cell.rows_accumulated_height = 0;

			// Determine the cell's box for formatting later, we may get an indefinite (-1) vertical content size.
			Box& box = cell.box;
			LayoutDetails::BuildBox(box, content_containing_block, element_cell, false, 0.f);

			// Determine the box width from the current column width.
			const float content_width = Math::Max(0.0f, spanning_column_border_width - box.GetSizeAcross(Box::HORIZONTAL, Box::BORDER, Box::PADDING));
			box.SetContent(Vector2f(content_width, box.GetSize().y));

			column += col_span;
		}

		// Partition the cells to determine those who end at this row.
		const auto it_cells_in_row_end = std::partition(cells.begin(), cells.end(), [row](const Cell& cell) { return cell.row_last == row; });

		// Determine the row height.
		float row_content_height = 0;
		if (row_box.GetSize().y >= 0)
		{
			//  The row has a definite size, use that.
			row_content_height = row_box.GetSize().y;
		}
		else
		{
			// The row does not have a definite size, we will use the maximum height of all its cells to determine the row height.
			// For each cell in this row, or spanning onto this row from any previous rows, increase the row height as necessary to make the cell fit.
			for (auto it = cells.begin(); it != it_cells_in_row_end; ++it)
			{
				Cell& cell = *it;
				Element* element_cell = cell.element_cell;
				Box& box = cell.box;

				// If the cell's height is also indefinite, we need to format it to get its height.
				if (box.GetSize().y < 0)
				{
					LayoutEngine::FormatElement(element_cell, content_containing_block, &box);
					box.SetContent(element_cell->GetBox().GetSize());
				}

				row_content_height = Math::Max(row_content_height, box.GetSizeAcross(Box::VERTICAL, Box::BORDER) - cell.rows_accumulated_height);
			}
		}

		row_content_height = Math::Clamp(row_content_height, row_min_height, row_max_height);

		for (Cell& cell : cells)
			cell.rows_accumulated_height += row_content_height;

		// Now we have the height of the row, position and format each cell.
		auto FormatCellsInRow = [row, &cells, it_cells_in_row_end, &table_content_overflow_size, element_table, content_containing_block, content_top_left]() {

			// Assumes the cells are already partitioned.
			for (auto it = cells.begin(); it != it_cells_in_row_end; ++it)
			{
				Cell& cell = *it;
				Element* element_cell = cell.element_cell;
				Box& box = cell.box;
				Style::VerticalAlign vertical_align = cell.element_cell->GetComputedValues().vertical_align;

				if (box.GetSize().y < 0)
				{
					const bool is_aligned = (vertical_align.type == Style::VerticalAlign::Middle || vertical_align.type == Style::VerticalAlign::Bottom);
					if (is_aligned)
					{
						// The size of the cell is indefinite, we need to get the height by formatting the cell.
						LayoutEngine::FormatElement(element_cell, content_containing_block, &box);
						box.SetContent(element_cell->GetBox().GetSize());
					}
					else
					{
						box.SetContent(Vector2f(box.GetSize().x, Math::Max(0.0f, cell.rows_accumulated_height - box.GetSizeAcross(Box::VERTICAL, Box::BORDER, Box::PADDING))));
					}
				}

				const float available_height = cell.rows_accumulated_height - box.GetSizeAcross(Box::VERTICAL, Box::BORDER);

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

				{
					// Format the cell in a new block formatting context.
					Vector2f cell_visible_overflow_size;
					LayoutEngine::FormatElement(element_cell, content_containing_block, &box, &cell_visible_overflow_size);

					table_content_overflow_size.x = Math::Max(table_content_overflow_size.x, cell.table_offset.x - content_top_left.x + cell_visible_overflow_size.x);
					table_content_overflow_size.y = Math::Max(table_content_overflow_size.y, cell.table_offset.y - content_top_left.y + cell_visible_overflow_size.y);

					// Set the position of the element within the the table container
					element_cell->SetOffset(cell.table_offset, element_table);
				}

			}

			// Remove the formatted cells from pending
			cells.erase(cells.begin(), it_cells_in_row_end);
		};

		FormatCellsInRow();

		// TODO: What to do with any remaining row-spanning cells after the last row?

		// Position and size the row element
		if (row_box.GetSize().y < 0.0f)
			row_box.SetContent(Vector2f(row_box.GetSize().x, row_content_height));

		element_row->SetOffset(row_element_offset, element_table);
		element_row->SetBox(row_box);

		const float row_bottom_offset = row_box.GetEdge(Box::MARGIN, Box::BOTTOM) + row_box.GetEdge(Box::BORDER, Box::BOTTOM) + row_box.GetEdge(Box::PADDING, Box::BOTTOM);
		table_cursor.y += row_content_height + row_bottom_offset;

		for (Cell& cell : cells)
			cell.rows_accumulated_height += row_bottom_offset;
	}

	const float table_content_height = table_cursor.y - content_top_left.y;

	// Size and position the column elements.
	for (const Column& column : columns)
	{
		if (Element* element = column.element_column)
		{
			Box box;
			LayoutDetails::BuildBox(box, content_containing_block, element, false, 0.0f);
			box.SetContent(Vector2f(column.column_width, table_content_height));
			element->SetBox(box);

			element->SetOffset(content_top_left + Vector2f(column.column_offset, 0.0f), element_table);
		}
	}

	table_block_context_box->ExtendInnerContentSize(table_content_overflow_size);

	CloseResult result = table_block_context_box->Close();
	return result;
}


LayoutTable::Columns LayoutTable::DetermineColumnWidths(Element* const element_table, const float table_content_width, const float column_gap)
{
	// The column widths are determined entirely by any <col> elements preceding the first row, and <td> elements in the first row.
	// If <col> has a fixed width, that is used. Otherwise, if <td> has a fixed width, that is used. Otherwise the column is 'flexible' width.
	// All flexible widths are then sized to evenly fill the content width of the table.

	struct ColumnMetric {
		// All widths are defined in terms of the border width of cells in the column. The column element can add padding,
		// and borders which extends beyond the cell's border width, and is instead added to 'sum_fixed_spacing'.
		float fixed_width = 0;
		float flex_width = 0;
		float min_width = 0;
		float max_width = 0;
		// The following are only used for <col> elements.
		Element* element_column = nullptr;
		bool has_fixed_size = false;
		int column_span = 0;
		float column_padding_border_left = 0;
		float column_padding_border_right = 0;
	};

	Vector<ColumnMetric> column_metrics;
	float sum_fixed_spacing = 0; // Includes column gaps and the column elements' padding, and border.

	const int num_table_children = element_table->GetNumChildren();

	Element* element_row = nullptr;

	// First look for any <col> elements preceding any <tr> elements, use them for defining the width of the respective columns.
	for (int i = 0; i < num_table_children; i++)
	{
		Element* element = element_table->GetChild(i);
		const ComputedValues& computed = element->GetComputedValues();

		if (computed.display == Style::Display::TableRow)
		{
			// End of column elements.
			element_row = element;
			break;
		}
		else if (computed.display != Style::Display::TableColumn)
		{
			continue;
		}

		const float padding_border_left = Math::Max(0.0f, ResolveValue(computed.padding_left, table_content_width)) + Math::Max(0.0f, computed.border_left_width);
		const float padding_border_right = Math::Max(0.0f, ResolveValue(computed.padding_right, table_content_width)) + Math::Max(0.0f, computed.border_right_width);
		const float padding_border_sum = padding_border_left + padding_border_right;

		sum_fixed_spacing += padding_border_sum;

		ColumnMetric column_metric;

		// Find the min/max width.
		column_metric.min_width = ResolveValue(computed.min_width, table_content_width);
		column_metric.max_width = (computed.max_width.value < 0.f ? FLT_MAX : ResolveValue(computed.max_width, table_content_width));

		if (computed.box_sizing == Style::BoxSizing::BorderBox)
		{
			column_metric.min_width = Math::Max(0.0f, column_metric.min_width - padding_border_sum);
			column_metric.max_width = Math::Max(0.0f, column_metric.max_width - padding_border_sum);
		}

		if (computed.width.type == Style::Width::Auto)
		{
			column_metric.flex_width = 1;
		}
		else
		{
			float width = ResolveValue(computed.width, table_content_width);

			if (computed.box_sizing == Style::BoxSizing::BorderBox)
				width = Math::Max(0.f, ResolveValue(computed.width, table_content_width) - padding_border_sum);

			column_metric.fixed_width = Math::Clamp(width, column_metric.min_width, column_metric.max_width);
			column_metric.has_fixed_size = true;
		}

		const int span = Math::Max(1, element->GetAttribute("span", 1));
		if (span > 1)
		{
			// Distribute any fixed widths over the columns we are spanning.
			const float width_factor = 1.f / float(span);
			column_metric.fixed_width *= width_factor;
			column_metric.min_width *= width_factor;
			column_metric.max_width *= width_factor;
		}

		column_metric.element_column = element;
		column_metric.column_span = span;
		column_metric.column_padding_border_left = padding_border_left;

		for (int j = 0; j < span; j++)
		{
			if (j == 1)
			{
				column_metric.element_column = nullptr;
				column_metric.column_span = 0;
				column_metric.column_padding_border_left = 0;
			}
			if (j == span - 1)
				column_metric.column_padding_border_right = padding_border_right;

			column_metrics.emplace_back(column_metric);
		}
	}


	const int num_row_children = (!element_row ? 0 : element_row->GetNumChildren());
	column_metrics.reserve(num_row_children);

	// Next, walk through the cells in the first table row. This procedure is subtly different from the <col>-iteration above:
	//    (1) Cells use their border width to line up the column, while <col> use their content width.
	//    (2a) We only add new columns here if they are not already represented by a <col> element.
	//    (2b) Otherwise, if the <col> element has auto (min-/max-) width, we use the cell's (min-/max-) width if it has any.
	for (int i = 0, column = 0; i < num_row_children; i++)
	{
		Element* element = element_row->GetChild(i);
		const ComputedValues& computed = element->GetComputedValues();

		if (computed.display != Style::Display::TableCell)
		{
			Log::Message(Log::LT_WARNING, "Only table cells ('display: table-cell') are allowed as children of table rows. %s", element->GetAddress().c_str());
			continue;
		}

		const float padding_border_sum =
			Math::Max(0.0f, ResolveValue(computed.padding_left, table_content_width)) +
			Math::Max(0.0f, ResolveValue(computed.padding_right, table_content_width)) +
			Math::Max(0.0f, computed.border_left_width) +
			Math::Max(0.0f, computed.border_right_width);

		ColumnMetric column_metric;

		// Find the min/max width.
		column_metric.min_width = ResolveValue(computed.min_width, table_content_width);
		column_metric.max_width = (computed.max_width.value < 0.f ? FLT_MAX : ResolveValue(computed.max_width, table_content_width));

		if (computed.box_sizing == Style::BoxSizing::ContentBox)
		{
			if (column_metric.min_width > 0)
				column_metric.min_width += padding_border_sum;
			if (column_metric.max_width < FLT_MAX)
				column_metric.max_width += padding_border_sum;
		}

		if (computed.width.type == Style::Width::Auto)
		{
			column_metric.flex_width = 1;
		}
		else
		{
			float width = ResolveValue(computed.width, table_content_width);
			
			if (computed.box_sizing == Style::BoxSizing::ContentBox)
				width += padding_border_sum;

			column_metric.fixed_width = Math::Clamp(width, column_metric.min_width, column_metric.max_width);
			column_metric.has_fixed_size = true;
		}

		const int colspan = Math::Max(1, element->GetAttribute("colspan", 1));

		if (colspan > 1)
		{
			const float width_factor = 1.f / float(colspan);
			column_metric.fixed_width *= width_factor;
			column_metric.min_width *= width_factor;
			column_metric.max_width *= width_factor;
		}

		for (int j = 0; j < colspan; j++)
		{
			if (j + column < (int)column_metrics.size())
			{
				ColumnMetric& destination = column_metrics[j + column];

				if (!destination.has_fixed_size && column_metric.has_fixed_size)
				{
					destination.fixed_width = column_metric.fixed_width;
					destination.has_fixed_size = true;
				}

				if (destination.min_width == 0)
					destination.min_width = column_metric.min_width;

				if (destination.max_width == FLT_MAX)
					destination.max_width = column_metric.max_width;
			}
			else
			{
				column_metrics.emplace_back(column_metric);
			}
		}

		column += colspan;
	}


	if (column_metrics.empty())
	{
		Log::Message(Log::LT_WARNING, "No columns or rows in table %s", element_table->GetAddress().c_str());
		// TODO: Handle this more gracefully.
		return Columns();
	}

	sum_fixed_spacing += column_gap * float((int)column_metrics.size() - 1);

	// Now all the widths are determined in terms of fixed or flexible widths.
	// Next, convert any flexible widths to fixed widths by filling up the table width.
	for (bool continue_iteration = true; continue_iteration; )
	{
		continue_iteration = false;
		float fr_to_px_ratio = 0;

		// Calculate the fr/px-ratio.
		{
			float sum_fixed_width = sum_fixed_spacing;  // [px]
			float sum_flex_width = 0;                   // [fr]

			for (const ColumnMetric& metric : column_metrics)
			{
				sum_flex_width += metric.flex_width;
				sum_fixed_width += (metric.flex_width == 0.f ? metric.fixed_width : 0.0f);
			}

			sum_flex_width = Math::Max(1.f, sum_flex_width);

			const float available_flex_width = Math::Max(0.0f, table_content_width - sum_fixed_width);
			fr_to_px_ratio = available_flex_width / sum_flex_width;
		}

		// Iterate through the columns and convert flexible widths to fixed widths.
		for (auto& metric : column_metrics)
		{
			if (metric.flex_width > 0)
			{
				const float fixed_flex_width = metric.flex_width * fr_to_px_ratio;
				metric.fixed_width = Math::Clamp(fixed_flex_width, metric.min_width, metric.max_width);

				if (metric.fixed_width != fixed_flex_width)
				{
					// We met a min/max-constraint, fix the size of this column. Start over with the procedure once we are done with all the columns.
					metric.flex_width = 0.0f;
					continue_iteration = true;
				}
			}
		}
	}

	// Fill in the resulting columns.
	Columns columns;
	columns.resize(column_metrics.size());
	float cursor_x = 0;

	for (size_t i = 0; i < column_metrics.size(); i++)
	{
		Column& col = columns[i];
		const ColumnMetric& metric = column_metrics[i];
		col.element_column = metric.element_column;
		col.cell_width = metric.fixed_width;
		col.cell_offset = cursor_x + metric.column_padding_border_left;
		col.column_width = col.cell_width; // Column content width is the cell border width, unless there is a spanning column element (see next loop).
		col.column_offset = cursor_x;

		cursor_x += metric.fixed_width + metric.column_padding_border_left + metric.column_padding_border_right + column_gap;
	}

	// Extend column widths to cover multiple columns for spanning column elements.
	for (size_t i = 0; i < column_metrics.size(); i++)
	{
		const ColumnMetric& metric = column_metrics[i];

		if (metric.column_span > 1 && metric.element_column && i + metric.column_span - 1 < column_metrics.size())
		{
			Column& col = columns[i];
			Column& col_last_span = columns[i + metric.column_span - 1];
			col.column_width = col_last_span.cell_width + (col_last_span.cell_offset - col.cell_offset);
		}
	}

	// TODO: What to do with any left-over space? Distribute evenly across columns, or make table smaller etc.

	return columns;
}

} // namespace Rml
