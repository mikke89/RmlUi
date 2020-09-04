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

	const Vector2f table_gap(10.f, 0.f);

	Vector<float> column_border_widths = DetermineColumnWidths(element_table, content_containing_block, table_gap);

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
	cells.reserve(column_border_widths.size());

	const int num_table_children = element_table->GetNumChildren();
	int row = -1;
	for (int i = 0; i < num_table_children; i++)
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
		table_cursor.y += table_gap.y;

		const Vector2f row_element_offset = table_cursor + Vector2f(row_box.GetEdge(Box::MARGIN, Box::LEFT), row_box.GetEdge(Box::MARGIN, Box::TOP));

		{
			const float row_top_offset = row_box.GetEdge(Box::MARGIN, Box::TOP) + row_box.GetEdge(Box::BORDER, Box::TOP) + row_box.GetEdge(Box::PADDING, Box::TOP);
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
				const int column_offset_from = column;

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

				if (column_last >= (int)column_border_widths.size())
				{
					Log::Message(Log::LT_WARNING, "Too many columns in table row %d: %s\nThe number of columns is %d, as determined by the first table row.",
						row + 1, element_row->GetAddress().c_str(), (int)column_border_widths.size());
					break;
				}

				for (int k = column_offset_from; k < column; k++)
					table_cursor.x += column_border_widths[k];

				table_cursor.x += table_gap.x * float(column - column_offset_from);

				for (int k = column; k <= column_last; k++)
					spanning_column_border_width += column_border_widths[k];

				spanning_column_border_width += table_gap.x * float(col_span - 1);
			}

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

			table_cursor.x += spanning_column_border_width + table_gap.x;

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

	table_block_context_box->ExtendInnerContentSize(table_content_overflow_size);

	CloseResult result = table_block_context_box->Close();
	return result;
}


LayoutTable::ColumnWidths LayoutTable::DetermineColumnWidths(Element* element_table, const Vector2f containing_block, const Vector2f table_gap)
{
	ColumnWidths column_widths;

	// Determine the widths of the cells.
	Element* element_first_row = nullptr;

	for (int i = 0; i < element_table->GetNumChildren(); i++)
	{
		element_first_row = element_table->GetChild(i);
		if (element_first_row->GetDisplay() != Style::Display::TableRow)
		{
			Log::Message(Log::LT_WARNING, "Only table rows ('display: table-row') are allowed as children of tables. %s", element_first_row->GetAddress().c_str());
		}
		else
		{
			break;
		}
	}

	if (!element_first_row)
	{
		Log::Message(Log::LT_WARNING, "No rows in table. %s", element_table->GetAddress().c_str());
		// TODO: Handle this more gracefully.
		return ColumnWidths();
	}


	const int num_row_children = element_first_row->GetNumChildren();

	struct CellMetrics {
		float fixed_content_width;
		float padding_border_edges_width;
		float flex_width;
		float min_content_width;
		float max_content_width;
	};
	Vector<CellMetrics> cell_metrics;
	cell_metrics.reserve(num_row_children);

	for (int i = 0; i < num_row_children; i++)
	{
		Element* element_cell = element_first_row->GetChild(i);

		const ComputedValues& computed = element_cell->GetComputedValues();

		if (computed.display != Style::Display::TableCell)
		{
			Log::Message(Log::LT_WARNING, "Only table cells ('display: table-cell') are allowed as children of table rows. %s", element_cell->GetAddress().c_str());
			continue;
		}

		const float padding_border_edges_width =
			Math::Max(0.0f, ResolveValue(computed.padding_left, containing_block.x)) +
			Math::Max(0.0f, ResolveValue(computed.padding_right, containing_block.x)) +
			Math::Max(0.0f, computed.border_left_width) +
			Math::Max(0.0f, computed.border_right_width);

		auto min_max_widths = [containing_block_width = containing_block.x](float& min_width, float& max_width, float border_padding_edges_width, const ComputedValues& computed) {
			min_width = ResolveValue(computed.min_width, containing_block_width);
			max_width = (computed.max_width.value < 0.f ? FLT_MAX : ResolveValue(computed.max_width, containing_block_width));

			if (computed.box_sizing == Style::BoxSizing::BorderBox)
			{
				min_width = Math::Max(0.0f, min_width - border_padding_edges_width);
				max_width = Math::Max(0.0f, max_width - border_padding_edges_width);
			}
		};

		CellMetrics metric = {};
		metric.padding_border_edges_width = padding_border_edges_width;
		min_max_widths(metric.min_content_width, metric.max_content_width, padding_border_edges_width, computed);

		if (computed.width.type == Style::Width::Auto)
		{
			metric.flex_width = 1;
		}
		else
		{
			if (computed.box_sizing == Style::BoxSizing::ContentBox)
				metric.fixed_content_width = ResolveValue(computed.width, containing_block.x);
			else
				metric.fixed_content_width = Math::Max(0.f, ResolveValue(computed.width, containing_block.x - padding_border_edges_width));

			metric.fixed_content_width = Math::Clamp(metric.fixed_content_width, metric.min_content_width, metric.max_content_width);
		}

		const int colspan = Math::Max(1, element_cell->GetAttribute("colspan", 1));
		if (colspan > 1)
		{
			const float width_factor = 1.f / float(colspan);
			metric.fixed_content_width *= width_factor;
			metric.min_content_width *= width_factor;
			metric.max_content_width *= width_factor;
		}

		for(int j = 0; j < colspan; j++)
			cell_metrics.push_back(metric);
	}


	auto calculate_fr_to_px_ratio = [table_content_width = containing_block.x, column_gap = table_gap.x, &cell_metrics]() {
		float sum_fixed_width = 0; // [px]
		float sum_flex_width = 0;  // [fr]

		for (const CellMetrics& metric : cell_metrics)
		{
			sum_flex_width += metric.flex_width;
			sum_fixed_width += (metric.flex_width == 0.f ? metric.fixed_content_width : 0.0f) + metric.padding_border_edges_width + column_gap;
		}

		sum_flex_width = Math::Max(1.f, sum_flex_width);
		sum_fixed_width = Math::Max(0.f, sum_fixed_width - column_gap);

		const float available_flex_width = table_content_width - sum_fixed_width;
		const float fr_to_px_ratio = available_flex_width / sum_flex_width;

		return fr_to_px_ratio;
	};


	for (bool continue_iteration = true; continue_iteration; )
	{
		continue_iteration = false;
		const float fr_to_px_ratio = calculate_fr_to_px_ratio();

		for (auto& metric : cell_metrics)
		{
			if (metric.flex_width > 0)
			{
				const float fixed_flex_width = metric.flex_width * fr_to_px_ratio;
				metric.fixed_content_width = Math::Clamp(fixed_flex_width, metric.min_content_width, metric.max_content_width);

				if (metric.fixed_content_width != fixed_flex_width)
				{
					// We met a min/max-constraint, fix the size of this column.
					metric.flex_width = 0.0f;
					continue_iteration = true;
				}
			}
		}
	}

	column_widths.resize(cell_metrics.size());

	for (size_t i = 0; i < cell_metrics.size(); i++)
	{
		column_widths[i] = cell_metrics[i].fixed_content_width + cell_metrics[i].padding_border_edges_width;
	}

	// TODO: What to do with any left-over space? Distribute evenly across columns, or make table smaller etc.

	return column_widths;
}

} // namespace Rml
