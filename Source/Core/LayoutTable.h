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

#ifndef RMLUI_CORE_LAYOUTTABLE_H
#define RMLUI_CORE_LAYOUTTABLE_H

#include "LayoutBlockBox.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class Box;

class LayoutTable
{
public:
	/// Formats and positions a table, including all elements contained within.
	/// @param[inout] box The box used for dimensioning the table, the resulting table size is set on the box.
	/// @param[in] min_size Minimum width and height of the table.
	/// @param[in] max_size Maximum width and height of the table.
	/// @param[in] element_table The table element.
	/// @return The content size of the table's overflowing content.
	static Vector2f FormatTable(Box& box, Vector2f min_size, Vector2f max_size, Element* element_table);

private:
	LayoutTable(Element* element_table, Vector2f table_gap, Vector2f table_content_offset, Vector2f table_initial_content_size, Vector2f table_min_size, Vector2f table_max_size);

	struct Column {
		Element* element_column = nullptr; // The '<col>' element which begins at this column, or nullptr if there is no such element or if the column is spanned from a previous column.
		Element* element_group = nullptr;  // The '<colgroup>' element which begins at this column, otherwise nullptr.
		float cell_width = 0;              // The *border* width of cells in this column.
		float cell_offset = 0;             // Horizontal offset from the table content box to the border box of cells in this column.
		float column_width = 0;            // The *content* width of the column element, which may span multiple columns.
		float column_offset = 0;           // Horizontal offset from the table content box to the border box of the column element.
		float group_width = 0;             // The *content* width of the column group element, which may span multiple columns.
		float group_offset = 0;            // Horizontal offset from the table content box to the border box of the column group element.
	};
	using ColumnList = Vector<Column>;

	struct Cell {
		Element* element_cell;         // The <td> element.
		int row_last;                  // The last row the cell spans.
		int column_begin, column_last; // The first and last columns the cell spans.
		Box box;                       // The cell element's resulting sizing box.
		Vector2f table_offset;         // The cell element's offset from the table border box.
	};
	using CellList = Vector<Cell>;

	// Format the table.
	void FormatTable();

	// Determines the column widths and populates the 'columns' data member.
	void DetermineColumnWidths();

	// Formats the table row element and all table cells ending at this row.
	// @return The y-position of the row's bottom edge.
	float FormatTableRow(int row_index, Element* element_row, float row_position_y);

	Element* const element_table;

	const Vector2f table_min_size, table_max_size;
	const Vector2f table_gap;
	const Vector2f table_content_offset;
	const Vector2f table_initial_content_size;

	// The final size of the table which will be determined by the size of its columns, rows, and spacing.
	Vector2f table_resulting_content_size;
	// Overflow size in case the contents of any cells overflow their cell box (without being caught by the cell).
	Vector2f table_content_overflow_size;

	// Defines all the columns of this table, one entry per table column (spanning columns will add multiple entries).
	ColumnList columns;

	// Cells are added during iteration of each table row, and removed once they are placed at the last of the rows they span.
	CellList cells;
};

} // namespace Rml
#endif
