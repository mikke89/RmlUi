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

#ifndef RMLUI_CORE_LAYOUT_LINEBOX_H
#define RMLUI_CORE_LAYOUT_LINEBOX_H

#include "../../../Include/RmlUi/Core/StyleTypes.h"
#include "InlineTypes.h"

namespace Rml {

class InlineBox;
class InlineBoxRoot;
class InlineLevelBox;

/*
    Horizontally places fragments generated from inline-level boxes.

    Inline boxes can nest other inline-level boxes, thereby creating a tree of inline-level boxes. This tree structure
    needs to be considered for the generated fragments as well, since their size and position depend on this tree
    structure.

    Note that a single inline-level box can generate multiple fragments, possibly placed in different line boxes.
*/
class LineBox final {
public:
	LineBox() = default;
	~LineBox();

	// Set the line box position and dimensions.
	void SetLineBox(Vector2f line_position, float line_width, float line_minimum_height);

	/// Generates a fragment from a box and places it on this line, if possible.
	/// @param[in] box The inline-level box to be placed.
	/// @param[in] layout_mode The inline layout mode, which affects the fragment generation.
	/// @param[in,out] inout_overflow_handle A handle to resume fragment generation from a partially placed box.
	/// @return True if the box should be placed again on a new line, either because the box could not fit or there is more content to be placed.
	bool AddBox(InlineLevelBox* box, InlineLayoutMode layout_mode, LayoutOverflowHandle& inout_overflow_handle);

	/// Closes the open inline box.
	/// @param[in] inline_box The inline box to be closed. Should match the currently open box, strictly used for verification.
	/// @note Only inline boxes need to be closed, not other inline-level boxes since they do not contain child boxes.
	void CloseInlineBox(InlineBox* inline_box);

	/// Vertically positions each fragment and sizes the line, after splitting any open inline boxes to be placed on a new line.
	/// @param[in] root_inline_box The root inline box of our inline container.
	/// @param[in] split_all_open_boxes Split all open inline boxes, even if they have no content.
	/// @param[out] out_height_of_line Resulting height of line. This can be different from the element's computed line-height property.
	/// @return The next line if any open fragments had to be split or wrapped down.
	UniquePtr<LineBox> DetermineVerticalPositioning(const InlineBoxRoot* root_inline_box, bool split_all_open_boxes, float& out_height_of_line);

	// Closes the line and submits all fragments. Thereby positioning, sizing, and placing their corresponding boxes.
	// @note The line must have been vertically positioned before closing.
	void Close(Element* offset_parent, Vector2f offset_parent_position, Style::TextAlign text_align);

	float GetBoxCursor() const { return box_cursor; }
	Vector2f GetPosition() const { return line_position; }
	float GetLineWidth() const { return line_width; }
	float GetLineMinimumHeight() const { return line_minimum_height; }

	InlineBox* GetOpenInlineBox();

	bool IsClosed() const { return is_closed; }
	bool HasContent() const { return has_content; }

	// Returns true if this line should be collapsed, such that it takes up no height in the inline container.
	bool CanCollapseLine() const;

	// Returns the width of the contents of the line, relative to the left edge of the line box. Includes spacing due to horizontal alignment.
	// @note Only available after line has been closed.
	float GetExtentRight() const;

	// Returns the baseline position, relative to the top of the line box.
	// @note Only available after line has been closed.
	float GetBaseline() const;

	String DebugDumpTree(int depth) const;

	void* operator new(size_t size);
	void operator delete(void* chunk, size_t size);

private:
	using FragmentIndex = int;
	using VerticalAlignType = Style::VerticalAlign::Type;

	static constexpr FragmentIndex RootFragmentIndex = -1;

	struct Fragment {
		Fragment() = default;
		Fragment(InlineLevelBox* box, FragmentConstructor constructor, VerticalAlignType vertical_align, float position_x, FragmentIndex parent) :
			box(box), type(constructor.type), fragment_handle(constructor.fragment_handle), vertical_align(vertical_align), position(position_x, 0.f),
			layout_width(constructor.layout_width), parent(parent)
		{}

		InlineLevelBox* box = nullptr;
		FragmentType type = FragmentType::Invalid;
		LayoutFragmentHandle fragment_handle = {};
		VerticalAlignType vertical_align = {};

		// Layout state.
		Vector2f position;        // Position relative to start of the line, disregarding floats, (x: outer-left edge, y: baseline).
		float layout_width = 0.f; // Inner width for inline boxes, otherwise outer width.

		// Vertical alignment state.
		FragmentIndex parent = RootFragmentIndex;
		FragmentIndex aligned_subtree_root = RootFragmentIndex; // Index of the aligned subtree the fragment belongs to.
		float baseline_offset = 0.f;                            // Vertical offset from aligned subtree root baseline to our baseline.

		// For inline boxes.
		bool split_left = false;
		bool split_right = false;
		bool has_content = false;
		FragmentIndex children_end_index = 0; // One-past-last-child of this box, as index into fragment list.

		// For aligned subtree root.
		float max_ascent = 0.f;
		float max_descent = 0.f;
	};
	using FragmentList = Vector<Fragment>;

	// Place an open fragment.
	void CloseFragment(Fragment& open_fragment, float right_inner_edge_position);

	// Splits the line, returning a new line if there are any open fragments.
	UniquePtr<LineBox> SplitLine(bool split_all_open_boxes);

	// Vertically align all descendants of the subtree. Returns the ascent of the top-most box, and descent of the bottom-most box.
	void VerticallyAlignSubtree(int subtree_root_index, int children_end_index, float& max_ascent, float& max_descent);

	// Returns true if the fragment establishes an aligned subtree.
	static bool IsAlignedSubtreeRoot(const Fragment& fragment)
	{
		return (fragment.vertical_align == VerticalAlignType::Top || fragment.vertical_align == VerticalAlignType::Center ||
			fragment.vertical_align == VerticalAlignType::Bottom);
	}
	// Returns the aligned subtree root for a given fragment, based on its ancestors.
	FragmentIndex DetermineAlignedSubtreeRoot(FragmentIndex index) const
	{
		while (index != RootFragmentIndex)
		{
			const Fragment& fragment = fragments[index];
			if (IsAlignedSubtreeRoot(fragment))
				return index;
			index = fragment.parent;
		}
		return index;
	}

	template <typename Func>
	void ForAllOpenFragments(Func&& func)
	{
		FragmentIndex index = open_fragments_leaf;
		while (index != RootFragmentIndex)
		{
			Fragment& fragment = fragments[index];
			index = fragment.parent;
			func(fragment);
		}
	}

	// Position of the line, relative to our block formatting context root.
	Vector2f line_position;
	// Available space for the line. Based on our parent box content width, possibly shrinked due to floating boxes.
	float line_width = 0.f;
	// Lower-bound estimate for the line box height.
	float line_minimum_height = 0.f;

	// The horizontal cursor. This is the outer-right position of the last placed fragment.
	float box_cursor = 0.f;
	// The contribution of opened inline boxes to the placement of the next fragment, due to their left edges (margin-border-padding).
	float open_spacing_left = 0.f;

	// List of fragments in this line box.
	FragmentList fragments;

	// Index of the last open fragment. The list of open fragments is this leaf and all of its ancestors up to the root.
	FragmentIndex open_fragments_leaf = RootFragmentIndex;

	bool has_content = false;
	bool is_vertically_positioned = false;
	bool is_closed = false;

	// Content offset due to space distribution from 'text-align'. Available after close.
	float offset_horizontal_alignment = 0.f;
	// The line box's height above baseline. Available after close.
	float total_height_above_baseline = 0.f;
};

} // namespace Rml
#endif
