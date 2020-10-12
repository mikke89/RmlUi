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
#include <float.h>

namespace Rml {


Vector2f LayoutTable::FormatTable(Box& box, Vector2f min_size, Vector2f max_size, Element* element_table)
{
	const ComputedValues& computed_table = element_table->GetComputedValues();

	Vector2f table_content_offset = box.GetPosition();
	Vector2f table_initial_content_size = Vector2f(box.GetSize().x, Math::Max(0.0f, box.GetSize().y));

	// When width or height is set, they act as minimum width or height, just as in CSS.
	if (computed_table.width.type != Style::Width::Auto)
		min_size.x = Math::Max(min_size.x, table_initial_content_size.x);
	if (computed_table.height.type != Style::Height::Auto)
		min_size.y = Math::Max(min_size.y, table_initial_content_size.y);

	Math::SnapToPixelGrid(table_content_offset, table_initial_content_size);

	const Vector2f table_gap = Vector2f(
		ResolveValue(computed_table.column_gap, table_initial_content_size.x), 
		ResolveValue(computed_table.row_gap, table_initial_content_size.y)
	);

	// Construct the layout object and format the table.
	LayoutTable layout_table(element_table, table_gap, table_content_offset, table_initial_content_size, min_size, max_size);

	layout_table.FormatTable();

	// Update the box size based on the new table size.
	box.SetContent(layout_table.table_resulting_content_size);

	return layout_table.table_content_overflow_size;
}


LayoutTable::LayoutTable(Element* element_table, Vector2f table_gap, Vector2f table_content_offset, Vector2f table_initial_content_size, Vector2f table_min_size, Vector2f table_max_size)
	: element_table(element_table), table_min_size(table_min_size), table_max_size(table_max_size), table_gap(table_gap), table_content_offset(table_content_offset), table_initial_content_size(table_initial_content_size)
{
	table_resulting_content_size = table_initial_content_size;
}

void LayoutTable::FormatTable()
{
	DetermineColumnWidths();

	// Now that we have the size of each column, we can move on to formatting the elements.
	// After we format and size an element, we record its height as well, and keep the maximum_height over all cells in the current row.
	// At the end of a row, we then know the height of the row, and we can proceed by positioning the cells.

	float table_cursor_y = table_content_offset.y;

	cells.reserve(columns.size());

	const int num_table_children = element_table->GetNumChildren();

	// Iterate through the table rows and row groups, and format them.
	for (int i = 0, row = -1; i < num_table_children; i++)
	{
		using Display = Style::Display;

		Element* element = element_table->GetChild(i);
		const Display display = element->GetDisplay();

		if (display != Display::TableRow && display != Display::TableRowGroup)
		{
			if (row >= 0 && (display == Display::TableColumn || display == Display::TableColumnGroup))
			{
				Log::Message(Log::LT_WARNING, "Table columns and column groups must precede any table rows. Ignoring element %s.", element->GetAddress().c_str());
			}
			else if (display != Display::TableColumn && display != Display::TableColumnGroup && display != Display::None)
			{
				Log::Message(Log::LT_WARNING, "Only table columns, column groups, rows, and row groups are valid children of tables. Ignoring element %s.", element->GetAddress().c_str());
			}
			continue;
		}

		if (display == Display::TableRow)
		{
			row += 1;
			table_cursor_y = FormatTableRow(row, element, table_cursor_y);
		}
		else if (display == Display::TableRowGroup)
		{
			const int num_row_group_children = element->GetNumChildren();

			Box row_group_box;
			LayoutDetails::BuildBox(row_group_box, table_initial_content_size, element, false, 0.f);

			table_cursor_y += row_group_box.GetEdge(Box::MARGIN, Box::TOP);
			const float pos_border_y = table_cursor_y;

			table_cursor_y += row_group_box.GetEdge(Box::BORDER, Box::TOP) + row_group_box.GetEdge(Box::PADDING, Box::TOP);
			const float pos_content_y = table_cursor_y;

			for (int j = 0; j < num_row_group_children; j++)
			{
				using Display = Style::Display;

				Element* element_row = element->GetChild(j);
				const Display display_row = element_row->GetDisplay();

				if (display_row != Display::TableRow)
				{
					if (display_row != Display::None)
					{
						Log::Message(Log::LT_WARNING, "Only table rows are valid children of table row groups. Ignoring element %s.", element_row->GetAddress().c_str());
					}
					continue;
				}

				row += 1;
				table_cursor_y = FormatTableRow(row, element_row, table_cursor_y);
			}

			// Size and position the row group element
			const Vector2f row_group_element_content_size(
				table_resulting_content_size.x - row_group_box.GetSizeAcross(Box::HORIZONTAL, Box::MARGIN, Box::PADDING),
				Math::Max(row_group_box.GetSize().y, table_cursor_y - pos_content_y)
			);
			row_group_box.SetContent(row_group_element_content_size);

			const Vector2f row_group_offset = Vector2f(table_content_offset.x + row_group_box.GetEdge(Box::MARGIN, Box::LEFT), pos_border_y);

			element->SetOffset(row_group_offset, element_table);
			element->SetBox(row_group_box);

			table_cursor_y += row_group_box.GetEdge(Box::MARGIN, Box::BOTTOM) + row_group_box.GetEdge(Box::BORDER, Box::BOTTOM) + row_group_box.GetEdge(Box::PADDING, Box::BOTTOM);
		}
	}

	table_resulting_content_size.y = Math::Clamp(table_cursor_y - table_content_offset.y, table_min_size.y, table_max_size.y);

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

	for (const Column& column : columns)
	{
		if (column.element_column)
			FormatColumn(column.element_column, column.column_width, column.column_offset);

		if (column.element_group)
			FormatColumn(column.element_group, column.group_width, column.group_offset);
	}

	if (!cells.empty())
	{
		Log::Message(Log::LT_WARNING, "One or more cells span below the last row in table %s. They will not be formatted. Add additional rows, or adjust the rowspan attribute.", element_table->GetAddress().c_str());
	}
}


void LayoutTable::DetermineColumnWidths()
{
	// The column widths are determined entirely by any <col> elements preceding the first row, and <td> elements in the first row.
	// If <col> has a fixed width, that is used. Otherwise, if <td> has a fixed width, that is used. Otherwise the column is 'flexible' width.
	// All flexible widths are then sized to evenly fill the width of the table.
	
	// Both <col> and <colgroup> can have border/padding which extend beyond the size of <td> and <col>, respectively.
	// Margins for <td>, <col>, <colgroup> are merged to produce a single left/right margin for each column, located outside <colgroup>.

	struct ColumnMetric {
		// All widths are defined in terms of the border width of cells in the column.
		bool has_fixed_width = false;
		float fixed_width = 0;
		float flex_width = 0;
		float min_width = 0;
		float max_width = 0;
		// The following are used for <col> elements.
		// If the element spans multiple columns, only the first column it begins at has the element set.
		Element* element_column = nullptr;
		int column_span = 0;
		float column_padding_border_left = 0;
		float column_padding_border_right = 0;
		// The following are used for <colgroup> elements.
		// If the element spans multiple columns, only the first column it begins at has the element set.
		Element* element_group = nullptr;
		int group_span = 0;
		float group_padding_border_left = 0;
		float group_padding_border_right = 0;
		// The margins are the sum of the margins from all <td>, <col>, <colgroup> elements.
		float sum_margin_left = 0;
		float sum_margin_right = 0;
	};

	Vector<ColumnMetric> column_metrics;

	Element* element_row = nullptr;

	const int num_table_children = element_table->GetNumChildren();

	// First, we find any <col> and <colgroup> elements preceding the first table row. For each such element,
	// add the corresponding number of entries to 'column_metrics'.
	for (int i = 0, column_index = 0; i < num_table_children; i++)
	{
		Element* element = element_table->GetChild(i);
		const Style::Display display = element->GetDisplay();

		auto PushGroup = [&column_metrics](Element* element_group, int group_span) {
			column_metrics.emplace_back();
			ColumnMetric& metric = column_metrics.back();
			metric.element_group = element_group;
			metric.group_span = group_span;
			for (int j = 1; j < group_span; j++)
				column_metrics.emplace_back();
		};
		auto PushColumn = [&column_metrics](Element* element_column, int column_span) {
			column_metrics.emplace_back();
			ColumnMetric& metric = column_metrics.back();
			metric.element_column = element_column;
			metric.column_span = column_span;
			for (int j = 1; j < column_span; j++)
				column_metrics.emplace_back();
		};

		if (display == Style::Display::TableColumn)
		{
			const int span = Math::Max(1, element->GetAttribute("span", 1));
			PushColumn(element, span);
			column_index += span;
		}
		else if (display == Style::Display::TableColumnGroup)
		{
			const size_t column_begin = column_metrics.size();
			int group_span = Math::Max(0, element->GetAttribute("span", 0));
			
			if (group_span == 0)
			{
				// Look through the column group to find all its column children.
				const int num_column_group_children = element->GetNumChildren();
				for (int j = 0; j < num_column_group_children; j++)
				{
					Element* child = element->GetChild(j);
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
				PushGroup(element, group_span);
			}

			if (group_span > 0)
			{
				RMLUI_ASSERT(column_begin + size_t(group_span - 1) < column_metrics.size());

				column_metrics[column_begin].element_group = element;
				column_metrics[column_begin].group_span = group_span;
				column_index += group_span;
			}
		}
		else if (display == Style::Display::TableRow)
		{
			// We found the first table row. Any columns found after this are considered invalid, break now.
			element_row = element;
			break;
		}
		else if (display == Style::Display::TableRowGroup)
		{
			// Look through the row group to find the first table row.
			const int num_row_group_children = element->GetNumChildren();
			for (int j = 0; j < num_row_group_children; j++)
			{
				Element* child = element->GetChild(j);
				if (child->GetDisplay() == Style::Display::TableRow)
				{
					element_row = child;
					break;
				}
			}

			// If we found our first visible row, we are done with processing columns. Otherwise, keep looking.
			if (element_row)
				break;
			else
				continue;
		}
	}
	
	auto GetEdgeWidths = [this](float& margin_left, float& margin_right, float& padding_border_left, float& padding_border_right, const ComputedValues& computed)
	{
		margin_left = ResolveValue(computed.margin_left, table_initial_content_size.x);
		margin_right = ResolveValue(computed.margin_right, table_initial_content_size.x);

		padding_border_left = Math::Max(0.0f, ResolveValue(computed.padding_left, table_initial_content_size.x)) + Math::Max(0.0f, computed.border_left_width);
		padding_border_right = Math::Max(0.0f, ResolveValue(computed.padding_right, table_initial_content_size.x)) + Math::Max(0.0f, computed.border_right_width);
	};

	// Fill the column metric with fixed, flexible and min/max widths, based on the element's computed values.
	auto InitializeColumnWidths = [this, GetEdgeWidths](ColumnMetric& metric, float& margin_left, float& margin_right, float& padding_border_left, float& padding_border_right, const ComputedValues& computed, int span, Style::BoxSizing column_target_box)
	{
		RMLUI_ASSERT(span >= 1);

		GetEdgeWidths(margin_left, margin_right, padding_border_left, padding_border_right, computed);

		const float padding_border_sum = padding_border_left + padding_border_right;

		// Find the min/max width.
		metric.min_width = ResolveValue(computed.min_width, table_initial_content_size.x);
		metric.max_width = (computed.max_width.value < 0.f ? FLT_MAX : ResolveValue(computed.max_width, table_initial_content_size.x));

		if (column_target_box == Style::BoxSizing::ContentBox && computed.box_sizing == Style::BoxSizing::BorderBox)
		{
			metric.min_width = Math::Max(0.0f, metric.min_width - padding_border_sum);
			if (metric.max_width < FLT_MAX)
				metric.max_width = Math::Max(0.0f, metric.max_width - padding_border_sum);
		}
		else if (column_target_box == Style::BoxSizing::BorderBox && computed.box_sizing == Style::BoxSizing::ContentBox)
		{
			if (metric.min_width > 0)
				metric.min_width += padding_border_sum;
			if (metric.max_width < FLT_MAX)
				metric.max_width += padding_border_sum;
		}

		// Find fixed and flexible widths.
		if (computed.width.type == Style::Width::Auto)
		{
			metric.flex_width = 1;
		}
		else if (computed.width.type == Style::Width::Percentage && computed.width.value >= 100.f)
		{
			// Percentages >= 100% are resolved as flexible widths.
			metric.flex_width = Math::Max(0.01f * computed.width.value / float(span), 0.f);
		}
		else
		{
			float width = ResolveValue(computed.width, table_initial_content_size.x);

			if (column_target_box == Style::BoxSizing::ContentBox && computed.box_sizing == Style::BoxSizing::BorderBox)
				width = Math::Max(0.f, width - padding_border_sum);
			else if (column_target_box == Style::BoxSizing::BorderBox && computed.box_sizing == Style::BoxSizing::ContentBox)
				width += padding_border_sum;

			metric.flex_width = 0;
			metric.fixed_width = Math::Clamp(width, metric.min_width, metric.max_width);
			metric.min_width = metric.fixed_width;
			metric.max_width = metric.fixed_width;
			metric.has_fixed_width = true;
		}

		if (span > 1)
		{
			// Account for distribution of fixed widths over the columns we are spanning.
			const float width_factor = 1.f / float(span);
			metric.fixed_width *= width_factor;
			metric.min_width *= width_factor;
			if (metric.max_width < FLT_MAX)
				metric.max_width *= width_factor;
		}
	};


	// First look for any <col> and <colgroup> elements preceding any <tr> elements, use them for initializing the respective columns.
	for (int i = 0; i < (int)column_metrics.size(); i++)
	{
		if (Element* element_group = column_metrics[i].element_group)
		{
			// The padding/border/margin of column groups are used, but their widths are ignored.
			const ComputedValues& computed = element_group->GetComputedValues();
			const int span = column_metrics[i].group_span;

			RMLUI_ASSERT(i + span - 1 < (int)column_metrics.size());

			float margin_left, margin_right;
			float padding_border_left, padding_border_right;
			
			GetEdgeWidths(margin_left, margin_right, padding_border_left, padding_border_right, computed);

			// Add left edges.
			ColumnMetric& metric_begin = column_metrics[i];
			metric_begin.group_padding_border_left = padding_border_left;
			metric_begin.sum_margin_left = margin_left;

			// Add right edges.
			ColumnMetric& metric_last = column_metrics[i + span - 1];
			metric_last.group_padding_border_right = padding_border_right;
			metric_last.sum_margin_right = margin_right;

			// Set the width to flexible by default. This may be overrided later by <col> or <td> elements.
			for (int j = 0; j < span; j++)
			{
				ColumnMetric& metric = column_metrics[i + j];
				metric.flex_width = 1;
				metric.max_width = FLT_MAX;
			}
		}

		if (Element* element_column = column_metrics[i].element_column)
		{
			// The padding/border/margin and widths of columns are used.
			const ComputedValues& computed = element_column->GetComputedValues();
			const int span = column_metrics[i].column_span;

			RMLUI_ASSERT(i + span - 1 < (int)column_metrics.size());

			float margin_left, margin_right;
			float padding_border_left, padding_border_right;

			ColumnMetric& metric_begin = column_metrics[i];
			InitializeColumnWidths(metric_begin, margin_left, margin_right, padding_border_left, padding_border_right, computed, span, Style::BoxSizing::ContentBox);

			// Add left edges. Increment the values because the edges may already have been sized from <colgroup>.
			metric_begin.column_padding_border_left += padding_border_left;
			metric_begin.sum_margin_left += margin_left;

			// Add right edges.
			ColumnMetric& metric_last = column_metrics[i + span - 1];
			metric_last.column_padding_border_right += padding_border_right;
			metric_last.sum_margin_right += margin_right;

			// The widths of all spanning columns are distributed equally.
			for (int j = 1; j < span; j++)
			{
				ColumnMetric& metric = column_metrics[i + j];

				metric.has_fixed_width = metric_begin.has_fixed_width;
				metric.fixed_width = metric_begin.fixed_width;
				metric.flex_width = metric_begin.flex_width;
				metric.min_width = metric_begin.min_width;
				metric.max_width = metric_begin.max_width;
			}
		}
	}

	const int num_row_children = (!element_row ? 0 : element_row->GetNumChildren());
	column_metrics.reserve(num_row_children);

	// Next, walk through the cells in the first table row.
	//   - We add new columns here if they are not already represented by a <col> or <colgroup> element.
	//   - Otherwise, we merge the metrics of the cell with the existing column: If the existing column
	//       has auto min-/max-/width, we use the cell's min-/max-/width if it has any.
	// Note: Cells use their border width to line up the column, while <col> use their content width.
	for (int i = 0, column = 0; i < num_row_children; i++)
	{
		Element* element = element_row->GetChild(i);
		const ComputedValues& computed = element->GetComputedValues();

		if (computed.display != Style::Display::TableCell)
			continue;

		const int colspan = Math::Max(1, element->GetAttribute("colspan", 1));

		float margin_left, margin_right;
		float padding_border_left, padding_border_right;
		ColumnMetric column_metric;

		InitializeColumnWidths(column_metric, margin_left, margin_right, padding_border_left, padding_border_right, computed, colspan, Style::BoxSizing::BorderBox);

		column_metric.sum_margin_left = margin_left;

		// Merge with existing columns if they exist, or add new columns.
		for (int j = 0; j < colspan; j++)
		{
			if (j == 1)
				column_metric.sum_margin_left = 0;
			if (j == colspan - 1)
				column_metric.sum_margin_right = margin_right;

			if (j + column < (int)column_metrics.size())
			{
				// Merge the existing column with the cell sizing data.
				ColumnMetric& destination = column_metrics[j + column];

				if (!destination.has_fixed_width && column_metric.has_fixed_width)
				{
					destination.fixed_width = column_metric.fixed_width;
					destination.flex_width = 0;
					destination.has_fixed_width = true;
				}

				if (destination.min_width == 0)
					destination.min_width = column_metric.min_width;

				if (destination.max_width == FLT_MAX)
					destination.max_width = column_metric.max_width;

				destination.sum_margin_left += column_metric.sum_margin_left;
				destination.sum_margin_right += column_metric.sum_margin_right;
			}
			else
			{
				// No existing column, add a new one.
				column_metrics.emplace_back(column_metric);
			}
		}

		column += colspan;
	}


	if (column_metrics.empty())
	{
		// No columns found in this table.
		return;
	}

	// The fixed spacing includes column gaps and the column and column group elements' padding, border, and margins.
	float sum_fixed_spacing = table_gap.x * float((int)column_metrics.size() - 1);

	for (const ColumnMetric& metric : column_metrics)
	{
		sum_fixed_spacing += metric.column_padding_border_left + metric.column_padding_border_right;
		sum_fixed_spacing += metric.group_padding_border_left + metric.group_padding_border_left;
		sum_fixed_spacing += metric.sum_margin_left + metric.sum_margin_right;
	}

	float table_available_width = 0.0f;

	// Now all the widths are determined in terms of fixed or flexible widths.
	// Next, convert any flexible widths to fixed widths by filling up the table width.
	for (bool continue_iteration = true; continue_iteration; )
	{
		continue_iteration = false;
		float fr_to_px_ratio = 0;

		// Calculate the fr/px-ratio. [fr] is here the unit for flexible width.
		{
			float sum_fixed_width = sum_fixed_spacing;  // [px]
			float sum_flex_width = 0;                   // [fr]

			for (const ColumnMetric& metric : column_metrics)
			{
				sum_flex_width += metric.flex_width;
				sum_fixed_width += (metric.flex_width == 0.f ? metric.fixed_width : 0.0f);
			}

			sum_flex_width = Math::Max(1.f, sum_flex_width);

			table_available_width = table_initial_content_size.x - sum_fixed_width;
			fr_to_px_ratio = Math::Max(0.0f, table_available_width) / sum_flex_width;
		}

		// Iterate through the columns and convert flexible widths to fixed widths.
		for (auto& metric : column_metrics)
		{
			if (metric.flex_width > 0)
			{
				const float fixed_flex_width = metric.flex_width * fr_to_px_ratio;
				metric.fixed_width = Math::Clamp(fixed_flex_width, metric.min_width, metric.max_width);
				table_available_width -= metric.fixed_width;

				if (metric.fixed_width != fixed_flex_width)
				{
					// We met a min/max-constraint, fix the size of this column. Start over with the procedure once we are done with all the columns.
					metric.flex_width = 0.0f;
					continue_iteration = true;
				}
			}
		}
	}

	// If we have distributed all the flexible space, and there is still space available, then distribute the available space over
	// the column widths while respecting max-widths.
	if (table_available_width > 0.5f)
	{
		const int num_columns = (int)column_metrics.size();

		struct ColumnAvailableWidth {
			int column;
			float available_width;
		};
		Vector<ColumnAvailableWidth> col_available_widths(num_columns);

		// Find the available width of all columns.
		for (int i = 0; i < num_columns; i++)
		{
			col_available_widths[i].column = i;
			col_available_widths[i].available_width = column_metrics[i].max_width - column_metrics[i].fixed_width;
		}

		// Sort the columns by available width, smallest to largest. This lets us "fill up" the most constrained columns first.
		std::sort(col_available_widths.begin(), col_available_widths.end(), [](const ColumnAvailableWidth& c1, const ColumnAvailableWidth& c2) {
			return c1.available_width < c2.available_width;
		});

		for (int i = 0; i < num_columns; i++)
		{
			const int column = col_available_widths[i].column;
			const int num_columns_remaining = num_columns - i;

			const float ideal_add_column_width = table_available_width / float(num_columns_remaining);
			const float add_column_width = Math::Min(ideal_add_column_width, col_available_widths[i].available_width);

			if (add_column_width > 0)
			{
				column_metrics[column].fixed_width += add_column_width;
				table_available_width = Math::Max(0.0f, table_available_width - add_column_width);
			}
		}
	}

	// Generate the column results based on the metrics.
	columns.resize(column_metrics.size());
	float cursor_x = 0;

	for (size_t i = 0; i < column_metrics.size(); i++)
	{
		Column& col = columns[i];
		const ColumnMetric& metric = column_metrics[i];
		col.element_column = metric.element_column;
		col.element_group = metric.element_group;

		col.group_offset = cursor_x + metric.sum_margin_left;
		col.column_offset = col.group_offset + metric.group_padding_border_left;
		col.cell_offset = col.column_offset + metric.column_padding_border_left;

		// The group and column width will be extended if they span multiple columns (see next loop).
		col.group_width = metric.fixed_width + metric.column_padding_border_left + metric.column_padding_border_right;
		col.column_width = metric.fixed_width; 
		col.cell_width = metric.fixed_width;

		cursor_x = col.cell_offset + metric.fixed_width + metric.column_padding_border_right + metric.group_padding_border_right + metric.sum_margin_right;
		if (i != column_metrics.size() - 1)
			cursor_x += table_gap.x;
	}
	
	// Adjust the table content width based on the accumulated column widths and spacing.
	table_resulting_content_size.x = Math::Clamp(cursor_x, table_min_size.x, table_max_size.x);

	// Extend column and column group widths to cover multiple columns for spanning column (group) elements.
	for (size_t i = 0; i < column_metrics.size(); i++)
	{
		const ColumnMetric& metric = column_metrics[i];

		if (metric.element_column && metric.column_span > 1 && i + metric.column_span - 1 < column_metrics.size())
		{
			Column& col = columns[i];
			Column& col_last_span = columns[i + metric.column_span - 1];
			col.column_width = col_last_span.cell_width + (col_last_span.cell_offset - col.cell_offset);
		}

		if (metric.element_group && metric.group_span > 1 && i + metric.group_span - 1 < column_metrics.size())
		{
			Column& col = columns[i];
			Column& col_last_span = columns[i + metric.group_span - 1];
			col.group_width = col_last_span.group_width + (col_last_span.column_offset - col.column_offset);
		}
	}

	// Finally, snap boxes to the pixel grid.
	for (Column& col : columns)
	{
		Math::SnapToPixelGrid(col.cell_offset, col.cell_width);
		Math::SnapToPixelGrid(col.column_offset, col.column_width);
		Math::SnapToPixelGrid(col.group_offset, col.group_width);
	}
}



float LayoutTable::FormatTableRow(int row_index, Element* element_row, float row_position_y)
{
	// First, determine the row height, then format all cells ending at this row.
	if (row_index > 0)
		row_position_y += table_gap.y;

	const ComputedValues& computed_row = element_row->GetComputedValues();

	Vector2f table_cursor = Vector2f(table_content_offset.x, row_position_y);

	Box row_box;
	float row_min_height, row_max_height;
	LayoutDetails::BuildBox(row_box, table_initial_content_size, element_row, false, 0.f);
	LayoutDetails::GetMinMaxHeight(row_min_height, row_max_height, computed_row, row_box, table_initial_content_size.y);

	// Add the row top spacing to the cursor and row-spanning elements.
	table_cursor.y += row_box.GetEdge(Box::MARGIN, Box::TOP) + row_box.GetEdge(Box::BORDER, Box::TOP) + row_box.GetEdge(Box::PADDING, Box::TOP);

	const int num_cells_spanning_this_row = (int)cells.size();
	const int num_row_children = element_row->GetNumChildren();

	// For all child cell elements of this row, add them to the list of cells and determine their position.
	for (int j = 0, column = 0; j < num_row_children; j++)
	{
		Element* element_cell = element_row->GetChild(j);

		const ComputedValues& computed_cell = element_cell->GetComputedValues();
		if (computed_cell.display != Style::Display::TableCell)
		{
			if (computed_cell.display != Style::Display::None)
				Log::Message(Log::LT_WARNING, "Only table cells are allowed as children of table rows. %s", element_cell->GetAddress().c_str());

			continue;
		}

		const int row_span = Math::Max(1, element_cell->GetAttribute("rowspan", 1));
		const int col_span = Math::Max(1, element_cell->GetAttribute("colspan", 1));

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

		const int column_last = column + col_span - 1;

		if (column_last >= (int)columns.size())
		{
			Log::Message(Log::LT_WARNING, "Too many columns in table row %d: %s\nThe number of columns is %d, as determined by the table columns or the first table row.",
				row_index + 1, element_row->GetAddress().c_str(), (int)columns.size());
			break;
		}

		// Find the horizontal offset for this cell.
		table_cursor.x = table_content_offset.x + columns[column].cell_offset;

		// Add the new cell to our list.
		cells.emplace_back();
		Cell& cell = cells.back();

		cell.row_last = row_index + row_span - 1;
		cell.column_begin = column;
		cell.column_last = column_last;
		cell.element_cell = element_cell;
		cell.table_offset = table_cursor;

		// Determine the cell's box for formatting later, we may get an indefinite (-1) vertical content size.
		Box& box = cell.box;
		LayoutDetails::BuildBox(box, table_initial_content_size, element_cell, false, 0.f);

		// Determine the cell's content width. Include any spanning columns in the cell width.
		const float cell_border_width = columns[column_last].cell_width + (columns[column_last].cell_offset - columns[column].cell_offset);
		const float content_width = Math::Max(0.0f, cell_border_width - box.GetSizeAcross(Box::HORIZONTAL, Box::BORDER, Box::PADDING));
		box.SetContent(Vector2f(content_width, box.GetSize().y));

		column += col_span;
	}

	// Partition the cells to determine those who end at this row.
	const auto it_cells_in_row_end = std::partition(cells.begin(), cells.end(), [row_index](const Cell& cell) { return cell.row_last == row_index; });

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
				LayoutEngine::FormatElement(element_cell, table_initial_content_size, &box);
				box.SetContent(element_cell->GetBox().GetSize());
			}

			const float cell_inrow_height = box.GetSizeAcross(Box::VERTICAL, Box::BORDER) - (table_cursor.y - cell.table_offset.y);
			row_content_height = Math::Max(row_content_height, cell_inrow_height);
		}
	}

	row_content_height = Math::Clamp(row_content_height, row_min_height, row_max_height);
	table_cursor.y += row_content_height;

	// Now we have the height of the row, position and format each cell.

	// Loop through every cell ending at this row.
	for (auto it = cells.begin(); it != it_cells_in_row_end; ++it)
	{
		Cell& cell = *it;
		Element* element_cell = cell.element_cell;
		Box& box = cell.box;
		Style::VerticalAlign vertical_align = cell.element_cell->GetComputedValues().vertical_align;

		const float cell_border_height = table_cursor.y - cell.table_offset.y;

		if (box.GetSize().y < 0)
		{
			const bool is_aligned = (vertical_align.type == Style::VerticalAlign::Middle || vertical_align.type == Style::VerticalAlign::Bottom);
			if (is_aligned)
			{
				// The size of the cell is indefinite, we need to get the height by formatting the cell.
				LayoutEngine::FormatElement(element_cell, table_initial_content_size, &box);
				box.SetContent(element_cell->GetBox().GetSize());
			}
			else
			{
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
		// @performance: We may have already formatted the element during the above procedure without the extra padding. In that case, we may
		//   instead set the new box and offset all descending elements whose offset parent is the cell, to account for the new padding box.
		//   That should be faster than formatting the element again, but there may be edge-cases not accounted for.
		Vector2f cell_visible_overflow_size;
		LayoutEngine::FormatElement(element_cell, table_initial_content_size, &box, &cell_visible_overflow_size);

		// Set the position of the element within the the table container
		element_cell->SetOffset(cell.table_offset, element_table);

		// The cell contents may overflow, propagate this to the table.
		table_content_overflow_size.x = Math::Max(table_content_overflow_size.x, cell.table_offset.x - table_content_offset.x + cell_visible_overflow_size.x);
		table_content_overflow_size.y = Math::Max(table_content_overflow_size.y, cell.table_offset.y - table_content_offset.y + cell_visible_overflow_size.y);
	}

	// Remove the formatted cells from pending
	cells.erase(cells.begin(), it_cells_in_row_end);

	// Size and position the row element
	const Vector2f row_element_content_size(
		table_resulting_content_size.x - row_box.GetSizeAcross(Box::HORIZONTAL, Box::MARGIN, Box::PADDING),
		Math::Max(row_box.GetSize().y, row_content_height)
	);
	row_box.SetContent(row_element_content_size);

	const Vector2f row_element_offset = Vector2f(table_content_offset.x, row_position_y) + Vector2f(row_box.GetEdge(Box::MARGIN, Box::LEFT), row_box.GetEdge(Box::MARGIN, Box::TOP));

	element_row->SetOffset(row_element_offset, element_table);
	element_row->SetBox(row_box);

	// Add the row bottom spacing to the cursor and any row-spanning cells.
	const float row_bottom_spacing = row_box.GetEdge(Box::MARGIN, Box::BOTTOM) + row_box.GetEdge(Box::BORDER, Box::BOTTOM) + row_box.GetEdge(Box::PADDING, Box::BOTTOM);
	table_cursor.y += row_bottom_spacing;

	return table_cursor.y;
}


} // namespace Rml
