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

#include "LineBox.h"
#include "../../../Include/RmlUi/Core/Element.h"
#include "../../../Include/RmlUi/Core/StyleTypes.h"
#include "InlineBox.h"
#include "InlineLevelBox.h"
#include "LayoutPools.h"
#include <float.h>
#include <numeric>

namespace Rml {

LineBox::~LineBox() {}

void LineBox::SetLineBox(Vector2f _line_position, float _line_width, float _line_minimum_height)
{
	line_position = _line_position;
	line_width = _line_width;
	line_minimum_height = _line_minimum_height;
}

bool LineBox::AddBox(InlineLevelBox* box, InlineLayoutMode layout_mode, LayoutOverflowHandle& inout_overflow_handle)
{
	RMLUI_ASSERT(!is_closed);
	const bool first_box = !HasContent();

	// Find the spacing this element must leave on its right side, to account for its parent inline boxes to be closed later.
	float open_spacing_right = 0.f;
	ForAllOpenFragments([&](const Fragment& fragment) { open_spacing_right += fragment.box->GetSpacingRight(); });

	const float box_placement_cursor = box_cursor + open_spacing_left;

	float available_width = FLT_MAX;
	if (layout_mode != InlineLayoutMode::Nowrap)
	{
		available_width = Math::RoundUp(line_width - box_placement_cursor);
		if (available_width < 0.f)
		{
			if (layout_mode == InlineLayoutMode::WrapAny)
				return true;
			else
				available_width = 0.f;
		}
	}

	FragmentConstructor constructor = box->CreateFragment(layout_mode, available_width, open_spacing_right, first_box, inout_overflow_handle);

	if (constructor.type == FragmentType::Invalid)
	{
		// Could not place fragment on this line, try again on a new line.
		RMLUI_ASSERT(layout_mode == InlineLayoutMode::WrapAny);
		return true;
	}

	const FragmentIndex fragment_index = (FragmentIndex)fragments.size();

	fragments.push_back(Fragment{box, constructor, box->GetVerticalAlign(), box_placement_cursor, open_fragments_leaf});
	fragments.back().aligned_subtree_root = DetermineAlignedSubtreeRoot(fragment_index);

	inout_overflow_handle = {};
	bool continue_on_new_line = false;

	switch (constructor.type)
	{
	case FragmentType::InlineBox:
	{
		RMLUI_ASSERT(constructor.layout_width < 0.f);
		RMLUI_ASSERT(rmlui_static_cast<InlineBox*>(box));

		open_fragments_leaf = fragment_index;
		open_spacing_left += box->GetSpacingLeft();
	}
	break;
	case FragmentType::SizedBox:
	case FragmentType::TextRun:
	{
		RMLUI_ASSERT(constructor.layout_width >= 0.f);

		box_cursor = box_placement_cursor + constructor.layout_width;
		open_spacing_left = 0.f;

		if (constructor.overflow_handle)
		{
			continue_on_new_line = true;
			inout_overflow_handle = constructor.overflow_handle;
		}

		// Mark open fragments as having content so we later know whether we should split or move them in case of overflow.
		ForAllOpenFragments([&](Fragment& fragment) { fragment.has_content = true; });
		has_content = true;
	}
	break;
	case FragmentType::Invalid:
		RMLUI_ERROR; // Handled above;
		break;
	}

	return continue_on_new_line;
}

void LineBox::CloseFragment(Fragment& open_fragment, float right_inner_edge_position)
{
	RMLUI_ASSERT(open_fragment.type == FragmentType::InlineBox);

	open_fragment.children_end_index = (int)fragments.size();
	const float spacing_left = (open_fragment.split_left ? 0.f : open_fragment.box->GetSpacingLeft());
	open_fragment.layout_width = Math::Max(right_inner_edge_position - open_fragment.position.x - spacing_left, 0.f);
}

void LineBox::CloseInlineBox(InlineBox* inline_box)
{
	if (open_fragments_leaf == RootFragmentIndex || fragments[open_fragments_leaf].box != inline_box)
	{
		RMLUI_ERRORMSG("Inline box open/close mismatch.");
		return;
	}

	box_cursor += open_spacing_left;
	open_spacing_left = 0.f;

	Fragment& fragment = fragments[open_fragments_leaf];
	CloseFragment(fragment, box_cursor);
	box_cursor += fragment.box->GetSpacingRight();

	open_fragments_leaf = fragment.parent;
}

UniquePtr<LineBox> LineBox::SplitLine(bool split_all_open_boxes)
{
	if (open_fragments_leaf == RootFragmentIndex)
		return nullptr;

	int num_open_fragments = 0;
	ForAllOpenFragments([&](const Fragment& /*fragment*/) { num_open_fragments += 1; });

	// Move the box cursor to account for any newly opened inline boxes to be closed.
	box_cursor += open_spacing_left;
	open_spacing_left = 0.f;

	// Make a new line with the open fragments.
	auto new_line = MakeUnique<LineBox>();
	new_line->fragments.resize(num_open_fragments);

	// Copy all open fragments to the next line. Do it in reverse order of iteration, since we iterate from back to front.
	FragmentIndex new_index = num_open_fragments;
	ForAllOpenFragments([&](Fragment& old_fragment) {
		new_index -= 1;
		RMLUI_ASSERT((size_t)new_index < new_line->fragments.size() && old_fragment.children_end_index == 0);

		// Copy the old fragment.
		Fragment& new_fragment = new_line->fragments[new_index];
		new_fragment = old_fragment;
		new_fragment.position.x = 0.f;
		new_fragment.parent = new_index - 1;
		new_fragment.aligned_subtree_root = new_line->DetermineAlignedSubtreeRoot(new_index);

		// Only fragments with content placed on the previous line is split, otherwise the fragment is moved down.
		if (new_fragment.has_content || split_all_open_boxes)
		{
			new_fragment.split_left = true;
			new_fragment.has_content = false;

			CloseFragment(old_fragment, box_cursor);
			old_fragment.split_right = true;
		}
		else
		{
			const float spacing_left = (new_fragment.split_left ? 0.f : new_fragment.box->GetSpacingLeft());
			new_line->open_spacing_left += spacing_left;

			// The old fragment is not closed here, which ensures that it will not be placed/submitted on the previous
			// line. Backtrack the box cursor since this fragment is moved down to the next line.
			box_cursor -= spacing_left;
		}
	});

	new_line->open_fragments_leaf = (int)new_line->fragments.size() - 1;
	open_fragments_leaf = RootFragmentIndex;

#ifdef RMLUI_DEBUG
	// Verify integrity of the fragment tree after split.
	for (int i = 0; i < (int)new_line->fragments.size(); i++)
	{
		const Fragment& fragment = new_line->fragments[i];
		RMLUI_ASSERT(fragment.type == FragmentType::InlineBox);
		RMLUI_ASSERT(fragment.parent < i);
		RMLUI_ASSERT(fragment.parent == i - 1);
		RMLUI_ASSERT(fragment.parent == RootFragmentIndex || new_line->fragments[fragment.parent].type == FragmentType::InlineBox);
		RMLUI_ASSERT(
			fragment.aligned_subtree_root == RootFragmentIndex || new_line->IsAlignedSubtreeRoot(new_line->fragments[fragment.aligned_subtree_root]));
		RMLUI_ASSERT(fragment.children_end_index == 0);
	}
#endif

	return new_line;
}

UniquePtr<LineBox> LineBox::DetermineVerticalPositioning(const InlineBoxRoot* root_inline_box, bool split_all_open_boxes, float& out_height_of_line)
{
	RMLUI_ASSERT(!is_closed && !is_vertically_positioned);

	UniquePtr<LineBox> new_line_box = SplitLine(split_all_open_boxes);

	RMLUI_ASSERT(open_fragments_leaf == RootFragmentIndex); // Ensure all open fragments are either closed or split.

	// Vertical alignment and sizing.
	//
	// Aligned subtree CSS definition: "The aligned subtree of an inline element contains that element and the aligned
	// subtrees of all children inline elements whose computed vertical-align value is not 'top' or 'bottom'."
	//
	// We have already determined each box's offset relative to its parent baseline, and its layout size above and below
	// its baseline. Now, for each aligned subtree, place all fragments belonging to that subtree relative to the
	// subtree root baseline. Simultaneously, consider each fragment and keep track of the maximum height above root
	// baseline (max_ascent) and maximum depth below root baseline (max_descent). Their sum is the height of that
	// aligned subtree.
	//
	// Next, treat the root inline box like just another aligned subtree. Then the line box height is first determined
	// by the height of that subtree. Other aligned subtrees might push out that size to make them fit. After the line
	// box size is determined, position each aligned subtree according to its vertical-align, and then position each
	// fragment relative to the aligned subtree they belong to.
	float max_ascent = root_inline_box->GetHeightAboveBaseline();
	float max_descent = root_inline_box->GetDepthBelowBaseline();

	{
		const int subtree_root_index = RootFragmentIndex;
		const int children_end_index = (int)fragments.size();
		VerticallyAlignSubtree(subtree_root_index, children_end_index, max_ascent, max_descent);
	}

	// Find all the aligned subtrees, and vertically align each of them independently.
	for (int i = 0; i < (int)fragments.size(); i++)
	{
		Fragment& fragment = fragments[i];
		if (IsAlignedSubtreeRoot(fragment))
		{
			fragment.max_ascent = fragment.box->GetHeightAboveBaseline();
			fragment.max_descent = fragment.box->GetDepthBelowBaseline();

			if (fragment.type == FragmentType::InlineBox)
			{
				const int subtree_root_index = i;
				VerticallyAlignSubtree(subtree_root_index, fragment.children_end_index, fragment.max_ascent, fragment.max_descent);
			}

			const float subtree_height = fragment.max_ascent + fragment.max_descent;

			// Increase the line box size to fit all line-relative aligned fragments.
			switch (fragment.vertical_align)
			{
			case VerticalAlignType::Top: max_descent = Math::Max(max_descent, subtree_height - max_ascent); break;
			case VerticalAlignType::Bottom: max_ascent = Math::Max(max_ascent, subtree_height - max_descent); break;
			case VerticalAlignType::Center:
			{
				// Distribute the subtree's height equally to the ascent and descent.
				const float distribute_height = 0.5f * (subtree_height - (max_ascent + max_descent));
				if (distribute_height > 0.f)
				{
					max_ascent += distribute_height;
					max_descent += distribute_height;
				}
			}
			break;
			default: RMLUI_ERROR; break;
			}
		}
	}

	// Size the line.
	out_height_of_line = max_ascent + max_descent;
	total_height_above_baseline = max_ascent;

	// Now that the line is sized we can set the vertical position of the fragments.
	for (Fragment& fragment : fragments)
	{
		switch (fragment.vertical_align)
		{
		case VerticalAlignType::Top: fragment.position.y = fragment.max_ascent; break;
		case VerticalAlignType::Bottom: fragment.position.y = out_height_of_line - fragment.max_descent; break;
		case VerticalAlignType::Center: fragment.position.y = 0.5f * (fragment.max_ascent - fragment.max_descent + out_height_of_line); break;
		default:
		{
			RMLUI_ASSERT(!IsAlignedSubtreeRoot(fragment));
			const float aligned_subtree_baseline =
				(fragment.aligned_subtree_root < 0 ? max_ascent : fragments[fragment.aligned_subtree_root].position.y);
			fragment.position.y = aligned_subtree_baseline + fragment.baseline_offset;
		}
		break;
		}
	}

	is_vertically_positioned = true;

	return new_line_box;
}

void LineBox::Close(Element* offset_parent, Vector2f offset_parent_position, Style::TextAlign text_align)
{
	RMLUI_ASSERT(is_vertically_positioned && !is_closed);

	// Horizontal alignment using available space on our line.
	if (box_cursor < line_width)
	{
		switch (text_align)
		{
		case Style::TextAlign::Center: offset_horizontal_alignment = (line_width - box_cursor) * 0.5f; break;
		case Style::TextAlign::Right: offset_horizontal_alignment = (line_width - box_cursor); break;
		case Style::TextAlign::Left:    // Already left-aligned.
		case Style::TextAlign::Justify: // Justification occurs at the text level.
			break;
		}
	}

	// Position and size all inline-level boxes, place geometry boxes.
	for (const Fragment& fragment : fragments)
	{
		// Skip inline boxes which have not been closed (moved down to next line).
		if (fragment.type == FragmentType::InlineBox && fragment.children_end_index == 0)
			continue;

		RMLUI_ASSERT(fragment.layout_width >= 0.f);

		const PlacedFragment placed_fragment = {
			offset_parent,
			fragment.fragment_handle,
			line_position - offset_parent_position + fragment.position + Vector2f(offset_horizontal_alignment, 0.f),
			fragment.layout_width,
			fragment.split_left,
			fragment.split_right,
		};
		fragment.box->Submit(placed_fragment);
	}

	is_closed = true;
}

void LineBox::VerticallyAlignSubtree(const int subtree_root_index, const int children_end_index, float& max_ascent, float& max_descent)
{
	const int children_begin_index = subtree_root_index + 1;

	// Iterate all descendant fragments which belong to our aligned subtree.
	for (int i = children_begin_index; i < children_end_index; i++)
	{
		Fragment& fragment = fragments[i];
		if (fragment.aligned_subtree_root != subtree_root_index)
			continue;

		// Position the baseline of fragments relative to their subtree root.
		const float parent_absolute_baseline = (fragment.parent < 0 ? 0.f : fragments[fragment.parent].baseline_offset);
		fragment.baseline_offset = parent_absolute_baseline + fragment.box->GetVerticalOffsetFromParent();

		// Expand this aligned subtree's height based on the height contributions of its descendants.
		if (fragment.type != FragmentType::TextRun)
		{
			max_ascent = Math::Max(max_ascent, fragment.box->GetHeightAboveBaseline() - fragment.baseline_offset);
			max_descent = Math::Max(max_descent, fragment.box->GetDepthBelowBaseline() + fragment.baseline_offset);
		}
	}
}

InlineBox* LineBox::GetOpenInlineBox()
{
	if (open_fragments_leaf == RootFragmentIndex)
		return nullptr;

	return rmlui_static_cast<InlineBox*>(fragments[open_fragments_leaf].box);
}

bool LineBox::CanCollapseLine() const
{
	// Roughly, collapse lines with only empty text fragments or inline boxes not taking up any width or spacing.
	for (const Fragment& fragment : fragments)
	{
		if (fragment.layout_width > 0.f)
			return false;
		else if (fragment.type == FragmentType::SizedBox)
			return false;
		else if (fragment.type == FragmentType::InlineBox && fragment.children_end_index > 0)
		{
			const bool any_spacing_left = (!fragment.split_left && fragment.box->GetSpacingLeft() != 0.f);
			const bool any_spacing_right = (!fragment.split_right && fragment.box->GetSpacingRight() != 0.f);
			if (any_spacing_left || any_spacing_right)
				return false;
		}
	}
	return true;
}

float LineBox::GetExtentRight() const
{
	RMLUI_ASSERT(is_closed);
	return box_cursor + offset_horizontal_alignment;
}

float LineBox::GetBaseline() const
{
	RMLUI_ASSERT(is_closed);
	return total_height_above_baseline;
}

String LineBox::DebugDumpTree(int depth) const
{
	const String value = String(depth * 2, ' ') + "LineBox (" + ToString(fragments.size()) + " fragment" + (fragments.size() == 1 ? "" : "s") + ")\n";
	return value;
}

void* LineBox::operator new(size_t size)
{
	return LayoutPools::AllocateLayoutChunk(size);
}

void LineBox::operator delete(void* chunk, size_t size)
{
	LayoutPools::DeallocateLayoutChunk(chunk, size);
}

} // namespace Rml
