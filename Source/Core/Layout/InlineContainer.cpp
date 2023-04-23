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

#include "InlineContainer.h"
#include "../../../Include/RmlUi/Core/ComputedValues.h"
#include "../../../Include/RmlUi/Core/Element.h"
#include "../../../Include/RmlUi/Core/ElementScroll.h"
#include "../../../Include/RmlUi/Core/ElementText.h"
#include "../../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../../Include/RmlUi/Core/Profiling.h"
#include "../../../Include/RmlUi/Core/Property.h"
#include "BlockContainer.h"
#include "FloatedBoxSpace.h"
#include "InlineLevelBox.h"
#include "LayoutDetails.h"
#include "LineBox.h"

namespace Rml {

InlineContainer::InlineContainer(BlockContainer* _parent, float _available_width) :
	LayoutBox(Type::InlineContainer), parent(_parent), root_inline_box(_parent->GetElement())
{
	RMLUI_ASSERT(_parent);

	box_size = {_available_width, -1.f};
	position = parent->NextBoxPosition();

	const auto& computed = parent->GetElement()->GetComputedValues();
	element_line_height = computed.line_height().value;
	wrap_content = (computed.white_space() != Style::WhiteSpace::Nowrap);
	text_align = computed.text_align();
}

InlineContainer::~InlineContainer() {}

InlineBox* InlineContainer::AddInlineElement(Element* element, const Box& box)
{
	RMLUI_ASSERT(element);

	InlineBox* inline_box = nullptr;
	InlineLevelBox* inline_level_box = nullptr;
	InlineBoxBase* parent_box = GetOpenInlineBox();

	if (auto text_element = rmlui_dynamic_cast<ElementText*>(element))
	{
		inline_level_box = parent_box->AddChild(MakeUnique<InlineLevelBox_Text>(text_element));
	}
	else if (box.GetSize().x >= 0.f)
	{
		inline_level_box = parent_box->AddChild(MakeUnique<InlineLevelBox_Atomic>(parent_box, element, box));
	}
	else
	{
		auto inline_box_ptr = MakeUnique<InlineBox>(parent_box, element, box);
		inline_box = inline_box_ptr.get();
		inline_level_box = parent_box->AddChild(std::move(inline_box_ptr));
	}

	const float minimum_line_height =
		Math::Max(element_line_height, (box.GetSize().y >= 0.f ? box.GetSizeAcross(BoxDirection::Vertical, BoxArea::Margin) : 0.f));

	LayoutOverflowHandle overflow_handle = {};
	float minimum_width_next = 0.f;

	while (true)
	{
		LineBox* line_box = EnsureOpenLineBox();

		UpdateLineBoxPlacement(line_box, minimum_width_next, minimum_line_height);

		InlineLayoutMode layout_mode = InlineLayoutMode::Nowrap;
		if (wrap_content)
		{
			const bool line_shrinked_by_floats = (line_box->GetLineWidth() + 0.5f < box_size.x && minimum_width_next < box_size.x);
			const bool can_wrap_any = (line_shrinked_by_floats || line_box->HasContent());
			layout_mode = (can_wrap_any ? InlineLayoutMode::WrapAny : InlineLayoutMode::WrapAfterContent);
		}

		const bool add_new_line = line_box->AddBox(inline_level_box, layout_mode, overflow_handle);
		if (!add_new_line)
			break;

		minimum_width_next = (line_box->HasContent() ? 0.f : line_box->GetLineWidth() + 1.f);

		// Keep adding boxes on a new line, either because the box couldn't fit on the current line at all, or because it had to be split.
		CloseOpenLineBox(false);
	}

	return inline_box;
}

void InlineContainer::CloseInlineElement(InlineBox* inline_box)
{
	if (LineBox* line_box = GetOpenLineBox())
	{
		line_box->CloseInlineBox(inline_box);
	}
	else
	{
		RMLUI_ERROR;
	}
}

void InlineContainer::AddBreak(float line_height)
{
	// Simply end the line if one is open, otherwise increment by the line height.
	if (GetOpenLineBox())
		CloseOpenLineBox(true);
	else
		box_cursor += line_height;
}

void InlineContainer::AddChainedBox(UniquePtr<LineBox> open_line_box)
{
	RMLUI_ASSERT(line_boxes.empty());
	RMLUI_ASSERT(open_line_box && !open_line_box->IsClosed());
	line_boxes.push_back(std::move(open_line_box));
}

void InlineContainer::Close(UniquePtr<LineBox>* out_open_line_box, Vector2f& out_position, float& out_height)
{
	RMLUI_ZoneScoped;

	// The parent container may need the open line box to be split and resumed.
	CloseOpenLineBox(true, out_open_line_box);

	// It is possible that floats were queued between closing the last line and closing this container, if so place them now.
	parent->PlaceQueuedFloats(position.y + box_cursor);

	// Set this box's height.
	box_size.y = Math::Max(box_cursor, 0.f);

	// Find the overflow size for our content, relative to our local space.
	Vector2f visible_overflow_size = {0.f, box_size.y};

	for (const auto& line_box : line_boxes)
	{
		visible_overflow_size.x = Math::Max(visible_overflow_size.x, line_box->GetPosition().x - position.x + line_box->GetExtentRight());
	}

	visible_overflow_size.x = Math::RoundDown(visible_overflow_size.x);
	SetVisibleOverflowSize(visible_overflow_size);

	out_position = position;
	out_height = box_size.y;
}

void InlineContainer::CloseOpenLineBox(bool split_all_open_boxes, UniquePtr<LineBox>* out_split_line)
{
	if (LineBox* line_box = GetOpenLineBox())
	{
		float height_of_line = 0.f;
		UniquePtr<LineBox> split_line_box = line_box->DetermineVerticalPositioning(&root_inline_box, split_all_open_boxes, height_of_line);

		// If the final height of the line is larger than previously considered, we might need to push the line down to
		// clear overlapping floats.
		if (height_of_line > line_box->GetLineMinimumHeight())
			UpdateLineBoxPlacement(line_box, 0.f, height_of_line);

		// Now that the line has been given a final position and size, close the line box to submit all the fragments.
		// Our parent block container acts as the containing block for our inline boxes.
		line_box->Close(parent->GetElement(), parent->GetPosition(), text_align);

		// Move the cursor down, unless we should collapse the line.
		if (!line_box->CanCollapseLine())
			box_cursor = (line_box->GetPosition().y - position.y) + height_of_line;

		// If we have any pending floating elements for our parent, then this would be an ideal time to place them.
		parent->PlaceQueuedFloats(position.y + box_cursor);

		if (split_line_box)
		{
			if (out_split_line)
				*out_split_line = std::move(split_line_box);
			else
				line_boxes.push_back(std::move(split_line_box));
		}
	}
}

bool InlineContainer::GetOpenLineBoxDimensions(float& out_vertical_position, Vector2f& out_tentative_size) const
{
	if (LineBox* line_box = GetOpenLineBox())
	{
		out_vertical_position = position.y + box_cursor;
		out_tentative_size = {line_box->GetBoxCursor(), line_box->GetLineMinimumHeight()};
		return true;
	}
	return false;
}

void InlineContainer::UpdateOpenLineBoxPlacement()
{
	if (LineBox* line_box = GetOpenLineBox())
		UpdateLineBoxPlacement(line_box, 0.f, element_line_height);
}

void InlineContainer::UpdateLineBoxPlacement(LineBox* line_box, float minimum_width, float minimum_height)
{
	RMLUI_ASSERT(line_box);

	Vector2f minimum_dimensions = {
		Math::Max(minimum_width, line_box->GetBoxCursor()),
		Math::Max(minimum_height, line_box->GetLineMinimumHeight()),
	};

	// @performance: We might benefit from doing this search only when the minimum dimensions change, or if we get new inline floats.
	const float ideal_position_y = position.y + box_cursor;
	float available_width = 0.f;
	const Vector2f line_position =
		parent->GetBlockBoxSpace()->NextBoxPosition(parent, available_width, ideal_position_y, minimum_dimensions, !wrap_content);
	available_width = Math::Max(available_width, 0.f);

	line_box->SetLineBox(line_position, available_width, minimum_dimensions.y);
}

float InlineContainer::GetShrinkToFitWidth() const
{
	float content_width = 0.0f;

	// Simply find our widest line.
	for (const auto& line_box : line_boxes)
		content_width = Math::Max(content_width, line_box->GetBoxCursor());

	return content_width;
}

Vector2f InlineContainer::GetStaticPositionEstimate(bool inline_level_box) const
{
	Vector2f result = {0.f, box_cursor};

	if (const LineBox* line_box = GetOpenLineBox())
	{
		if (inline_level_box)
			result.x += line_box->GetBoxCursor();
		else
			result.y += element_line_height;
	}

	return result;
}

bool InlineContainer::GetBaselineOfLastLine(float& out_baseline) const
{
	if (!line_boxes.empty())
	{
		out_baseline = line_boxes.back()->GetPosition().y + line_boxes.back()->GetBaseline();
		return true;
	}
	return false;
}

LineBox* InlineContainer::EnsureOpenLineBox()
{
	if (line_boxes.empty() || line_boxes.back()->IsClosed())
	{
		line_boxes.push_back(MakeUnique<LineBox>());
	}
	return line_boxes.back().get();
}

LineBox* InlineContainer::GetOpenLineBox() const
{
	if (line_boxes.empty() || line_boxes.back()->IsClosed())
		return nullptr;
	return line_boxes.back().get();
}

InlineBoxBase* InlineContainer::GetOpenInlineBox()
{
	if (LineBox* line_box = GetOpenLineBox())
	{
		if (InlineBox* inline_box = line_box->GetOpenInlineBox())
			return inline_box;
	}
	return &root_inline_box;
}

String InlineContainer::DebugDumpTree(int depth) const
{
	String value = String(depth * 2, ' ') + "InlineContainer" + '\n';

	value += root_inline_box.DebugDumpTree(depth + 1);

	for (const auto& line_box : line_boxes)
		value += line_box->DebugDumpTree(depth + 1);

	return value;
}

} // namespace Rml
