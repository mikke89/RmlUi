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

#include "FloatedBoxSpace.h"
#include "../../../Include/RmlUi/Core/ComputedValues.h"
#include "../../../Include/RmlUi/Core/Element.h"
#include "../../../Include/RmlUi/Core/ElementScroll.h"
#include "BlockContainer.h"
#include "LayoutPools.h"
#include <float.h>

namespace Rml {

FloatedBoxSpace::FloatedBoxSpace() {}

FloatedBoxSpace::~FloatedBoxSpace() {}

Vector2f FloatedBoxSpace::NextBoxPosition(const BlockContainer* parent, float& box_width, float cursor, const Vector2f dimensions, bool nowrap) const
{
	return NextBoxPosition(parent, box_width, cursor, dimensions, nowrap, Style::Float::None);
}

Vector2f FloatedBoxSpace::NextFloatPosition(const BlockContainer* parent, float& out_box_width, float cursor, Vector2f dimensions,
	Style::Float float_property, Style::Clear clear_property) const
{
	// Shift the cursor down (if necessary) so it isn't placed any higher than a previously-floated box.
	for (int i = 0; i < NUM_ANCHOR_EDGES; ++i)
	{
		if (!boxes[i].empty())
			cursor = Math::Max(cursor, boxes[i].back().offset.y);
	}

	// Shift the cursor down past to clear boxes, if necessary.
	cursor = DetermineClearPosition(cursor, clear_property);

	// Find a place to put this box.
	const bool nowrap = false;
	const Vector2f margin_offset = NextBoxPosition(parent, out_box_width, cursor, dimensions, nowrap, float_property);

	return margin_offset;
}

void FloatedBoxSpace::PlaceFloat(Style::Float float_property, Vector2f margin_position, Vector2f margin_size, Vector2f overflow_position,
	Vector2f overflow_size)
{
	boxes[float_property == Style::Float::Left ? LEFT : RIGHT].push_back(FloatedBox{margin_position, margin_size});

	// Set our extents so they enclose the new box.
	extent_top_left_overflow = Math::Min(extent_top_left_overflow, overflow_position);
	extent_bottom_right_overflow = Math::Max(extent_bottom_right_overflow, overflow_position + overflow_size);
	extent_bottom_right_margin = Math::Max(extent_bottom_right_margin, margin_position + margin_size);
}

float FloatedBoxSpace::DetermineClearPosition(float cursor, Style::Clear clear_property) const
{
	using namespace Style;
	// Clear left boxes.
	if (clear_property == Clear::Left || clear_property == Clear::Both)
	{
		for (size_t i = 0; i < boxes[LEFT].size(); ++i)
			cursor = Math::Max(cursor, boxes[LEFT][i].offset.y + boxes[LEFT][i].dimensions.y);
	}

	// Clear right boxes.
	if (clear_property == Clear::Right || clear_property == Clear::Both)
	{
		for (size_t i = 0; i < boxes[RIGHT].size(); ++i)
			cursor = Math::Max(cursor, boxes[RIGHT][i].offset.y + boxes[RIGHT][i].dimensions.y);
	}

	return cursor;
}

Vector2f FloatedBoxSpace::NextBoxPosition(const BlockContainer* parent, float& maximum_box_width, const float cursor, const Vector2f dimensions,
	const bool nowrap, const Style::Float float_property) const
{
	const float parent_scrollbar_width = parent->GetElement()->GetElementScroll()->GetScrollbarSize(ElementScroll::VERTICAL);
	const float parent_edge_left = parent->GetPosition().x + parent->GetBox().GetPosition().x;
	const float parent_edge_right = parent_edge_left + parent->GetBox().GetSize().x - parent_scrollbar_width;

	const AnchorEdge box_edge = (float_property == Style::Float::Right ? RIGHT : LEFT);

	Vector2f box_position = {parent_edge_left, cursor};

	if (box_edge == RIGHT)
		box_position.x = parent_edge_right - dimensions.x;

	float next_cursor = FLT_MAX;

	// First up; we iterate through all boxes that share our edge, pushing ourself to the side of them if we intersect
	// them. We record the height of the lowest box that gets in our way; in the event we can't be positioned at this
	// height, we'll reposition ourselves at that height for the next iteration.
	for (const FloatedBox& fixed_box : boxes[box_edge])
	{
		// If the fixed box's bottom edge is above our top edge, then we can safely skip it.
		if (fixed_box.offset.y + fixed_box.dimensions.y <= box_position.y)
			continue;

		// If the fixed box's top edge is below our bottom edge, then we can safely skip it.
		if (fixed_box.offset.y >= box_position.y + dimensions.y)
			continue;

		// We're intersecting this box vertically, so the box is pushed to the side if necessary.
		bool collision = false;
		if (box_edge == LEFT)
		{
			float right_edge = fixed_box.offset.x + fixed_box.dimensions.x;
			collision = box_position.x < right_edge;
			if (collision)
				box_position.x = right_edge;
		}
		else
		{
			collision = box_position.x + dimensions.x > fixed_box.offset.x;
			if (collision)
				box_position.x = fixed_box.offset.x - dimensions.x;
		}

		// If there was a collision, then we *might* want to remember the height of this box if it is the earliest-
		// terminating box we've collided with so far.
		if (collision && !nowrap)
		{
			next_cursor = Math::Min(next_cursor, fixed_box.offset.y + fixed_box.dimensions.y);

			// Were we pushed out of our containing box? If so, try again at the next cursor position.
			if (box_position.x < parent_edge_left || box_position.x + dimensions.x > parent_edge_right)
				return NextBoxPosition(parent, maximum_box_width, next_cursor + 0.01f, dimensions, nowrap, float_property);
		}
	}

	// Second; we go through all of the boxes on the other edge, checking for horizontal collisions and determining the
	// maximum width the box can stretch to, if it is placed at this location.
	maximum_box_width = (box_edge == LEFT ? parent_edge_right - box_position.x : box_position.x + dimensions.x);

	for (const FloatedBox& fixed_box : boxes[1 - box_edge])
	{
		// If the fixed box's bottom edge is above our top edge, then we can safely skip it.
		if (fixed_box.offset.y + fixed_box.dimensions.y <= box_position.y)
			continue;

		// If the fixed box's top edge is below our bottom edge, then we can safely skip it.
		if (fixed_box.offset.y >= box_position.y + dimensions.y)
			continue;

		// We intersect this box vertically, so check if it intersects horizontally.
		bool collision = false;
		if (box_edge == LEFT)
		{
			maximum_box_width = Math::Min(maximum_box_width, fixed_box.offset.x - box_position.x);
			collision = box_position.x + dimensions.x > fixed_box.offset.x;
		}
		else
		{
			maximum_box_width = Math::Min(maximum_box_width, (box_position.x + dimensions.x) - (fixed_box.offset.x + fixed_box.dimensions.x));
			collision = box_position.x < fixed_box.offset.x + fixed_box.dimensions.x;
		}

		// If we collided with this box ... d'oh! We'll try again lower down the page, at the highest bottom-edge of
		// any of the boxes we've been pushed around by so far.
		if (collision && !nowrap)
		{
			next_cursor = Math::Min(next_cursor, fixed_box.offset.y + fixed_box.dimensions.y);
			return NextBoxPosition(parent, maximum_box_width, next_cursor + 0.01f, dimensions, nowrap, float_property);
		}
	}

	// If we are restricted from wrapping the position down, then we are already done now that we've shifted horizontally.
	if (nowrap)
		return box_position;

	// Third; we go through all of the boxes (on both sides), checking for vertical collisions.
	for (int i = 0; i < 2; ++i)
	{
		for (const FloatedBox& fixed_box : boxes[i])
		{
			// If the fixed box's bottom edge is above our top edge, then we can safely skip it.
			if (fixed_box.offset.y + fixed_box.dimensions.y <= box_position.y)
				continue;

			// If the fixed box's top edge is below our bottom edge, then we can safely skip it.
			if (fixed_box.offset.y >= box_position.y + dimensions.y)
				continue;

			// We collide vertically; if we also collide horizontally, then we have to try again further down the
			// layout. If the fixed box's left edge is to right of our right edge, then we can safely skip it.
			if (fixed_box.offset.x >= box_position.x + dimensions.x)
				continue;

			// If the fixed box's right edge is to the left of our left edge, then we can safely skip it.
			if (fixed_box.offset.x + fixed_box.dimensions.x <= box_position.x)
				continue;

			// D'oh! We hit this box. Ah well; we'll try again lower down the page, at the highest bottom-edge of any
			// of the boxes we've been pushed around by so far.
			next_cursor = Math::Min(next_cursor, fixed_box.offset.y + fixed_box.dimensions.y);
			return NextBoxPosition(parent, maximum_box_width, next_cursor + 0.01f, dimensions, nowrap, float_property);
		}
	}

	// Looks like we've found a winner!
	return box_position;
}

Vector2f FloatedBoxSpace::GetDimensions(FloatedBoxEdge edge) const
{
	// For now, we don't really use the top-left extent, because it is not allowed in CSS to scroll to content located
	// to the top or left, and thus we have no use for it currently. We could use it later to help detect overflow on
	// the top-left sides. For example so we can hide parts of floats pushing outside the top-left sides of its parent
	// which is set to 'overflow: auto'.
	return edge == FloatedBoxEdge::Margin ? extent_bottom_right_margin : extent_bottom_right_overflow;
}

float FloatedBoxSpace::GetShrinkToFitWidth(float edge_left, float edge_right) const
{
	// For the left-anchored boxes: Find the right-most edge of the boxes, relative to our parent's left edge.
	float left_shrink_width = 0.f;
	for (const FloatedBox& box : boxes[LEFT])
		left_shrink_width = Math::Max(left_shrink_width, box.offset.x - edge_left + box.dimensions.x);

	// Conversely, for the right-anchored boxes: Find the left-most edge, relative to our parent's right edge.
	float right_shrink_width = 0.f;
	for (const FloatedBox& box : boxes[RIGHT])
		right_shrink_width = Math::Max(right_shrink_width, edge_right - box.offset.x);

	return left_shrink_width + right_shrink_width;
}

void* FloatedBoxSpace::operator new(size_t size)
{
	return LayoutPools::AllocateLayoutChunk(size);
}

void FloatedBoxSpace::operator delete(void* chunk, size_t size)
{
	LayoutPools::DeallocateLayoutChunk(chunk, size);
}

} // namespace Rml
