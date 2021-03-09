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

#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class Box;
class TableGrid;
struct TrackBox;
using TrackBoxList = Vector<TrackBox>;


class LayoutTable {
public:
	/// Formats and positions a table, including all elements contained within.
	/// @param[inout] box The box used for dimensioning the table, the resulting table size is set on the box.
	/// @param[in] min_size Minimum width and height of the table.
	/// @param[in] max_size Maximum width and height of the table.
	/// @param[in] element_table The table element.
	/// @return The content size of the table's overflowing content.
	static Vector2f FormatTable(Box& box, Vector2f min_size, Vector2f max_size, Element* element_table);

private:
	LayoutTable(Element* element_table, const TableGrid& grid, Vector2f table_gap, Vector2f table_content_offset,
		Vector2f table_initial_content_size, bool table_auto_height, Vector2f table_min_size, Vector2f table_max_size);

	// Format the table.
	void FormatTable();

	// Determines the column widths and populates the 'columns' data member.
	void DetermineColumnWidths();

	// Generate the initial boxes for all cells, content height may be indeterminate for now (-1).
	void InitializeCellBoxes();

	// Determines the row heights and populates the 'rows' data member.
	void DetermineRowHeights();

	// Format the table row and row group elements.
	void FormatRows();

	// Format the table row and row group elements.
	void FormatColumns();

	// Format the table cell elements.
	void FormatCells();

	Element* const element_table;

	const TableGrid& grid;

	const bool table_auto_height;
	const Vector2f table_min_size, table_max_size;
	const Vector2f table_gap;
	const Vector2f table_content_offset;
	const Vector2f table_initial_content_size;

	// The final size of the table which will be determined by the size of its columns, rows, and spacing.
	Vector2f table_resulting_content_size;
	// Overflow size in case the contents of any cells overflow their cell box (without being caught by the cell).
	Vector2f table_content_overflow_size;

	// Defines the boxes for all columns in this table, one entry per table column (spanning columns will add multiple entries).
	TrackBoxList columns;

	// Defines the boxes for all rows in this table, one entry per table row.
	TrackBoxList rows;

	// Defines the boxes for all cells in this table.
	using BoxList = Vector<Box>;
	BoxList cells;
};

} // namespace Rml
#endif
