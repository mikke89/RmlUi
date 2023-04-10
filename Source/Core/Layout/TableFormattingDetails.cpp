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

#include "TableFormattingDetails.h"
#include "../../../Include/RmlUi/Core/ComputedValues.h"
#include "../../../Include/RmlUi/Core/Element.h"
#include "ContainerBox.h"
#include "LayoutDetails.h"
#include <algorithm>
#include <float.h>

namespace Rml {

bool TableGrid::Build(Element* element_table, TableWrapper& table_wrapper)
{
	RMLUI_ASSERT(rows.empty() && columns.empty() && cells.empty() && open_cells.empty());
	ElementList non_parented_cell_elements;

	const int num_table_children = element_table->GetNumChildren();
	for (int i = 0; i < num_table_children; i++)
	{
		using Display = Style::Display;

		Element* element = element_table->GetChild(i);
		const Display display = element->GetDisplay();

		if (display == Display::None)
			continue;

		if (!non_parented_cell_elements.empty() && display != Display::TableCell)
		{
			PushRow(nullptr, std::move(non_parented_cell_elements), table_wrapper);
			non_parented_cell_elements.clear();
		}

		if (display == Display::TableCell)
		{
			non_parented_cell_elements.push_back(element);
		}
		else if (display == Display::TableRow)
		{
			PushRow(element, {}, table_wrapper);
		}
		else if (display == Display::TableRowGroup)
		{
			const int num_row_group_children = element->GetNumChildren();
			const int row_group_index = (int)rows.size();
			int num_rows_added = 0;

			for (int j = 0; j < num_row_group_children; j++)
			{
				Element* element_row = element->GetChild(j);
				const Display display_row = element_row->GetDisplay();

				if (display_row != Display::TableRow)
				{
					if (display_row != Display::None)
					{
						Log::Message(Log::LT_WARNING, "Only table rows are valid children of table row groups. Ignoring element %s.",
							element_row->GetAddress().c_str());
					}
					continue;
				}

				PushRow(element_row, {}, table_wrapper);
				num_rows_added += 1;
			}

			if (num_rows_added > 0)
			{
				rows[row_group_index].element_group = element;
				rows[row_group_index].group_span = num_rows_added;
			}
			if (element->GetPosition() == Style::Position::Relative)
				table_wrapper.AddRelativeElement(element);
		}
		else if (rows.empty() && display == Display::TableColumn)
		{
			const int span = Math::Max(1, element->GetAttribute("span", 1));
			PushColumn(element, span);
		}
		else if (rows.empty() && display == Display::TableColumnGroup)
		{
			PushColumnGroup(element);
		}
		else
		{
			if (display == Display::TableColumn || display == Display::TableColumnGroup)
				Log::Message(Log::LT_WARNING, "Table columns and column groups must precede any table rows. Ignoring element %s.",
					element->GetAddress().c_str());
			else
				Log::Message(Log::LT_WARNING,
					"Only table columns, column groups, rows, row groups, and cells are valid children of tables. Ignoring element %s.",
					element->GetAddress().c_str());
		}
	}

	if (!non_parented_cell_elements.empty())
	{
		PushRow(nullptr, std::move(non_parented_cell_elements), table_wrapper);
		non_parented_cell_elements.clear();
	}

	// Sort cells by the last row they span.
	std::sort(cells.begin(), cells.end(), [](const Cell& a, const Cell& b) { return a.row_last < b.row_last; });

	if (!open_cells.empty())
	{
		Log::Message(Log::LT_WARNING,
			"One or more cells span below the last row in table %s. They will not be formatted. Add additional rows, or adjust the rowspan "
			"attribute.",
			element_table->GetAddress().c_str());
	}
	open_cells.clear();
	open_cells.shrink_to_fit();

	return true;
}

void TableGrid::PushColumn(Element* element_column, int span)
{
	Column column;
	column.element_column = element_column;
	column.column_span = span;

	columns.push_back(column);

	for (int j = 1; j < span; j++)
		columns.push_back({});
}

void TableGrid::PushColumnGroup(Element* element_column_group)
{
	const int column_begin = (int)columns.size();
	int group_span = Math::Max(0, element_column_group->GetAttribute("span", 0));

	if (group_span == 0)
	{
		// Look through the column group to find all its column children.
		const int num_column_group_children = element_column_group->GetNumChildren();
		for (int j = 0; j < num_column_group_children; j++)
		{
			Element* child = element_column_group->GetChild(j);
			if (child->GetDisplay() == Style::Display::TableColumn)
			{
				const int column_span = Math::Max(1, child->GetAttribute("span", 1));
				PushColumn(child, column_span);
				group_span += column_span;
			}
		}
	}
	else
	{
		// Push empty columns, the group properties are filled below.
		for (int j = 0; j < group_span; j++)
			columns.push_back({});
	}

	if (group_span > 0)
	{
		columns[column_begin].element_group = element_column_group;
		columns[column_begin].group_span = group_span;
	}
}

void TableGrid::PushOrMergeColumnsFromFirstRow(Element* element_cell, int column_begin, int span)
{
	for (int column_index = column_begin; column_index < column_begin + span; column_index++)
	{
		Column* column = nullptr;
		if (column_index < (int)columns.size())
		{
			column = &columns[column_index];
		}
		else
		{
			RMLUI_ASSERT(column_index == (int)columns.size());
			columns.push_back({});
			column = &columns.back();
		}

		if (column_index == column_begin)
		{
			column->element_cell = element_cell;
			column->cell_span = span;
		}
	}
}

void TableGrid::PushRow(Element* element_row, ElementList cell_elements, TableWrapper& table_wrapper)
{
	const int row_index = (int)rows.size();

	if (element_row)
	{
		RMLUI_ASSERT(cell_elements.empty());

		const int num_row_children = element_row->GetNumChildren();
		cell_elements.reserve(num_row_children);

		for (int j = 0; j < num_row_children; j++)
		{
			Element* element_cell = element_row->GetChild(j);

			const Style::Display cell_display = element_cell->GetComputedValues().display();
			if (cell_display == Style::Display::TableCell)
			{
				cell_elements.push_back(element_cell);
			}
			else if (cell_display != Style::Display::None)
			{
				Log::Message(Log::LT_WARNING, "Only table cells are allowed as children of table rows. %s", element_cell->GetAddress().c_str());
			}
		}

		if (element_row->GetPosition() == Style::Position::Relative)
			table_wrapper.AddRelativeElement(element_row);
	}

	rows.push_back(Row{element_row, nullptr, 0});

	const int num_cells_spanning_this_row = (int)open_cells.size();

	// For all child cell elements of this row, add them to the list of open cells.
	for (int j = 0, column = 0; j < (int)cell_elements.size(); j++)
	{
		Element* element_cell = cell_elements[j];

		const int row_span = Math::Max(1, element_cell->GetAttribute("rowspan", 1));
		const int col_span = Math::Max(1, element_cell->GetAttribute("colspan", 1));

		if (row_index == 0)
		{
			// This is the first row. The cells of this row along with previous <col> elements define the columns.
			PushOrMergeColumnsFromFirstRow(element_cell, column, col_span);
		}

		// Offset the column if we have any rowspan elements from previous rows overlapping with the current column.
		for (bool continue_offset_column = true; continue_offset_column;)
		{
			continue_offset_column = false;
			for (int k = 0; k < num_cells_spanning_this_row; k++)
			{
				if (column >= open_cells[k].column_begin && column <= open_cells[k].column_last)
				{
					column = open_cells[k].column_last + 1;
					continue_offset_column = true;
					break;
				}
			}
		}

		const int column_last = column + col_span - 1;

		if (column_last >= (int)columns.size())
		{
			Log::Message(Log::LT_WARNING,
				"Too many columns in table row %d while encountering cell: %s\nThe number of columns is %d, as determined by the table columns or "
				"the first table row.",
				row_index + 1, element_cell->GetAddress().c_str(), (int)columns.size());
			break;
		}

		const Style::Position cell_position = element_cell->GetPosition();
		if (cell_position == Style::Position::Absolute || cell_position == Style::Position::Fixed)
		{
			ContainerBox* containing_box = LayoutDetails::GetContainingBlock(&table_wrapper, cell_position).container;
			containing_box->AddAbsoluteElement(element_cell, {}, table_wrapper.GetElement());
		}
		else
		{
			// Add the new cell to our list.
			open_cells.emplace_back();
			Cell& cell = open_cells.back();

			cell.element_cell = element_cell;
			cell.row_begin = row_index;
			cell.row_last = row_index + row_span - 1;
			cell.column_begin = column;
			cell.column_last = column_last;

			if (cell_position == Style::Position::Relative)
				table_wrapper.AddRelativeElement(element_cell);
		}

		column += col_span;
	}

	// Partition the cells to determine those who end at this row.
	const auto it_cells_in_row_end =
		std::partition(open_cells.begin(), open_cells.end(), [row_index](const Cell& cell) { return cell.row_last == row_index; });

	// Close cells ending at this row.
	cells.insert(cells.end(), open_cells.begin(), it_cells_in_row_end);
	open_cells.erase(open_cells.begin(), it_cells_in_row_end);
}

void TracksSizing::GetEdgeSizes(float& margin_a, float& margin_b, float& padding_border_a, float& padding_border_b,
	const ComputedAxisSize& computed) const
{
	LayoutDetails::GetEdgeSizes(margin_a, margin_b, padding_border_a, padding_border_b, computed, table_initial_content_size);
}

void TracksSizing::ApplyGroupElement(const int index, const int span, const ComputedAxisSize& computed)
{
	RMLUI_ASSERT(span >= 1 && index + span - 1 < (int)metrics.size());

	float margin_a, margin_b;
	float padding_border_a, padding_border_b;

	GetEdgeSizes(margin_a, margin_b, padding_border_a, padding_border_b, computed);

	// Add left/top edges.
	TrackMetric& metric_begin = metrics[index];
	metric_begin.group_padding_border_a = padding_border_a;
	metric_begin.sum_margin_a = margin_a;

	// Add right/bottom edges.
	TrackMetric& metric_last = metrics[index + span - 1];
	metric_last.group_padding_border_b = padding_border_b;
	metric_last.sum_margin_b = margin_b;
}

void TracksSizing::ApplyTrackElement(const int index, const int span, const ComputedAxisSize& computed)
{
	RMLUI_ASSERT(span >= 1 && index + span - 1 < (int)metrics.size());

	float margin_a, margin_b;
	float padding_border_a, padding_border_b;
	TrackMetric& metric_begin = metrics[index];

	// We target the content box because track sizes are defined in terms of the border box of cells, equal to the content size of tracks.
	InitializeSize(metric_begin, margin_a, margin_b, padding_border_a, padding_border_b, computed, span, Style::BoxSizing::ContentBox);

	// Add left/top edges. Increment the values because we are merging with any previously set edges.
	metric_begin.column_padding_border_a += padding_border_a;
	metric_begin.sum_margin_a += margin_a;

	// Add right/bottom edges.
	TrackMetric& metric_last = metrics[index + span - 1];
	metric_last.column_padding_border_b += padding_border_b;
	metric_last.sum_margin_b += margin_b;

	// The size of all spanning tracks are distributed equally.
	for (int j = 1; j < span; j++)
	{
		TrackMetric& metric = metrics[index + j];

		metric.sizing_mode = metric_begin.sizing_mode;
		metric.fixed_size = metric_begin.fixed_size;
		metric.flex_size = metric_begin.flex_size;
		metric.min_size = metric_begin.min_size;
		metric.max_size = metric_begin.max_size;
	}
}

void TracksSizing::ApplyCellElement(const int index, const int span, const ComputedAxisSize& computed)
{
	//  Merge the metrics of the cell with the existing track: If the existing track
	//  has auto min-/max-/size, we use the cell's min-/max-/size if it has any.

	RMLUI_ASSERT(span >= 1 && index + span - 1 < (int)metrics.size());

	float margin_a, margin_b;
	float padding_border_a, padding_border_b;
	TrackMetric cell_metric;

	// We target the border box because track sizes are defined in terms of the border box of cells.
	InitializeSize(cell_metric, margin_a, margin_b, padding_border_a, padding_border_b, computed, span, Style::BoxSizing::BorderBox);

	cell_metric.sum_margin_a = margin_a;

	// Merge the size determined by this cell with any existing track sizes.
	for (int j = 0; j < span; j++)
	{
		if (j == 1)
			cell_metric.sum_margin_a = 0;
		if (j == span - 1)
			cell_metric.sum_margin_b = margin_b;

		// Merge the existing track metrics with the cell sizing data.
		TrackMetric& destination = metrics[index + j];

		if (destination.sizing_mode != TrackSizingMode::Fixed && cell_metric.sizing_mode == TrackSizingMode::Fixed)
		{
			destination.fixed_size = cell_metric.fixed_size;
			destination.flex_size = 0;
			destination.sizing_mode = TrackSizingMode::Fixed;
		}

		if (destination.min_size == 0)
			destination.min_size = cell_metric.min_size;

		if (destination.max_size == FLT_MAX)
			destination.max_size = cell_metric.max_size;

		destination.sum_margin_a += cell_metric.sum_margin_a;
		destination.sum_margin_b += cell_metric.sum_margin_b;
	}
}

void TracksSizing::InitializeSize(TrackMetric& metric, float& margin_a, float& margin_b, float& padding_border_a, float& padding_border_b,
	const ComputedAxisSize& computed, const int span, const Style::BoxSizing target_box) const
{
	RMLUI_ASSERT(span >= 1);

	GetEdgeSizes(margin_a, margin_b, padding_border_a, padding_border_b, computed);

	const float padding_border_sum = padding_border_a + padding_border_b;

	// Find the min/max size.
	metric.min_size = ResolveValue(computed.min_size, table_initial_content_size);
	metric.max_size = ResolveValue(computed.max_size, table_initial_content_size);

	if (target_box == Style::BoxSizing::ContentBox && computed.box_sizing == Style::BoxSizing::BorderBox)
	{
		metric.min_size = Math::Max(0.0f, metric.min_size - padding_border_sum);
		if (metric.max_size < FLT_MAX)
			metric.max_size = Math::Max(0.0f, metric.max_size - padding_border_sum);
	}
	else if (target_box == Style::BoxSizing::BorderBox && computed.box_sizing == Style::BoxSizing::ContentBox)
	{
		if (metric.min_size > 0)
			metric.min_size += padding_border_sum;
		if (metric.max_size < FLT_MAX)
			metric.max_size += padding_border_sum;
	}

	// Find fixed and flexible sizes.
	if (computed.size.type == Style::LengthPercentageAuto::Auto)
	{
		metric.sizing_mode = TrackSizingMode::Auto;
	}
	else if (computed.size.type == Style::LengthPercentageAuto::Percentage && computed.size.value >= 100.f)
	{
		// Percentages >= 100% are resolved as flexible size.
		metric.sizing_mode = TrackSizingMode::Flexible;
		metric.flex_size = Math::Max(0.01f * computed.size.value / float(span), 0.f);
	}
	else
	{
		float width = ResolveValue(computed.size, table_initial_content_size);

		if (target_box == Style::BoxSizing::ContentBox && computed.box_sizing == Style::BoxSizing::BorderBox)
			width = Math::Max(0.f, width - padding_border_sum);
		else if (target_box == Style::BoxSizing::BorderBox && computed.box_sizing == Style::BoxSizing::ContentBox)
			width += padding_border_sum;

		metric.sizing_mode = TrackSizingMode::Fixed;
		metric.flex_size = 0;
		metric.fixed_size = Math::Clamp(width, metric.min_size, metric.max_size);
		metric.min_size = metric.fixed_size;
		metric.max_size = metric.fixed_size;
	}

	if (span > 1)
	{
		// Account for distribution of fixed size over the tracks we are spanning.
		const float width_factor = 1.f / float(span);
		metric.fixed_size *= width_factor;
		metric.min_size *= width_factor;
		if (metric.max_size < FLT_MAX)
			metric.max_size *= width_factor;
	}
}

void TracksSizing::ResolveFlexibleSize()
{
	// The fixed spacing includes the table gaps, and the track and track-group elements' padding, border, and margins.
	float sum_fixed_spacing = table_gap * float((int)metrics.size() - 1);

	for (const TrackMetric& metric : metrics)
	{
		// Any auto size must have been resolved to either fixed or flexible before running this algorithm.
		RMLUI_ASSERT(metric.sizing_mode == TrackSizingMode::Fixed || metric.sizing_mode == TrackSizingMode::Flexible);

		sum_fixed_spacing += metric.column_padding_border_a + metric.column_padding_border_b;
		sum_fixed_spacing += metric.group_padding_border_a + metric.group_padding_border_a;
		sum_fixed_spacing += metric.sum_margin_a + metric.sum_margin_b;
	}

	float table_available_size = 0.0f;

	// Convert any flexible sizes to fixed sizes by filling up the size of the table.
	for (bool continue_iteration = true; continue_iteration;)
	{
		continue_iteration = false;
		float fr_to_px_ratio = 0;

		// Calculate the fr/px-ratio. [fr] is here the unit for flexible width.
		{
			float sum_fixed_size = sum_fixed_spacing; // [px]
			float sum_flex_size = 0;                  // [fr]

			for (const TrackMetric& metric : metrics)
			{
				sum_flex_size += metric.flex_size;
				sum_fixed_size += (metric.flex_size == 0.f ? metric.fixed_size : 0.0f);
			}

			sum_flex_size = Math::Max(1.f, sum_flex_size);

			table_available_size = table_initial_content_size - sum_fixed_size;
			fr_to_px_ratio = Math::Max(0.0f, table_available_size) / sum_flex_size;
		}

		// Iterate through each track and convert flexible size to fixed size.
		for (auto& metric : metrics)
		{
			if (metric.flex_size > 0)
			{
				const float fixed_flex_size = metric.flex_size * fr_to_px_ratio;
				metric.fixed_size = Math::Clamp(fixed_flex_size, metric.min_size, metric.max_size);
				table_available_size -= metric.fixed_size;

				if (metric.fixed_size != fixed_flex_size)
				{
					// We met a min/max-constraint, fix the size of this track. Start over with the procedure once we are done with all the tracks.
					metric.flex_size = 0.0f;
					continue_iteration = true;
				}
			}
		}
	}

	// If we have distributed all the flexible space, and there is still space available, then distribute the available space over
	// the track sizes while respecting max-widths.
	if (table_available_size > 0.5f)
	{
		const int num_tracks = (int)metrics.size();

		struct TrackAvailableSize {
			int track;
			float available_size;
		};
		Vector<TrackAvailableSize> track_available_sizes(num_tracks);

		// Find the available size of all tracks.
		for (int i = 0; i < num_tracks; i++)
		{
			track_available_sizes[i].track = i;
			track_available_sizes[i].available_size = metrics[i].max_size - metrics[i].fixed_size;
		}

		// Sort the tracks by available size, smallest to largest. This lets us "fill up" the most constrained tracks first.
		std::sort(track_available_sizes.begin(), track_available_sizes.end(),
			[](const TrackAvailableSize& c1, const TrackAvailableSize& c2) { return c1.available_size < c2.available_size; });

		for (int i = 0; i < num_tracks; i++)
		{
			const int track = track_available_sizes[i].track;
			const int num_tracks_remaining = num_tracks - i;

			const float ideal_add_track_size = table_available_size / float(num_tracks_remaining);
			const float add_track_size = Math::Min(ideal_add_track_size, track_available_sizes[i].available_size);

			if (add_track_size > 0)
			{
				metrics[track].fixed_size += add_track_size;
				table_available_size = Math::Max(0.0f, table_available_size - add_track_size);
			}
		}
	}
}

static float InitializeTrackBoxes(TrackBoxList& boxes, const TrackMetricList& metrics, const float table_gap)
{
	boxes.resize(metrics.size());

	float cursor = 0;

	// Walk through all the metrics and populate the track box accordingly.
	for (size_t i = 0; i < metrics.size(); i++)
	{
		TrackBox& box = boxes[i];
		const TrackMetric& metric = metrics[i];

		box.group_offset = cursor + metric.sum_margin_a;
		box.track_offset = box.group_offset + metric.group_padding_border_a;
		box.cell_offset = box.track_offset + metric.column_padding_border_a;

		// The group and column width will be extended if they span multiple columns (see next loop).
		box.group_size = metric.fixed_size + metric.column_padding_border_a + metric.column_padding_border_b;
		box.cell_size = metric.fixed_size;
		box.track_size = metric.fixed_size;

		cursor = box.cell_offset + metric.fixed_size + metric.column_padding_border_b + metric.group_padding_border_b + metric.sum_margin_b;
		if (i != metrics.size() - 1)
			cursor += table_gap;
	}

	return cursor;
}

static void SnapTrackBoxesToPixelGrid(TrackBoxList& boxes)
{
	for (TrackBox& box : boxes)
	{
		Math::SnapToPixelGrid(box.cell_offset, box.cell_size);
		Math::SnapToPixelGrid(box.track_offset, box.track_size);
		Math::SnapToPixelGrid(box.group_offset, box.group_size);
	}
}

float BuildColumnBoxes(TrackBoxList& column_boxes, const TrackMetricList& column_metrics, const TableGrid::ColumnList& grid_columns,
	const float table_gap_x)
{
	const float columns_width = InitializeTrackBoxes(column_boxes, column_metrics, table_gap_x);
	const int num_columns = (int)column_metrics.size();

	// Extend column and column group widths to cover all the columns they span.
	for (int i = 0; i < num_columns; i++)
	{
		const int column_span = grid_columns[i].column_span;
		const int group_span = grid_columns[i].group_span;

		if (column_span > 1 && i + column_span - 1 < num_columns)
		{
			TrackBox& metric = column_boxes[i];
			TrackBox& metric_last_span = column_boxes[i + column_span - 1];
			metric.track_size = metric_last_span.cell_size + (metric_last_span.cell_offset - metric.cell_offset);
		}

		if (group_span > 1 && i + group_span - 1 < num_columns)
		{
			TrackBox& metric = column_boxes[i];
			TrackBox& metric_last_span = column_boxes[i + group_span - 1];
			metric.group_size = metric_last_span.group_size + (metric_last_span.track_offset - metric.track_offset);
		}
	}

	SnapTrackBoxesToPixelGrid(column_boxes);

	return columns_width;
}

float BuildRowBoxes(TrackBoxList& row_boxes, const TrackMetricList& row_metrics, const TableGrid::RowList& grid_rows, const float table_gap_y)
{
	const float rows_height = InitializeTrackBoxes(row_boxes, row_metrics, table_gap_y);
	const int num_rows = (int)row_metrics.size();

	// Extend row group heights to cover the all rows they span.
	for (int i = 0; i < num_rows; i++)
	{
		const int group_span = grid_rows[i].group_span;

		if (group_span > 1 && i + group_span - 1 < num_rows)
		{
			TrackBox& metric = row_boxes[i];
			TrackBox& metric_last_span = row_boxes[i + group_span - 1];
			metric.group_size = metric_last_span.group_size + (metric_last_span.track_offset - metric.track_offset);
		}
	}

	SnapTrackBoxesToPixelGrid(row_boxes);

	return rows_height;
}

} // namespace Rml
