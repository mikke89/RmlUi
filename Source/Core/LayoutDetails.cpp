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

#include "LayoutDetails.h"
#include "LayoutEngine.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/ElementScroll.h"
#include "../../Include/RmlUi/Core/Math.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include <float.h>

namespace Rml {

// Generates the box for an element.
void LayoutDetails::BuildBox(Box& box, const Vector2f& containing_block, Element* element, bool inline_element, bool allow_shrink)
{
	if (element == nullptr)
	{
		box.SetContent(containing_block);
		return;
	}

	const ComputedValues& computed = element->GetComputedValues();

	// Calculate the padding area.
	float padding = ResolveValue(computed.padding_top, containing_block.x);
	box.SetEdge(Box::PADDING, Box::TOP, Math::Max(0.0f, padding));
	padding = ResolveValue(computed.padding_right, containing_block.x);
	box.SetEdge(Box::PADDING, Box::RIGHT, Math::Max(0.0f, padding));
	padding = ResolveValue(computed.padding_bottom, containing_block.x);
	box.SetEdge(Box::PADDING, Box::BOTTOM, Math::Max(0.0f, padding));
	padding = ResolveValue(computed.padding_left, containing_block.x);
	box.SetEdge(Box::PADDING, Box::LEFT, Math::Max(0.0f, padding));

	// Calculate the border area.
	box.SetEdge(Box::BORDER, Box::TOP, Math::Max(0.0f, computed.border_top_width));
	box.SetEdge(Box::BORDER, Box::RIGHT, Math::Max(0.0f, computed.border_right_width));
	box.SetEdge(Box::BORDER, Box::BOTTOM, Math::Max(0.0f, computed.border_bottom_width));
	box.SetEdge(Box::BORDER, Box::LEFT, Math::Max(0.0f, computed.border_left_width));

	// Calculate the size of the content area.
	Vector2f content_area(-1, -1);
	bool replaced_element = false;

	// If the element has intrinsic dimensions, then we use those as the basis for the content area and only adjust
	// them if a non-auto style has been applied to them.
	if (element->GetIntrinsicDimensions(content_area))
	{
		replaced_element = true;

		Vector2f original_content_area = content_area;

		// The element has resized itself, so we only resize it if a RCSS width or height was set explicitly. A value of
		// 'auto' (or 'auto-fit', ie, both keywords) means keep (or adjust) the intrinsic dimensions.
		bool auto_width = false, auto_height = false;

		if (computed.width.type != Style::Width::Auto)
			content_area.x = ResolveValue(computed.width, containing_block.x);
		else
			auto_width = true;

		if (computed.height.type != Style::Height::Auto)
			content_area.y = ResolveValue(computed.height, containing_block.y);
		else
			auto_height = true;

		// If one of the dimensions is 'auto' then we need to scale it such that the original ratio is preserved.
		if (auto_width && !auto_height)
			content_area.x = (content_area.y / original_content_area.y) * original_content_area.x;
		else if (auto_height && !auto_width)
			content_area.y = (content_area.x / original_content_area.x) * original_content_area.y;

		// Reduce the width and height to make up for borders and padding.
		content_area.x -= (box.GetEdge(Box::BORDER, Box::LEFT) +
						   box.GetEdge(Box::PADDING, Box::LEFT) +
						   box.GetEdge(Box::BORDER, Box::RIGHT) +
						   box.GetEdge(Box::PADDING, Box::RIGHT));
		content_area.y -= (box.GetEdge(Box::BORDER, Box::TOP) +
						   box.GetEdge(Box::PADDING, Box::TOP) +
						   box.GetEdge(Box::BORDER, Box::BOTTOM) +
						   box.GetEdge(Box::PADDING, Box::BOTTOM));

		content_area.x = Math::Max(content_area.x, 0.0f);
		content_area.y = Math::Max(content_area.y, 0.0f);
	}

	// If the element is inline, then its calculations are much more straightforward (no worrying about auto margins
	// and dimensions, etc). All we do is calculate the margins, set the content area and bail.
	if (inline_element)
	{
		if (replaced_element)
		{
			content_area.x = ClampWidth(content_area.x, computed, containing_block.x);
			content_area.y = ClampHeight(content_area.y, computed, containing_block.y);
		}

		// If the element was not replaced, then we leave its dimension as unsized (-1, -1) and ignore the width and
		// height properties.
		box.SetContent(content_area);

		// Evaluate the margins. Any declared as 'auto' will resolve to 0.
		box.SetEdge(Box::MARGIN, Box::TOP, ResolveValue(computed.margin_top, containing_block.x));
		box.SetEdge(Box::MARGIN, Box::RIGHT, ResolveValue(computed.margin_right, containing_block.x));
		box.SetEdge(Box::MARGIN, Box::BOTTOM, ResolveValue(computed.margin_bottom, containing_block.x));
		box.SetEdge(Box::MARGIN, Box::LEFT, ResolveValue(computed.margin_left, containing_block.x));
	}

	// The element is block, so we need to run the box through the ringer to potentially evaluate auto margins and
	// dimensions.
	else
	{
		box.SetContent(content_area);
		BuildBoxWidth(box, computed, containing_block, element, allow_shrink, replaced_element);
		BuildBoxHeight(box, computed, containing_block.y);
	}
}

// Generates the box for an element placed in a block box.
void LayoutDetails::BuildBox(Box& box, float& min_height, float& max_height, LayoutBlockBox* containing_box, Element* element, bool inline_element, bool allow_shrink)
{
	Vector2f containing_block = GetContainingBlock(containing_box);
	BuildBox(box, containing_block, element, inline_element, allow_shrink);

	float box_height = box.GetSize().y;
	if (box_height < 0)
	{
		auto& computed = element->GetComputedValues();
		min_height = ResolveValue(computed.min_height, containing_block.y);
		max_height = (computed.max_height.value < 0.f ? FLT_MAX : ResolveValue(computed.max_height, containing_block.y));
	}
	else
	{
		min_height = box_height;
		max_height = box_height;
	}
}

// Clamps the width of an element based from its min-width and max-width properties.
float LayoutDetails::ClampWidth(float width, const ComputedValues& computed, float containing_block_width)
{
	float min_width = ResolveValue(computed.min_width, containing_block_width);
	float max_width = (computed.max_width.value < 0.f ? FLT_MAX : ResolveValue(computed.max_width, containing_block_width));

	return Math::Clamp(width, min_width, max_width);
}

// Clamps the height of an element based from its min-height and max-height properties.
float LayoutDetails::ClampHeight(float height, const ComputedValues& computed, float containing_block_height)
{
	float min_height = ResolveValue(computed.min_height, containing_block_height);
	float max_height = (computed.max_height.value < 0.f ? FLT_MAX : ResolveValue(computed.max_height, containing_block_height));

	return Math::Clamp(height, min_height, max_height);
}

// Returns the fully-resolved, fixed-width and -height containing block from a block box.
Vector2f LayoutDetails::GetContainingBlock(const LayoutBlockBox* containing_box)
{
	Vector2f containing_block;

	containing_block.x = containing_box->GetBox().GetSize(Box::CONTENT).x;
	if (containing_box->GetElement() != nullptr)
		containing_block.x -= containing_box->GetElement()->GetElementScroll()->GetScrollbarSize(ElementScroll::VERTICAL);

	while ((containing_block.y = containing_box->GetBox().GetSize(Box::CONTENT).y) < 0)
	{
		containing_box = containing_box->GetParent();
		if (containing_box == nullptr)
		{
			RMLUI_ERROR;
			containing_block.y = 0;
		}
	}
	if (containing_box != nullptr &&
		containing_box->GetElement() != nullptr)
		containing_block.y -= containing_box->GetElement()->GetElementScroll()->GetScrollbarSize(ElementScroll::HORIZONTAL);

	containing_block.x = Math::Max(0.0f, containing_block.x);
	containing_block.y = Math::Max(0.0f, containing_block.y);

	return containing_block;
}


float LayoutDetails::GetShrinkToFitWidth(Element* element, Vector2f containing_block)
{
	LayoutBlockBox containing_block_box(nullptr, nullptr);
	containing_block_box.GetBox().SetContent(containing_block);

	LayoutBlockBox* block_context_box = containing_block_box.AddBlockElement(element, false);

	for (int i = 0; i < element->GetNumChildren(); i++)
	{
		if (!LayoutEngine::FormatElement(block_context_box, element->GetChild(i)))
			i = -1;
	}

	// We only do layouting to get the fit-to-shrink width here, and for this purpose we may get
	// away with not closing the boxes. This is avoided for performance reasons.
	//block_context_box->Close();
	//block_context_box->CloseAbsoluteElements();

	return Math::Min(containing_block.x, block_context_box->GetShrinkToFitWidth());
}

// Builds the block-specific width and horizontal margins of a Box.
void LayoutDetails::BuildBoxWidth(Box& box, const ComputedValues& computed, Vector2f containing_block, Element* element, bool allow_shrink, bool replaced_element)
{
	RMLUI_ZoneScoped;

	Vector2f content_area = box.GetSize();

	// Determine if the element has an automatic width, and if not calculate it.
	bool width_auto;
	if (content_area.x >= 0)
	{
		width_auto = false;
	}
	else
	{
		if (computed.width.type == Style::Width::Auto)
		{
			width_auto = true;
		}
		else
		{
			width_auto = false;
			content_area.x = ResolveValue(computed.width, containing_block.x);
		}
	}

	// Determine if the element has automatic margins.
	bool margins_auto[2];
	int num_auto_margins = 0;

	for (int i = 0; i < 2; ++i)
	{
		auto* margin_value = (i == 0 ? &computed.margin_left : &computed.margin_right);
		if (margin_value->type == Style::Margin::Auto)
		{
			margins_auto[i] = true;
			num_auto_margins++;
		}
		else
		{
			margins_auto[i] = false;
			box.SetEdge(Box::MARGIN, i == 0 ? Box::LEFT : Box::RIGHT, ResolveValue(*margin_value, containing_block.x));
		}
	}

	// If the width is set to auto, we need to calculate the width
	if (width_auto)
	{
		// Apply the shrink-to-fit algorithm here to find the width of the element.
		// See CSS 2.1 section 10.3.7 for when this should be applied.
		const bool shrink_to_fit = allow_shrink && !replaced_element &&
			(
				(computed.float_ != Style::Float::None) ||
				((computed.position == Style::Position::Absolute || computed.position == Style::Position::Fixed) && (computed.left.type == Style::Left::Auto || computed.right.type == Style::Right::Auto)) ||
				(computed.display == Style::Display::InlineBlock)
			);

		
		float left = 0.0f, right = 0.0f;
		// If we are dealing with an absolutely positioned element we need to
		// consider if the left and right properties are set, since the width can be affected.
		if (computed.position == Style::Position::Absolute || computed.position == Style::Position::Fixed)
		{
			if (computed.left.type != Style::Left::Auto)
				left = ResolveValue(computed.left, containing_block.x);
			if (computed.right.type != Style::Right::Auto)
				right = ResolveValue(computed.right, containing_block.x);
		}

		if (margins_auto[0])
			box.SetEdge(Box::MARGIN, Box::LEFT, 0);
		if (margins_auto[1])
			box.SetEdge(Box::MARGIN, Box::RIGHT, 0);

		if (shrink_to_fit)
		{
			content_area.x = GetShrinkToFitWidth(element, containing_block);
		}
		else
		{
			// We resolve any auto margins to 0 and the width is set to whatever is left of the containing block.
			content_area.x = containing_block.x - (left +
				box.GetCumulativeEdge(Box::CONTENT, Box::LEFT) +
				box.GetCumulativeEdge(Box::CONTENT, Box::RIGHT) +
				right);
			content_area.x = Math::Max(0.0f, content_area.x);
		}
	}
	// Otherwise, the margins that are set to auto will pick up the remaining width of the containing block.
	else if (num_auto_margins > 0)
	{
		float margin = (containing_block.x - (box.GetCumulativeEdge(Box::CONTENT, Box::LEFT) +
												  box.GetCumulativeEdge(Box::CONTENT, Box::RIGHT) +
												  content_area.x)) / num_auto_margins;

		if (margins_auto[0])
			box.SetEdge(Box::MARGIN, Box::LEFT, margin);
		if (margins_auto[1])
			box.SetEdge(Box::MARGIN, Box::RIGHT, margin);
	}

	// Clamp the calculated width; if the width is changed by the clamp, then the margins need to be recalculated if
	// they were set to auto.
	float clamped_width = ClampWidth(content_area.x, computed, containing_block.x);
	if (clamped_width != content_area.x)
	{
		content_area.x = clamped_width;
		box.SetContent(content_area);

		if (num_auto_margins > 0)
		{
			// Reset the automatic margins.
			if (margins_auto[0])
				box.SetEdge(Box::MARGIN, Box::LEFT, 0);
			if (margins_auto[1])
				box.SetEdge(Box::MARGIN, Box::RIGHT, 0);

			BuildBoxWidth(box, computed, containing_block, element, allow_shrink, replaced_element);
		}
	}
	else
		box.SetContent(content_area);
}

// Builds the block-specific height and vertical margins of a Box.
void LayoutDetails::BuildBoxHeight(Box& box, const ComputedValues& computed, float containing_block_height)
{
	RMLUI_ZoneScoped;

	Vector2f content_area = box.GetSize();

	// Determine if the element has an automatic height, and if not calculate it.
	bool height_auto;
	if (content_area.y >= 0)
	{
		height_auto = false;
	}
	else
	{
		if (computed.height.type == Style::Height::Auto)
		{
			height_auto = true;
		}
		else
		{
			height_auto = false;
			content_area.y = ResolveValue(computed.height, containing_block_height);
		}
	}

	// Determine if the element has automatic margins.
	bool margins_auto[2];
	int num_auto_margins = 0;

	for (int i = 0; i < 2; ++i)
	{
		auto* margin_value = (i == 0 ? &computed.margin_top : &computed.margin_bottom);
		if (margin_value->type == Style::Margin::Auto)
		{
			margins_auto[i] = true;
			num_auto_margins++;
		}
		else
		{
			margins_auto[i] = false;
			box.SetEdge(Box::MARGIN, i == 0 ? Box::TOP : Box::BOTTOM, ResolveValue(*margin_value, containing_block_height));
		}
	}

	// If the height is set to auto, we need to calculate the height
	if (height_auto)
	{
		// We resolve any auto margins to 0
		if (margins_auto[0])
			box.SetEdge(Box::MARGIN, Box::TOP, 0);
		if (margins_auto[1])
			box.SetEdge(Box::MARGIN, Box::BOTTOM, 0);

		// If the height is set to auto for a box in normal flow, the height is set to -1.
		content_area.y = -1;

		// But if we are dealing with an absolutely positioned element we need to
		// consider if the top and bottom properties are set, since the height can be affected.
		if (computed.position == Style::Position::Absolute || computed.position == Style::Position::Fixed)
		{
			float top = 0.0f, bottom = 0.0f;

			if (computed.top.type != Style::Top::Auto && computed.bottom.type != Style::Bottom::Auto)
			{
				top = ResolveValue(computed.top, containing_block_height );
				bottom = ResolveValue(computed.bottom, containing_block_height );

				// The height gets resolved to whatever is left of the containing block
				content_area.y = containing_block_height - (top +
				                                            box.GetCumulativeEdge(Box::CONTENT, Box::TOP) +
				                                            box.GetCumulativeEdge(Box::CONTENT, Box::BOTTOM) +
				                                            bottom);
				content_area.y = Math::Max(0.0f, content_area.y);
			}
		}
	}
	// Otherwise, the margins that are set to auto will pick up the remaining width of the containing block.
	else if (num_auto_margins > 0)
	{
		float margin;
		if (content_area.y >= 0)
		{
			margin = (containing_block_height - (box.GetCumulativeEdge(Box::CONTENT, Box::TOP) +
												 box.GetCumulativeEdge(Box::CONTENT, Box::BOTTOM) +
												 content_area.y)) / num_auto_margins;
		}
		else
			margin = 0;

		if (margins_auto[0])
			box.SetEdge(Box::MARGIN, Box::TOP, margin);
		if (margins_auto[1])
			box.SetEdge(Box::MARGIN, Box::BOTTOM, margin);
	}

	if (content_area.y >= 0)
	{
		// Clamp the calculated height; if the height is changed by the clamp, then the margins need to be recalculated if
		// they were set to auto.
		float clamped_height = ClampHeight(content_area.y, computed, containing_block_height);
		if (clamped_height != content_area.y)
		{
			content_area.y = clamped_height;
			box.SetContent(content_area);

			if (num_auto_margins > 0)
			{
				// Reset the automatic margins.
				if (margins_auto[0])
					box.SetEdge(Box::MARGIN, Box::TOP, 0);
				if (margins_auto[1])
					box.SetEdge(Box::MARGIN, Box::BOTTOM, 0);

				BuildBoxHeight(box, computed, containing_block_height);
			}

			return;
		}
	}

	box.SetContent(content_area);
}

} // namespace Rml
