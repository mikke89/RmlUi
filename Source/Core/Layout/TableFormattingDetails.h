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

#ifndef RMLUI_CORE_LAYOUT_TABLEFORMATTINGDETAILS_H
#define RMLUI_CORE_LAYOUT_TABLEFORMATTINGDETAILS_H

#include "../../../Include/RmlUi/Core/StyleTypes.h"
#include "../../../Include/RmlUi/Core/Types.h"
#include <float.h>

namespace Rml {

class TableWrapper;
struct ComputedAxisSize;

/*
    TableGrid builds the structure of the table, that is a list of rows, columns, and cells, taking
    spanning attributes into account to position cells.
*/
class TableGrid {
public:
	// Build a list of columns, rows, and cells in this table.
	bool Build(Element* element_table, TableWrapper& table_wrapper);

	struct Column {
		Element* element_column = nullptr; // The '<col>' element which begins at this column, or nullptr if there is no such element or if the column
		                                   // is spanned from a previous column.
		Element* element_group = nullptr;  // The '<colgroup>' element which begins at this column, otherwise nullptr.
		Element* element_cell = nullptr;   // The '<td>' element of the first row which begins at this column, otherwise nullptr.
		int column_span = 0;               // The span of the '<col>' element.
		int group_span = 0;                // The span of the '<colgroup>' element.
		int cell_span = 0;                 // The colspan of the '<td>' element.
	};
	struct Row {
		Element* element_row = nullptr; // The '<tr>' element which begins at this column, or nullptr if there is no such element or if the column is
		                                // spanned from a previous column.
		Element* element_group = nullptr; // The '<tbody>' element which begins at this column, otherwise nullptr.
		int group_span = 0;               // The span of the '<tbody>' element.
	};
	struct Cell {
		Element* element_cell = nullptr;       // The <td> element.
		int row_begin = 0, row_last = 0;       // The first and last rows the cell spans.
		int column_begin = 0, column_last = 0; // The first and last columns the cell spans.
	};

	using ColumnList = Vector<Column>;
	using RowList = Vector<Row>;
	using CellList = Vector<Cell>;

	ColumnList columns;
	RowList rows;
	CellList cells;

private:
	void PushColumn(Element* element_column, int column_span);

	void PushColumnGroup(Element* element_column_group);

	void PushOrMergeColumnsFromFirstRow(Element* element_cell, int column_begin, int span);

	void PushRow(Element* element_row, ElementList cell_elements, TableWrapper& table_wrapper);

	CellList open_cells;
};

enum class TrackSizingMode { Auto, Fixed, Flexible };

/*
    TrackMetric describes the size and the edges of a given track (row or column) in the table.
*/
struct TrackMetric {
	// All sizes are defined in terms of the border size of cells in the row or column.
	TrackSizingMode sizing_mode = TrackSizingMode::Auto;
	float fixed_size = 0;
	float flex_size = 0;
	float min_size = 0;
	float max_size = FLT_MAX;
	// The following are used for row/column elements.
	float column_padding_border_a = 0;
	float column_padding_border_b = 0;
	// The following are used for group elements.
	float group_padding_border_a = 0;
	float group_padding_border_b = 0;
	// The margins are the sum of the margins from all cells, tracks, and group elements.
	float sum_margin_a = 0;
	float sum_margin_b = 0;
};

using TrackMetricList = Vector<TrackMetric>;

/*
    TracksSizing is a helper class for building the track metrics, with methods applicable to both rows and columns sizing.
*/
class TracksSizing {
public:
	TracksSizing(TrackMetricList& metrics, float table_initial_content_size, float table_gap) :
		metrics(metrics), table_initial_content_size(table_initial_content_size), table_gap(table_gap)
	{}

	// Apply group element. This sets the initial size of edges.
	void ApplyGroupElement(const int index, const int span, const ComputedAxisSize& computed);
	// Apply track element. This merges its edges, and sets the initial content size.
	void ApplyTrackElement(const int index, const int span, const ComputedAxisSize& computed);
	// Apply cell element for column sizing. This merges its content size and margins.
	void ApplyCellElement(const int index, const int span, const ComputedAxisSize& computed);

	// Convert flexible size to fixed size for all tracks.
	void ResolveFlexibleSize();

private:
	void GetEdgeSizes(float& margin_a, float& margin_b, float& padding_border_a, float& padding_border_b, const ComputedAxisSize& computed) const;

	// Fill the track metric with fixed, flexible and min/max size, based on the element's computed values.
	void InitializeSize(TrackMetric& metric, float& margin_a, float& margin_b, float& padding_border_a, float& padding_border_b,
		const ComputedAxisSize& computed, const int span, const Style::BoxSizing target_box) const;

	TrackMetricList& metrics;
	const float table_initial_content_size;
	const float table_gap;
};

/*
    TrackBox represents the size and offset of any given track (row or column).
      Rows:    Sizes == Heights. Offsets along vertical axis.
      Columns: Sizes == Widths.  Offsets along horizontal axis.
*/
struct TrackBox {
	float cell_size = 0;    // The *border* size of cells in this track, does not account for spanning cells.
	float cell_offset = 0;  // Offset from the table content box to the border box of cells in this track.
	float track_size = 0;   // The *content* size of the row/column element, which may span multiple tracks.
	float track_offset = 0; // Offset from the table content box to the border box of the row/column element.
	float group_size = 0;   // The *content* size of the group element, which may span multiple tracks.
	float group_offset = 0; // Offset from the table content box to the border box of the group element.
};
using TrackBoxList = Vector<TrackBox>;

// Build a list of column boxes from the provided metrics.
// @return The accumulated width of all columns.
float BuildColumnBoxes(TrackBoxList& column_boxes, const TrackMetricList& column_metrics, const TableGrid::ColumnList& grid_columns,
	float table_gap_x);

// Build a list of row boxes from the provided metrics.
// @return The accumulated height of all rows.
float BuildRowBoxes(TrackBoxList& row_boxes, const TrackMetricList& row_metrics, const TableGrid::RowList& grid_rows, float table_gap_y);

// Return the border size of a cell spanning one or multiple tracks.
inline float GetSpanningCellBorderSize(const TrackBoxList& boxes, const int index, const int index_last_span)
{
	RMLUI_ASSERT(index < (int)boxes.size() && index_last_span < (int)boxes.size());
	return boxes[index_last_span].cell_size + (boxes[index_last_span].cell_offset - boxes[index].cell_offset);
}

} // namespace Rml
#endif
