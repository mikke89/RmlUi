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

#include "LayoutLineBox.h"
#include "LayoutBlockBox.h"
#include "LayoutEngine.h"
#include "LayoutInlineBoxText.h"
#include "../../Include/RmlUi/Core/Property.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../Include/RmlUi/Core/ElementText.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include <stack>

namespace Rml {

static float GetSpacing(const Box& box, Box::Edge edge)
{
	return box.GetEdge(Box::PADDING, edge) +
		   box.GetEdge(Box::BORDER, edge) +
		   box.GetEdge(Box::MARGIN, edge);
}

LayoutLineBox::LayoutLineBox(LayoutBlockBox* _parent) : position(-1, -1), dimensions(-1, -1)
{
	parent = _parent;

	box_cursor = 0;
	open_inline_box = nullptr;

	position_set = false;
	wrap_content = false;
}

LayoutLineBox::~LayoutLineBox()
{
}

// Closes the line box, positioning all inline elements within it.
LayoutInlineBox* LayoutLineBox::Close(UniquePtr<LayoutInlineBox> overflow)
{
	RMLUI_ZoneScoped;

	// If we haven't positioned this line yet, and it has elements in it, then this is a great opportunity to do so.
	if (!position_set &&
		!inline_boxes.empty())
	{
		parent->PositionLineBox(position, dimensions.x, wrap_content, Vector2f(0, 0));
		dimensions.y = 0;

		position_set = true;
	}
	// If the line has been positioned and our content is greater than our original size (for example, if we aren't
	// wrapping or had to render a very long word), then we push our dimensions out to compensate.
	else
		dimensions.x = Math::Max(dimensions.x, box_cursor);

	// Now we calculate the baselines of each of our inline boxes relative to their parent box's baseline; either us,
	// or another of our inline boxes. The maximum distance each element is above and below our baseline is calculated
	// from that, and therefore our height.
	float ascender = 0;
	float descender = 0;
	float minimum_height = 0;

	for (size_t i = 0; i < inline_boxes.size(); ++i)
	{
		LayoutInlineBox* inline_box = inline_boxes[i].get();

		// Check if we've got an element aligned to the line box rather than a baseline.
		if (inline_box->GetVerticalAlignProperty().type == Style::VerticalAlign::Top ||
			inline_box->GetVerticalAlignProperty().type == Style::VerticalAlign::Bottom)
		{
			// Get this element to calculate the baseline offsets of its children; it can't calculate its own baseline
			// because we don't know the height of the line box yet. We don't actually care about its ascender or
			// descender either, just its height.
			float box_ascender, box_descender;
			inline_box->CalculateBaseline(box_ascender, box_descender);

			minimum_height = Math::Max(minimum_height, inline_box->GetHeight());
		}
		// Otherwise, we have an element anchored to a baseline, so we can fetch its ascender and descender relative
		// to our baseline.
		else if (inline_box->GetParent() == nullptr)
		{
			float box_ascender, box_descender;
			inline_box->CalculateBaseline(box_ascender, box_descender);

			ascender = Math::Max(ascender, box_ascender - inline_box->GetPosition().y);
			descender = Math::Max(descender, box_descender + inline_box->GetPosition().y);
		}
	}

	// We've now got the maximum ascender and descender, we can calculate the dimensions of the line box.
	dimensions.y = Math::Max(minimum_height, ascender + descender);
	// And from that, we can now set the final baseline of each box.
	for (size_t i = 0; i < inline_boxes.size(); ++i)
	{
		LayoutInlineBox* inline_box = inline_boxes[i].get();

		// Check again if this element is aligned to the line box. We don't need to worry about offsetting an element
		// tied to the top of the line box, as its position will always stay at exactly 0.
		if (inline_box->GetVerticalAlignProperty().type == Style::VerticalAlign::Top||
			inline_box->GetVerticalAlignProperty().type == Style::VerticalAlign::Bottom)
		{
			if (inline_box->GetVerticalAlignProperty().type == Style::VerticalAlign::Top)
				inline_box->OffsetBaseline(inline_box->GetHeight() - inline_box->GetBaseline());
			else
				inline_box->OffsetBaseline(dimensions.y - inline_box->GetBaseline());
		}
		// Otherwise, this element is tied to a baseline.
		else if (inline_box->GetParent() == nullptr)
			inline_box->OffsetBaseline(ascender);
	}

	// Position all the boxes horizontally in the line. We only need to reposition the elements if they're set to
	// centre or right; the element are already placed left-aligned, and justification occurs at the text level.
	Style::TextAlign text_align_property = parent->GetParent()->GetElement()->GetComputedValues().text_align;
	if (text_align_property == Style::TextAlign::Center ||
		text_align_property == Style::TextAlign::Right)
	{
		float element_offset = 0;
		switch (text_align_property)
		{
			case Style::TextAlign::Center:  element_offset = (dimensions.x - box_cursor) * 0.5f; break;
			case Style::TextAlign::Right:   element_offset = (dimensions.x - box_cursor); break;
			default: break;
		}

		if (element_offset != 0)
		{
			for (size_t i = 0; i < inline_boxes.size(); i++)
				inline_boxes[i]->SetHorizontalPosition(inline_boxes[i]->GetPosition().x + element_offset);
		}
	}

	// Get each line box to set the position of their element, relative to their parents.
	for (int i = (int) inline_boxes.size() - 1; i >= 0; --i)
	{
		inline_boxes[i]->PositionElement();

		// Check if this inline box is part of the open box chain.
		bool inline_box_open = false;
		LayoutInlineBox* open_box = open_inline_box;
		while (open_box != nullptr &&
			   !inline_box_open)
		{
			if (inline_boxes[i].get() == open_box)
				inline_box_open = true;

			open_box = open_box->GetParent();
		}

		inline_boxes[i]->SizeElement(inline_box_open);
	}

	return parent->CloseLineBox(this, std::move(overflow), open_inline_box);
}

// Closes one of the line box's inline boxes.
void LayoutLineBox::CloseInlineBox(LayoutInlineBox* inline_box)
{
	RMLUI_ASSERT(open_inline_box == inline_box);

	open_inline_box = inline_box->GetParent();
	box_cursor += GetSpacing(inline_box->GetBox(), Box::RIGHT);
}

// Attempts to add a new element to this line box.
LayoutInlineBox* LayoutLineBox::AddElement(Element* element, const Box& box)
{
	RMLUI_ZoneScoped;

	ElementText* element_text = rmlui_dynamic_cast<ElementText*>(element);

	if (element_text)
		return AddBox(MakeUnique<LayoutInlineBoxText>(element_text));
	else
		return AddBox(MakeUnique<LayoutInlineBox>(element, box));
}

// Attempts to add a new inline box to this line.
LayoutInlineBox* LayoutLineBox::AddBox(UniquePtr<LayoutInlineBox> box_ptr)
{
	RMLUI_ZoneScoped;

	// Set to true if we're flowing the first box (with content) on the line.
	bool first_box = false;
	// The spacing this element must leave on the right of the line, to account not only for its margins and padding,
	// but also for its parents which will close immediately after it.
	float right_spacing;

	// If this line is unplaced, then this is the first inline box; if it is sized, then we can place and size this
	// line.
	if (!position_set)
	{
		// Add the new box to the list of boxes in the line box. As this line box has not been placed, we don't have to
		// check if it can fit yet.
		LayoutInlineBox* box = AppendBox(std::move(box_ptr));

		// If the new box has a physical prescence, then we must place this line once we've figured out how wide it has to
		// be.
		if (box->GetBox().GetSize().x >= 0)
		{
			// Calculate the dimensions for the box we need to fit.
			Vector2f minimum_dimensions = box->GetBox().GetSize();

			// Add the width of any empty, already closed tags, or still opened spaced tags.
			minimum_dimensions.x += box_cursor;

			// Calculate the right spacing for the element.
			right_spacing = GetSpacing(box->GetBox(), Box::RIGHT);
			// Add the right spacing for any ancestor elements that must close immediately after it.
			LayoutInlineBox* closing_box = box;
			while (closing_box && closing_box->IsLastChild())
			{
				closing_box = closing_box->GetParent();
				if (closing_box)
					right_spacing += GetSpacing(closing_box->GetBox(), Box::RIGHT);
			}

			if (!box->CanOverflow())
				minimum_dimensions.x += right_spacing;

			parent->PositionLineBox(position, dimensions.x, wrap_content, minimum_dimensions);
			dimensions.y = minimum_dimensions.y;

			first_box = true;
			position_set = true;
		}
		else
			return box;
	}

	// This line has already been placed and sized, so we'll check if we can fit this new inline box on the line.
	else
	{
		LayoutInlineBox* box = box_ptr.get();

		// Build up the spacing required on the right side of this element. This consists of the right spacing on the
		// new element, and the right spacing on all parent element that will close next.
		right_spacing = GetSpacing(box->GetBox(), Box::RIGHT);
		if (open_inline_box != nullptr &&
			box->IsLastChild())
		{
			LayoutInlineBox* closing_box = open_inline_box;
			while (closing_box != nullptr &&
				   closing_box->IsLastChild())
			{
				closing_box = closing_box->GetParent();
				if (closing_box != nullptr)
					right_spacing += GetSpacing(closing_box->GetBox(), Box::RIGHT);
			}
		}

		// Determine the inline box's spacing requirements (before we get onto it's actual content width).
		float element_width = box->GetBox().GetPosition(Box::CONTENT).x;
		if (!box->CanOverflow())
			element_width += right_spacing;

		// Add on the box's content area (if it has content).
		if (box->GetBox().GetSize().x >= 0)
			element_width += box->GetBox().GetSize().x;

		if (wrap_content &&
			box_cursor + element_width > dimensions.x)
		{
			// We can't fit the new inline element into this box! So we'll close this line box, and send the inline box
			// onto the next line.
			return Close(std::move(box_ptr));
		}
		else
		{
			// We can fit the new inline element into this box.
			AppendBox(std::move(box_ptr));
		}
	}

	float available_width = -1;
	if (wrap_content)
		available_width = dimensions.x - (open_inline_box->GetPosition().x + open_inline_box->GetBox().GetPosition(Box::CONTENT).x);

	// Flow the box's content into the line.
	UniquePtr<LayoutInlineBox> overflow_box = open_inline_box->FlowContent(first_box, available_width, right_spacing);
	box_cursor += open_inline_box->GetBox().GetSize().x;

	// If our box overflowed, then we'll close this line (as no more content will fit onto it) and tell our block box
	// to make a new line.
	if (overflow_box)
	{
		open_inline_box = open_inline_box->GetParent();
		return Close(std::move(overflow_box));
	}

	return open_inline_box;
}

// Adds an inline box as a chained hierarchy overflowing to this line.
void LayoutLineBox::AddChainedBox(LayoutInlineBox* chained_box)
{
	Stack< LayoutInlineBox* > hierarchy;
	LayoutInlineBox* chain = chained_box;
	while (chain != nullptr)
	{
		hierarchy.push(chain);
		chain = chain->GetParent();
	}

	while (!hierarchy.empty())
	{
		AddBox(MakeUnique<LayoutInlineBox>(hierarchy.top()));
		hierarchy.pop();
	}
}

// Returns the position of the line box, relative to its parent's block box's content area.
const Vector2f& LayoutLineBox::GetPosition() const
{
	return position;
}

// Returns the position of the line box, relative to its parent's block box's offset parent.
Vector2f LayoutLineBox::GetRelativePosition() const
{
	return position - (parent->GetOffsetParent()->GetPosition() - parent->GetOffsetRoot()->GetPosition());
}

// Returns the dimensions of the line box.
const Vector2f& LayoutLineBox::GetDimensions() const
{
	return dimensions;
}

// Returns the line box's open inline box.
LayoutInlineBox* LayoutLineBox::GetOpenInlineBox()
{
	return open_inline_box;
}

// Returns the line's containing block box.
LayoutBlockBox* LayoutLineBox::GetBlockBox()
{
	return parent;
}

float LayoutLineBox::GetBoxCursor() const 
{
	return box_cursor; 
}

bool LayoutLineBox::GetBaselineOfLastLine(float& baseline) const
{
	if (inline_boxes.empty())
		return false;
	baseline = inline_boxes.back()->GetBaseline();
	return true;
}

void* LayoutLineBox::operator new(size_t size)
{
	return LayoutEngine::AllocateLayoutChunk(size);
}

void LayoutLineBox::operator delete(void* chunk, size_t size)
{
	LayoutEngine::DeallocateLayoutChunk(chunk, size);
}

// Appends an inline box to the end of the line box's list of inline boxes.
LayoutInlineBox* LayoutLineBox::AppendBox(UniquePtr<LayoutInlineBox> box_ptr)
{
	LayoutInlineBox* box = box_ptr.get();
	inline_boxes.push_back(std::move(box_ptr));

	box->SetParent(open_inline_box);
	box->SetLine(this);
	box->SetHorizontalPosition(box_cursor + box->GetBox().GetEdge(Box::MARGIN, Box::LEFT));
	box_cursor += GetSpacing(box->GetBox(), Box::LEFT);

	open_inline_box = box;
	return box;
}

} // namespace Rml
