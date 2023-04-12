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

#ifndef RMLUI_CORE_LAYOUT_INLINECONTAINER_H
#define RMLUI_CORE_LAYOUT_INLINECONTAINER_H

#include "../../../Include/RmlUi/Core/Box.h"
#include "../../../Include/RmlUi/Core/Types.h"
#include "InlineBox.h"
#include "LayoutBox.h"

namespace Rml {

class BlockContainer;
class LineBox;

/**
    A container for inline-level boxes.

    Always a direct child of a block container where it acts as a block-level box, and starts a new inline formatting
    context. It maintains a stack of line boxes in which fragments generated from inline-level boxes are placed within.
    Not a proper CSS term, but effectively a "block container that only contains inline-level boxes".
 */
class InlineContainer final : public LayoutBox {
public:
	/// Creates a new block box in an inline context.
	InlineContainer(BlockContainer* parent, float available_width);
	~InlineContainer();

	/// Adds a new inline-level element to this inline-context box.
	/// @param[in] element The new inline-level element.
	/// @param[in] box The box defining the element's bounds.
	/// @return The inline box if one was generated for the elmeent, otherwise nullptr.
	/// @note Any non-null return value must be closed with a call to CloseInlineElement().
	InlineBox* AddInlineElement(Element* element, const Box& box);

	/// Closes the previously added inline box.
	/// @param[in] inline_box The box to close.
	/// @note Calls to this function should be submitted in reverse order to AddInlineElement().
	void CloseInlineElement(InlineBox* inline_box);

	/// Add a break to the last line.
	void AddBreak(float line_height);
	/// Adds a line box for resuming one that was previously split.
	/// @param[in] open_line_box The line box overflowing from a previous inline container.
	void AddChainedBox(UniquePtr<LineBox> open_line_box);

	/// Closes the box. This will determine the element's height (if it was unspecified).
	/// @param[out] out_open_line_box Optionally, output the open inline box.
	/// @param[out] out_position Outputs the position of this container.
	/// @param[out] out_height Outputs the height of this container.
	void Close(UniquePtr<LineBox>* out_open_line_box, Vector2f& out_position, float& out_height);

	/// Update the placement of the open line box, e.g. to account for newly placed floats.
	void UpdateOpenLineBoxPlacement();

	/// Returns the position and tentative size of the currently open line box, if any.
	bool GetOpenLineBoxDimensions(float& out_vertical_position, Vector2f& out_tentative_size) const;
	/// Returns an estimate for the position of a hypothetical next box to be placed, relative to the content box of this container.
	Vector2f GetStaticPositionEstimate(bool inline_level_box) const;

	// -- Inherited from LayoutBox --

	float GetShrinkToFitWidth() const override;
	bool GetBaselineOfLastLine(float& out_baseline) const override;
	String DebugDumpTree(int depth) const override;

private:
	using LineBoxList = Vector<UniquePtr<LineBox>>;

	LineBox* EnsureOpenLineBox();
	LineBox* GetOpenLineBox() const;
	InlineBoxBase* GetOpenInlineBox();

	/// Close any open line box.
	/// @param[in] split_all_open_boxes Split all open inline boxes, even if they have no content.
	/// @param[out] out_split_line Optionally return any resulting split line, otherwise it will be added as a new line box to this container.
	void CloseOpenLineBox(bool split_all_open_boxes, UniquePtr<LineBox>* out_split_line = nullptr);

	/// Find and set the position and line width for the currently open line box.
	/// @param[in] line_box The currently open line box.
	/// @param[in] minimum_width The minimum line width to consider for this search.
	/// @param[in] minimum_height The minimum line height to to be considered for this and future searches.
	void UpdateLineBoxPlacement(LineBox* line_box, float minimum_width, float minimum_height);

	BlockContainer* parent; // [not-null]

	Vector2f position;
	Vector2f box_size;

	// The element's computed line-height. Not necessarily the same as the height of our lines.
	float element_line_height = 0.f;
	// True if content should wrap instead of overflowing the line box.
	bool wrap_content = false;
	// The element's text-align property.
	Style::TextAlign text_align = {};

	// The vertical position of the currently open line, or otherwise the next one to be placed, relative to the top of this box.
	float box_cursor = 0;

	// The root of the tree of inline boxes located in this inline container.
	InlineBoxRoot root_inline_box;

	// The list of line boxes, each of which flows fragments generated by inline boxes.
	// @performance Investigate using a list backed by the pools instead, to avoid allocations.
	LineBoxList line_boxes;
};

} // namespace Rml
#endif
