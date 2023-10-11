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

#include "BlockContainer.h"
#include "../../../Include/RmlUi/Core/ComputedValues.h"
#include "../../../Include/RmlUi/Core/Element.h"
#include "../../../Include/RmlUi/Core/ElementScroll.h"
#include "../../../Include/RmlUi/Core/Profiling.h"
#include "FloatedBoxSpace.h"
#include "InlineContainer.h"
#include "LayoutDetails.h"
#include "LineBox.h"

namespace Rml {

BlockContainer::BlockContainer(ContainerBox* _parent_container, FloatedBoxSpace* _space, Element* _element, const Box& _box, float _min_height,
	float _max_height) :
	ContainerBox(Type::BlockContainer, _element, _parent_container),
	box(_box), min_height(_min_height), max_height(_max_height), space(_space)
{
	RMLUI_ASSERT(element);

	if (!space)
	{
		// We are the root of the formatting context, establish a new space for floated boxes.
		root_space = MakeUnique<FloatedBoxSpace>();
		space = root_space.get();
	}
}

BlockContainer::~BlockContainer() {}

bool BlockContainer::Close(BlockContainer* parent_block_container)
{
	// If the last child of this block box is an inline box, then we haven't closed it; close it now!
	if (!CloseOpenInlineContainer())
		return false;

	// Set this box's height, if necessary.
	if (box.GetSize().y < 0)
	{
		float content_height = box_cursor;

		if (!parent_block_container)
			content_height = Math::Max(content_height, space->GetDimensions(FloatedBoxEdge::Margin).y - (position.y + box.GetPosition().y));

		content_height = Math::Clamp(content_height, min_height, max_height);
		box.SetContent({box.GetSize().x, content_height});
	}

	// Find the size of our content.
	const Vector2f space_box = space->GetDimensions(FloatedBoxEdge::Overflow) - (position + box.GetPosition());
	Vector2f content_box = Math::Max(inner_content_size, space_box);
	content_box.y = Math::Max(content_box.y, box_cursor);

	if (!SubmitBox(content_box, box, max_height))
		return false;

	// If we are the root of our block formatting context, this will be null. Otherwise increment our parent's cursor to account for this box.
	if (parent_block_container)
	{
		RMLUI_ASSERTMSG(GetParent() == parent_block_container, "Mismatched parent box.");

		// If this close fails, it means this block box has caused our parent box to generate an automatic vertical scrollbar.
		if (!parent_block_container->EncloseChildBox(this, position, box.GetSizeAcross(BoxDirection::Vertical, BoxArea::Border),
				box.GetEdge(BoxArea::Margin, BoxEdge::Bottom)))
			return false;
	}

	// Now that we have been sized, we can proceed with formatting and placing positioned elements that this container
	// acts as a containing block for.
	ClosePositionedElements();

	// Find the element baseline which is the distance from the margin bottom of the element to its baseline.
	float element_baseline = 0;

	// For inline-blocks with visible overflow, this is the baseline of the last line of the element (see CSS2 10.8.1).
	if (element->GetDisplay() == Style::Display::InlineBlock && !IsScrollContainer())
	{
		float baseline = 0;
		bool found_baseline = GetBaselineOfLastLine(baseline);

		// The retrieved baseline is the vertical distance from the top of our root space (the coordinate system of our
		// local block formatting context), convert it to the element's local coordinates.
		if (found_baseline)
		{
			const float bottom_position =
				position.y + box.GetSizeAcross(BoxDirection::Vertical, BoxArea::Border) + box.GetEdge(BoxArea::Margin, BoxEdge::Bottom);
			element_baseline = bottom_position - baseline;
		}
	}

	SetElementBaseline(element_baseline);

	EnsureEmptyInterruptedLineBox();

	SubmitElementLayout();

	return true;
}

bool BlockContainer::EncloseChildBox(LayoutBox* child, Vector2f child_position, float child_height, float child_margin_bottom)
{
	child_position -= (box.GetPosition() + position);

	box_cursor = child_position.y + child_height + child_margin_bottom;

	// Extend the inner content size. The vertical size can be larger than the box_cursor due to overflow.
	inner_content_size = Math::Max(inner_content_size, child_position + child->GetVisibleOverflowSize());

	const Vector2f content_size = Math::Max(Vector2f{box.GetSize().x, box_cursor}, inner_content_size);

	const bool result = CatchOverflow(content_size, box, max_height);

	return result;
}

BlockContainer* BlockContainer::OpenBlockBox(Element* child_element, const Box& child_box, float min_height, float max_height)
{
	if (!CloseOpenInlineContainer())
		return nullptr;

	auto child_container_ptr = MakeUnique<BlockContainer>(this, space, child_element, child_box, min_height, max_height);
	BlockContainer* child_container = child_container_ptr.get();

	child_container->position = NextBoxPosition(child_box, child_element->GetComputedValues().clear());
	child_element->SetOffset(child_container->position - position, element);

	child_container->ResetScrollbars(child_box);

	// Store relatively positioned elements with their containing block so that their offset can be updated after
	// their containing block has been sized.
	if (child_element->GetPosition() == Style::Position::Relative)
		AddRelativeElement(child_element);

	child_boxes.push_back(std::move(child_container_ptr));

	return child_container;
}

LayoutBox* BlockContainer::AddBlockLevelBox(UniquePtr<LayoutBox> block_level_box_ptr, Element* child_element, const Box& child_box)
{
	RMLUI_ASSERT(child_box.GetSize().y >= 0.f); // Assumes child element already formatted and sized.

	if (!CloseOpenInlineContainer())
		return nullptr;

	// Clear any floats to avoid overlapping them. In CSS, it is allowed to instead shrink the box and place it next to
	// any floats, but we keep it simple here for now and just clear them.
	Vector2f child_position = NextBoxPosition(child_box, Style::Clear::Both);

	child_element->SetOffset(child_position - position, element);

	if (child_element->GetPosition() == Style::Position::Relative)
		AddRelativeElement(child_element);

	LayoutBox* block_level_box = block_level_box_ptr.get();
	child_boxes.push_back(std::move(block_level_box_ptr));

	if (!EncloseChildBox(block_level_box, child_position, child_box.GetSizeAcross(BoxDirection::Vertical, BoxArea::Border),
			child_box.GetEdge(BoxArea::Margin, BoxEdge::Bottom)))
		return nullptr;

	return block_level_box;
}

InlineBoxHandle BlockContainer::AddInlineElement(Element* element, const Box& child_box)
{
	RMLUI_ZoneScoped;

	// Inline-level elements need to be added to an inline container, open one if needed.
	InlineContainer* inline_container = EnsureOpenInlineContainer();

	InlineBox* inline_box = inline_container->AddInlineElement(element, child_box);

	if (element->GetPosition() == Style::Position::Relative)
		AddRelativeElement(element);

	return {inline_box};
}

void BlockContainer::CloseInlineElement(InlineBoxHandle handle)
{
	// If the inline-level element did not generate an inline box, then there is no need to close anything.
	if (!handle.inline_box)
		return;

	// Usually the inline container the box was placed in is still the open box, and we can just close the inline
	// element in it. However, it is possible that an intermediary block-level element was placed, thereby splitting the
	// inline element into multiple inline containers around the block-level box. If we don't have an open inline
	// container at all, open a new one, even if the sole purpose of the new line is to close this inline element.
	EnsureOpenInlineContainer()->CloseInlineElement(handle.inline_box);
}

void BlockContainer::AddBreak()
{
	const float line_height = element->GetLineHeight();

	// Check for an inline box as our last child; if so, we can simply end its line and bail.
	if (InlineContainer* inline_container = GetOpenInlineContainer())
	{
		inline_container->AddBreak(line_height);
		return;
	}

	// No inline box as our last child; no problem, just increment the cursor by the line height of this element.
	box_cursor += line_height;
}

void BlockContainer::AddFloatElement(Element* element, Vector2f visible_overflow_size)
{
	if (InlineContainer* inline_container = GetOpenInlineContainer())
	{
		// Try to add the float to our inline container, placing it next to any open line if possible. Otherwise, queue it for later.
		bool float_placed = false;
		float line_position_top = 0.f;
		Vector2f line_size;
		if (queued_float_elements.empty() && inline_container->GetOpenLineBoxDimensions(line_position_top, line_size))
		{
			const Vector2f margin_size = element->GetBox().GetSize(BoxArea::Margin);
			const Style::Float float_property = element->GetComputedValues().float_();
			const Style::Clear clear_property = element->GetComputedValues().clear();

			float available_width = 0.f;
			const Vector2f float_position =
				space->NextFloatPosition(this, available_width, line_position_top, margin_size, float_property, clear_property);

			const float line_position_bottom = line_position_top + line_size.y;
			const float line_and_element_width = margin_size.x + line_size.x;

			// If the float can be positioned on the open line, and it can fit next to the line's contents, place it now.
			if (float_position.y < line_position_bottom && line_and_element_width <= available_width)
			{
				PlaceFloat(element, line_position_top, visible_overflow_size);
				inline_container->UpdateOpenLineBoxPlacement();
				float_placed = true;
			}
		}

		if (!float_placed)
			queued_float_elements.push_back({element, visible_overflow_size});
	}
	else
	{
		// There is no inline container, so just place it!
		const Vector2f box_position = NextBoxPosition();
		PlaceFloat(element, box_position.y, visible_overflow_size);
	}

	if (element->GetPosition() == Style::Position::Relative)
		AddRelativeElement(element);
}

Vector2f BlockContainer::GetOpenStaticPosition(Style::Display display) const
{
	// Estimate the next box as if it had static position (10.6.4). If the element is inline-level, position it on the
	// open line if we have one. Otherwise, block-level elements are positioned on a hypothetical next line.
	Vector2f static_position = NextBoxPosition();

	if (const InlineContainer* inline_container = GetOpenInlineContainer())
	{
		const bool inline_level_element = (display == Style::Display::Inline || display == Style::Display::InlineBlock);
		static_position += inline_container->GetStaticPositionEstimate(inline_level_element);
	}

	return static_position;
}

Vector2f BlockContainer::NextBoxPosition() const
{
	Vector2f box_position = position + box.GetPosition();
	box_position.y += box_cursor;
	return box_position;
}

Vector2f BlockContainer::NextBoxPosition(const Box& child_box, Style::Clear clear_property) const
{
	const float child_top_margin = child_box.GetEdge(BoxArea::Margin, BoxEdge::Top);

	Vector2f box_position = NextBoxPosition();

	box_position.x += child_box.GetEdge(BoxArea::Margin, BoxEdge::Left);
	box_position.y += child_top_margin;

	float clear_margin = space->DetermineClearPosition(box_position.y, clear_property) - box_position.y;
	if (clear_margin > 0.f)
	{
		box_position.y += clear_margin;
	}
	else if (const LayoutBox* block_box = GetOpenLayoutBox())
	{
		// Check for a collapsing vertical margin with our last child, which will be vertically adjacent to the new box.
		if (const Box* open_box = block_box->GetIfBox())
		{
			const float open_bottom_margin = open_box->GetEdge(BoxArea::Margin, BoxEdge::Bottom);
			const float margin_sum = open_bottom_margin + child_top_margin;

			// The collapsed margin size depends on the sign of each margin, according to CSS behavior. The margins have
			// already been added to the 'box_position', so subtract their sum as needed.
			const int num_negative_margins = int(child_top_margin < 0.f) + int(open_bottom_margin < 0.f);
			switch (num_negative_margins)
			{
			case 0:
				// Use the largest margin.
				box_position.y += Math::Max(child_top_margin, open_bottom_margin) - margin_sum;
				break;
			case 1:
				// Use the sum of the positive and negative margin. These are already added to the position, so do nothing.
				break;
			case 2:
				// Use the most negative margin.
				box_position.y += Math::Min(child_top_margin, open_bottom_margin) - margin_sum;
				break;
			}
		}
	}

	return box_position;
}

void BlockContainer::PlaceQueuedFloats(float vertical_position)
{
	if (!queued_float_elements.empty())
	{
		for (QueuedFloat entry : queued_float_elements)
			PlaceFloat(entry.element, vertical_position, entry.visible_overflow_size);

		queued_float_elements.clear();
	}
}

float BlockContainer::GetShrinkToFitWidth() const
{
	auto& computed = element->GetComputedValues();

	float content_width = 0.0f;
	if (computed.width().type == Style::Width::Length)
	{
		// We have a definite width, so use that size.
		content_width = box.GetSize().x;
	}
	else
	{
		// Nope, then use the largest outer shrink-to-fit width of our children. Percentage sizing would be relative to
		// our containing block width and is treated just like 'auto' in this context.
		for (const auto& block_box : child_boxes)
		{
			const float child_inner_width = block_box->GetShrinkToFitWidth();
			float child_edges_width = 0.f;
			if (const Box* child_box = block_box->GetIfBox())
				child_edges_width = child_box->GetSizeAcross(BoxDirection::Horizontal, BoxArea::Margin, BoxArea::Padding);

			content_width = Math::Max(content_width, child_edges_width + child_inner_width);
		}
	}

	if (root_space)
	{
		// Since we are the root of the block formatting context, add the width contributions of the floated boxes in
		// our context. The basic algorithm used can produce overestimates, since floats may not be located next to the
		// rest of the content.
		const float edge_left = box.GetPosition().x;
		const float edge_right = edge_left + box.GetSize().x;
		content_width += space->GetShrinkToFitWidth(edge_left, edge_right);
	}

	float min_width, max_width;
	LayoutDetails::GetMinMaxWidth(min_width, max_width, computed, box, 0.f);
	content_width = Math::Clamp(content_width, min_width, max_width);

	return content_width;
}

const Box* BlockContainer::GetIfBox() const
{
	return &box;
}

Element* BlockContainer::GetElement() const
{
	return element;
}

const FloatedBoxSpace* BlockContainer::GetBlockBoxSpace() const
{
	return space;
}

Vector2f BlockContainer::GetPosition() const
{
	return position;
}

Box& BlockContainer::GetBox()
{
	return box;
}

const Box& BlockContainer::GetBox() const
{
	return box;
}

void BlockContainer::ResetContents()
{
	RMLUI_ZoneScopedC(0xDD3322);

	if (root_space)
		root_space->Reset();

	child_boxes.clear();
	queued_float_elements.clear();

	box_cursor = 0;
	interrupted_line_box.reset();

	inner_content_size = {};
}

String BlockContainer::DebugDumpTree(int depth) const
{
	String value = String(depth * 2, ' ') + "BlockContainer" + " | " + LayoutDetails::GetDebugElementName(element) + '\n';

	for (auto&& block_box : child_boxes)
		value += block_box->DumpLayoutTree(depth + 1);

	return value;
}

InlineContainer* BlockContainer::GetOpenInlineContainer()
{
	return const_cast<InlineContainer*>(static_cast<const BlockContainer&>(*this).GetOpenInlineContainer());
}

const InlineContainer* BlockContainer::GetOpenInlineContainer() const
{
	if (!child_boxes.empty() && child_boxes.back()->GetType() == Type::InlineContainer)
		return rmlui_static_cast<InlineContainer*>(child_boxes.back().get());
	return nullptr;
}

InlineContainer* BlockContainer::EnsureOpenInlineContainer()
{
	// First check to see if we already have an open inline container.
	InlineContainer* inline_container = GetOpenInlineContainer();

	// Otherwise, we open a new one.
	if (!inline_container)
	{
		const float scrollbar_width = (IsScrollContainer() ? element->GetElementScroll()->GetScrollbarSize(ElementScroll::VERTICAL) : 0.f);
		const float available_width = box.GetSize().x - scrollbar_width;

		auto inline_container_ptr = MakeUnique<InlineContainer>(this, available_width);
		inline_container = inline_container_ptr.get();
		child_boxes.push_back(std::move(inline_container_ptr));

		if (interrupted_line_box)
		{
			inline_container->AddChainedBox(std::move(interrupted_line_box));
			interrupted_line_box.reset();
		}
	}

	return inline_container;
}

const LayoutBox* BlockContainer::GetOpenLayoutBox() const
{
	if (!child_boxes.empty())
		return child_boxes.back().get();
	return nullptr;
}

bool BlockContainer::CloseOpenInlineContainer()
{
	if (InlineContainer* inline_container = GetOpenInlineContainer())
	{
		EnsureEmptyInterruptedLineBox();

		Vector2f child_position;
		float child_height = 0.f;
		inline_container->Close(&interrupted_line_box, child_position, child_height);

		// Increment our cursor. If this close fails, it means this block container generated an automatic scrollbar.
		if (!EncloseChildBox(inline_container, child_position, child_height, 0.f))
			return false;
	}

	return true;
}

void BlockContainer::EnsureEmptyInterruptedLineBox()
{
	if (interrupted_line_box)
	{
		RMLUI_ERROR; // Internal error: Interrupted line box leaked.
		interrupted_line_box.reset();
	}
}

void BlockContainer::PlaceFloat(Element* element, float vertical_position, Vector2f visible_overflow_size)
{
	const Box& element_box = element->GetBox();

	const Vector2f border_size = element_box.GetSize(BoxArea::Border);
	visible_overflow_size = Math::Max(border_size, visible_overflow_size);

	const Vector2f margin_top_left = {element_box.GetEdge(BoxArea::Margin, BoxEdge::Left), element_box.GetEdge(BoxArea::Margin, BoxEdge::Top)};
	const Vector2f margin_bottom_right = {element_box.GetEdge(BoxArea::Margin, BoxEdge::Right),
		element_box.GetEdge(BoxArea::Margin, BoxEdge::Bottom)};
	const Vector2f margin_size = border_size + margin_top_left + margin_bottom_right;

	Style::Float float_property = element->GetComputedValues().float_();
	Style::Clear clear_property = element->GetComputedValues().clear();

	float unused_box_width = 0.f;
	const Vector2f margin_position = space->NextFloatPosition(this, unused_box_width, vertical_position, margin_size, float_property, clear_property);
	const Vector2f border_position = margin_position + margin_top_left;

	space->PlaceFloat(float_property, margin_position, margin_size, border_position, visible_overflow_size);

	// Shift the offset into this container's space, which acts as the float element's containing block.
	element->SetOffset(border_position - position, GetElement());
}

bool BlockContainer::GetBaselineOfLastLine(float& out_baseline) const
{
	// Return the baseline of our last child that itself has a baseline.
	for (int i = (int)child_boxes.size() - 1; i >= 0; i--)
	{
		if (child_boxes[i]->GetBaselineOfLastLine(out_baseline))
			return true;
	}

	return false;
}

} // namespace Rml
