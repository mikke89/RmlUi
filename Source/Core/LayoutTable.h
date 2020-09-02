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
	using CloseResult = LayoutBlockBox::CloseResult;

	/// Formats and positions a table, including all table-rows and table-cells contained within.
	/// @param[in] block_context_box The open block box to layout the element in.
	/// @param[in] element The table element.
	static CloseResult FormatTable(LayoutBlockBox* table_block_context_box, Element* element_table);


private:
	struct Column {
		Element* element_column = nullptr; // The '<col>' element which defines this column, or nullptr if the column is spanned from a previous column or defined by cells in the first row.
		float cell_width = 0;              // The *border* width of cells in this column.
		float cell_offset = 0;             // Horizontal offset from the table content box to the border box of cells in this column.
		float column_width = 0;            // The *content* width of the column element, which may span multiple columns.
		float column_offset = 0;           // Horizontal offset from the table content box to the border box of the column element.
	};
	using Columns = Vector<Column>;

	static Columns DetermineColumnWidths(Element* element_table, float column_gap, float& table_content_width);
};

} // namespace Rml
#endif
