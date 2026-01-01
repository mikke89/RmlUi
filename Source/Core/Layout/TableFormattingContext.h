#pragma once

#include "../../../Include/RmlUi/Core/Types.h"
#include "FormattingContext.h"
#include "TableFormattingDetails.h"

namespace Rml {

class Box;
class TableGrid;
class TableWrapper;
struct TrackBox;
using TrackBoxList = Vector<TrackBox>;

/*
    Formats a table element and its parts according to table layout rules.
*/
class TableFormattingContext final : public FormattingContext {
public:
	static UniquePtr<LayoutBox> Format(ContainerBox* parent_container, Element* element, const Box* override_initial_box);

private:
	TableFormattingContext() = default;

	using BoxList = Vector<Box>;

	/// Format the table and its children.
	/// @param[out] table_content_size The final size of the table which will be determined by the size of its columns, rows, and spacing.
	/// @param[out] table_overflow_size Overflow size in case the contents of any cells overflow their cell box (without being caught by the cell).
	/// @param[out] table_baseline The baseline of the table wrapper, in terms of the vertical distance from its top-left border corner.
	/// @note Expects the table grid to have been built, and all table parameters to be set already.
	void FormatTable(Vector2f& table_content_size, Vector2f& table_overflow_size, float& table_baseline) const;

	// Determines the column widths, and populates the columns.
	void DetermineColumnWidths(TrackBoxList& columns, float& table_content_width) const;

	// Generate the initial boxes for all cells, content height may be indeterminate for now (-1).
	void InitializeCellBoxes(BoxList& cells, const TrackBoxList& columns) const;

	// Determines the row heights, and populates the rows.
	void DetermineRowHeights(TrackBoxList& rows, BoxList& cells, float& table_content_height) const;

	// Format the table row and row group elements.
	void FormatRows(const TrackBoxList& rows, float table_content_width) const;

	// Format the table row and row group elements.
	void FormatColumns(const TrackBoxList& columns, float table_content_height) const;

	// Format the table cell elements.
	void FormatCells(BoxList& cells, Vector2f& table_overflow_size, const TrackBoxList& rows, const TrackBoxList& columns,
		float& table_baseline) const;

	Element* element_table = nullptr;
	TableWrapper* table_wrapper_box = nullptr;

	TableGrid grid;

	bool table_auto_height = false;
	Vector2f table_min_size, table_max_size;
	Vector2f table_gap;
	Vector2f table_content_offset;
	Vector2f table_initial_content_size;
};

} // namespace Rml
