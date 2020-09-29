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

// Convert width or height of a border box to the width or height of its corresponding content box.
static inline float BorderSizeToContentSize(float border_size, float border_padding_edges_size)
{
	if (border_size < 0.0f || border_size == FLT_MAX)
		return border_size;

	return Math::Max(0.0f, border_size - border_padding_edges_size);
}

// Generates the box for an element.
void LayoutDetails::BuildBox(Box& box, Vector2f containing_block, Element* element, bool inline_element, float override_shrink_to_fit_width)
{
	if (!element)
	{
		box.SetContent(containing_block);
		return;
	}

	const ComputedValues& computed = element->GetComputedValues();

	// Calculate the padding area.
	box.SetEdge(Box::PADDING, Box::TOP, Math::Max(0.0f, ResolveValue(computed.padding_top, containing_block.x)));
	box.SetEdge(Box::PADDING, Box::RIGHT, Math::Max(0.0f, ResolveValue(computed.padding_right, containing_block.x)));
	box.SetEdge(Box::PADDING, Box::BOTTOM, Math::Max(0.0f, ResolveValue(computed.padding_bottom, containing_block.x)));
	box.SetEdge(Box::PADDING, Box::LEFT, Math::Max(0.0f, ResolveValue(computed.padding_left, containing_block.x)));

	// Calculate the border area.
	box.SetEdge(Box::BORDER, Box::TOP, Math::Max(0.0f, computed.border_top_width));
	box.SetEdge(Box::BORDER, Box::RIGHT, Math::Max(0.0f, computed.border_right_width));
	box.SetEdge(Box::BORDER, Box::BOTTOM, Math::Max(0.0f, computed.border_bottom_width));
	box.SetEdge(Box::BORDER, Box::LEFT, Math::Max(0.0f, computed.border_left_width));

	// Prepare sizing of the content area.
	Vector2f content_area(-1, -1);
	Vector2f min_size = Vector2f(0, 0);
	Vector2f max_size = Vector2f(FLT_MAX, FLT_MAX);

	// Intrinsic size for replaced elements.
	Vector2f intrinsic_size(-1, -1);
	float intrinsic_ratio = -1;

	const bool replaced_element = element->GetIntrinsicDimensions(intrinsic_size, intrinsic_ratio);

	// Calculate the content area and constraints. 'auto' width and height are handled later.
	// For inline non-replaced elements, width and height are ignored, so we can skip the calculations.
	if (!inline_element || replaced_element)
	{
		if (content_area.x < 0 && computed.width.type != Style::Width::Auto)
			content_area.x = ResolveValue(computed.width, containing_block.x);

		if (content_area.y < 0 && computed.height.type != Style::Width::Auto)
			content_area.y = ResolveValue(computed.height, containing_block.y);

		min_size = Vector2f(
			ResolveValue(computed.min_width, containing_block.x),
			ResolveValue(computed.min_height, containing_block.y)
		);
		max_size = Vector2f(
			(computed.max_width.value < 0.f ? FLT_MAX : ResolveValue(computed.max_width, containing_block.x)),
			(computed.max_height.value < 0.f ? FLT_MAX : ResolveValue(computed.max_height, containing_block.y))
		);

		// Adjust sizes for the given box sizing model.
		if (computed.box_sizing == Style::BoxSizing::BorderBox)
		{
			const float border_padding_width = box.GetSizeAcross(Box::HORIZONTAL, Box::BORDER, Box::PADDING);
			const float border_padding_height = box.GetSizeAcross(Box::VERTICAL, Box::BORDER, Box::PADDING);

			min_size.x = BorderSizeToContentSize(min_size.x, border_padding_width);
			max_size.x = BorderSizeToContentSize(max_size.x, border_padding_width);
			content_area.x = BorderSizeToContentSize(content_area.x, border_padding_width);

			min_size.y = BorderSizeToContentSize(min_size.y, border_padding_height);
			max_size.y = BorderSizeToContentSize(max_size.y, border_padding_height);
			content_area.y = BorderSizeToContentSize(content_area.y, border_padding_height);
		}

		if (content_area.x >= 0)
			content_area.x = Math::Clamp(content_area.x, min_size.x, max_size.x);
		if (content_area.y >= 0)
			content_area.y = Math::Clamp(content_area.y, min_size.y, max_size.y);

		if (replaced_element)
			content_area = CalculateSizeForReplacedElement(content_area, min_size, max_size, intrinsic_size, intrinsic_ratio);
	}

	box.SetContent(content_area);

	// Evaluate the margins, and width and height if they are auto.
	BuildBoxSizeAndMargins(box, min_size, max_size, containing_block, element, inline_element, replaced_element, override_shrink_to_fit_width);
}

// Generates the box for an element placed in a block box.
void LayoutDetails::BuildBox(Box& box, float& min_height, float& max_height, LayoutBlockBox* containing_box, Element* element, bool inline_element, float override_shrink_to_fit_width)
{
	Vector2f containing_block = LayoutDetails::GetContainingBlock(containing_box);

	BuildBox(box, containing_block, element, inline_element, override_shrink_to_fit_width);

	if (element)
		GetDefiniteMinMaxHeight(min_height, max_height, element->GetComputedValues(), box, containing_block.y);
	else
		min_height = max_height = box.GetSize().y;
}

void LayoutDetails::GetMinMaxWidth(float& min_width, float& max_width, const ComputedValues& computed, const Box& box, float containing_block_width)
{
	min_width = ResolveValue(computed.min_width, containing_block_width);
	max_width = (computed.max_width.value < 0.f ? FLT_MAX : ResolveValue(computed.max_width, containing_block_width));

	if (computed.box_sizing == Style::BoxSizing::BorderBox)
	{
		const float border_padding_width = box.GetSizeAcross(Box::HORIZONTAL, Box::BORDER, Box::PADDING);
		min_width = BorderSizeToContentSize(min_width, border_padding_width);
		max_width = BorderSizeToContentSize(max_width, border_padding_width);
	}
}


void LayoutDetails::GetMinMaxHeight(float& min_height, float& max_height, const ComputedValues& computed, const Box& box, float containing_block_height)
{
	min_height = ResolveValue(computed.min_height, containing_block_height);
	max_height = (computed.max_height.value < 0.f ? FLT_MAX : ResolveValue(computed.max_height, containing_block_height));

	if (computed.box_sizing == Style::BoxSizing::BorderBox)
	{
		const float border_padding_height = box.GetSizeAcross(Box::VERTICAL, Box::BORDER, Box::PADDING);
		min_height = BorderSizeToContentSize(min_height, border_padding_height);
		max_height = BorderSizeToContentSize(max_height, border_padding_height);
	}
}

void LayoutDetails::GetDefiniteMinMaxHeight(float& min_height, float& max_height, const ComputedValues& computed, const Box& box, float containing_block_height)
{
	const float box_height = box.GetSize().y;
	if (box_height < 0)
	{
		GetMinMaxHeight(min_height, max_height, computed, box, containing_block_height);
	}
	else
	{
		min_height = box_height;
		max_height = box_height;
	}
}

// Returns the fully-resolved, fixed-width and -height containing block from a block box.
Vector2f LayoutDetails::GetContainingBlock(const LayoutBlockBox* containing_box)
{
	RMLUI_ASSERT(containing_box);

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


void LayoutDetails::BuildBoxSizeAndMargins(Box& box, Vector2f min_size, Vector2f max_size, Vector2f containing_block, Element* element, bool inline_element, bool replaced_element, float override_shrink_to_fit_width)
{
	const ComputedValues& computed = element->GetComputedValues();

	if (inline_element)
	{
		// For inline elements, their calculations are straightforward. No worrying about auto margins and dimensions, etc.
		// Evaluate the margins. Any declared as 'auto' will resolve to 0.
		box.SetEdge(Box::MARGIN, Box::TOP, ResolveValue(computed.margin_top, containing_block.x));
		box.SetEdge(Box::MARGIN, Box::RIGHT, ResolveValue(computed.margin_right, containing_block.x));
		box.SetEdge(Box::MARGIN, Box::BOTTOM, ResolveValue(computed.margin_bottom, containing_block.x));
		box.SetEdge(Box::MARGIN, Box::LEFT, ResolveValue(computed.margin_left, containing_block.x));
	}
	else
	{
		// The element is block, so we need to run the box through the ringer to potentially evaluate auto margins and dimensions.
		BuildBoxWidth(box, computed, min_size.x, max_size.x, containing_block, element, replaced_element, override_shrink_to_fit_width);
		BuildBoxHeight(box, computed, min_size.y, max_size.y, containing_block.y);
	}
}

float LayoutDetails::GetShrinkToFitWidth(Element* element, Vector2f containing_block)
{
	RMLUI_ASSERT(element);

	Box box;
	float min_height, max_height;
	LayoutDetails::BuildBox(box, containing_block, element, false, containing_block.x);
	LayoutDetails::GetDefiniteMinMaxHeight(min_height, max_height, element->GetComputedValues(), box, containing_block.y);

	// First we need to format the element, then we get the shrink-to-fit width based on the largest line or box.
	LayoutBlockBox containing_block_box(nullptr, nullptr, Box(containing_block), 0.0f, FLT_MAX);

	// Here we fix the element's width to its containing block so that any content is wrapped at this width.
	// We can consider to instead set this to infinity and clamp it to the available width later after formatting,
	// but right now the formatting procedure doesn't work well with such numbers.
	LayoutBlockBox* block_context_box = containing_block_box.AddBlockElement(element, box, min_height, max_height);

	// @performance. Some formatting can be simplified, eg. absolute elements do not contribute to the shrink-to-fit width.
	// Also, children of elements with a fixed width and height don't need to be formatted further.
	for (int i = 0; i < element->GetNumChildren(); i++)
	{
		if (!LayoutEngine::FormatElement(block_context_box, element->GetChild(i)))
			i = -1;
	}

	// We only do layouting to get the fit-to-shrink width here, and for this purpose we may get
	// away with not closing the boxes. This is avoided for performance reasons.
	//block_context_box->Close();

	return Math::Min(containing_block.x, block_context_box->GetShrinkToFitWidth());
}

Vector2f LayoutDetails::CalculateSizeForReplacedElement(const Vector2f specified_content_size, const Vector2f min_size, const Vector2f max_size, const Vector2f intrinsic_size, const float intrinsic_ratio)
{
	// Start with the element's specified width and height. If any of them are auto, use the element's intrinsic
	// dimensions and ratio to find a suitable content size.
	Vector2f content_size = specified_content_size;

	const bool auto_width = (content_size.x < 0);
	const bool auto_height = (content_size.y < 0);

	if (auto_width)
		content_size.x = intrinsic_size.x;

	if (auto_height)
		content_size.y = intrinsic_size.y;

	// Use a fallback size if we still couldn't determine the size.
	if (content_size.x < 0)
		content_size.x = 300;
	if (content_size.y < 0)
		content_size.y = 150;

	// Resolve the size constraints.
	const float min_width = min_size.x;
	const float max_width = max_size.x;
	const float min_height = min_size.y;
	const float max_height = max_size.y;

	// If we have an intrinsic ratio and one of the dimensions is 'auto', then scale it such that the ratio is preserved.
	if (intrinsic_ratio > 0)
	{
		if (auto_width && !auto_height)
		{
			content_size.x = content_size.y * intrinsic_ratio;
		}
		else if (auto_height && !auto_width)
		{
			content_size.y = content_size.x / intrinsic_ratio;
		}
		else if (auto_width && auto_height)
		{
			// If both width and height are auto, try to preserve the ratio under the respective min/max constraints.
			const float w = content_size.x;
			const float h = content_size.y;

			if ((w < min_width && h > max_height) || (w > max_width && h < min_height))
			{
				// Cannot preserve aspect ratio, let it be clamped.
			}
			else if (w < min_width && h < min_height)
			{
				// Increase the size such that both min-constraints are respected. The non-scaled axis will
				// be clamped below, preserving the aspect ratio.
				if (min_width <= min_height * intrinsic_ratio)
					content_size.x = min_height * intrinsic_ratio;
				else
					content_size.y = min_width / intrinsic_ratio;
			}
			else if (w > max_width && h > max_height)
			{
				// Shrink the size such that both max-constraints are respected. The non-scaled axis will
				// be clamped below, preserving the aspect ratio.
				if (max_width <= max_height * intrinsic_ratio)
					content_size.y = max_width / intrinsic_ratio;
				else
					content_size.x = max_height * intrinsic_ratio;
			}
			else
			{
				// Single constraint violations.
				if (w < min_width)
					content_size.y = min_width / intrinsic_ratio;
				else if (w > max_width)
					content_size.y = max_width / intrinsic_ratio;
				else if (h < min_height)
					content_size.x = min_height * intrinsic_ratio;
				else if (h > max_height)
					content_size.x = max_height * intrinsic_ratio;
			}
		}
	}

	content_size.x = Math::Clamp(content_size.x, min_width, max_width);
	content_size.y = Math::Clamp(content_size.y, min_height, max_height);

	return content_size;
}

// Builds the block-specific width and horizontal margins of a Box.
void LayoutDetails::BuildBoxWidth(Box& box, const ComputedValues& computed, float min_width, float max_width, Vector2f containing_block, Element* element, bool replaced_element, float override_shrink_to_fit_width)
{
	RMLUI_ZoneScoped;

	Vector2f content_area = box.GetSize();

	// Determine if the element has automatic margins.
	bool margins_auto[2];
	int num_auto_margins = 0;

	for (int i = 0; i < 2; ++i)
	{
		const Style::Margin& margin_value = (i == 0 ? computed.margin_left : computed.margin_right);
		if (margin_value.type == Style::Margin::Auto)
		{
			margins_auto[i] = true;
			num_auto_margins++;
			box.SetEdge(Box::MARGIN, i == 0 ? Box::LEFT : Box::RIGHT, 0);
		}
		else
		{
			margins_auto[i] = false;
			box.SetEdge(Box::MARGIN, i == 0 ? Box::LEFT : Box::RIGHT, ResolveValue(margin_value, containing_block.x));
		}
	}

	const bool width_auto = (content_area.x < 0);

	// If the width is set to auto, we need to calculate the width
	if (width_auto)
	{
		// Apply the shrink-to-fit algorithm here to find the width of the element.
		// See CSS 2.1 section 10.3.7 for when this should be applied.
		const bool shrink_to_fit = !replaced_element &&
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

		if (shrink_to_fit && override_shrink_to_fit_width < 0)
		{
			content_area.x = GetShrinkToFitWidth(element, containing_block);
			override_shrink_to_fit_width = content_area.x;
		}
		else if (shrink_to_fit)
		{
			content_area.x = override_shrink_to_fit_width;
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
		const float margin = (containing_block.x - box.GetSizeAcross(Box::HORIZONTAL, Box::MARGIN)) / float(num_auto_margins);

		if (margins_auto[0])
			box.SetEdge(Box::MARGIN, Box::LEFT, margin);
		if (margins_auto[1])
			box.SetEdge(Box::MARGIN, Box::RIGHT, margin);
	}

	// Clamp the calculated width; if the width is changed by the clamp, then the margins need to be recalculated if
	// they were set to auto.
	const float clamped_width = Math::Clamp(content_area.x, min_width, max_width);
	if (clamped_width != content_area.x)
	{
		content_area.x = clamped_width;
		box.SetContent(content_area);

		if (num_auto_margins > 0)
			BuildBoxWidth(box, computed, min_width, max_width, containing_block, element, replaced_element, override_shrink_to_fit_width);
	}
	else
		box.SetContent(content_area);
}

// Builds the block-specific height and vertical margins of a Box.
void LayoutDetails::BuildBoxHeight(Box& box, const ComputedValues& computed, float min_height, float max_height, float containing_block_height)
{
	RMLUI_ZoneScoped;

	Vector2f content_area = box.GetSize();

	// Determine if the element has automatic margins.
	bool margins_auto[2];
	int num_auto_margins = 0;

	for (int i = 0; i < 2; ++i)
	{
		const Style::Margin& margin_value = (i == 0 ? computed.margin_top : computed.margin_bottom);
		if (margin_value.type == Style::Margin::Auto)
		{
			margins_auto[i] = true;
			num_auto_margins++;
			box.SetEdge(Box::MARGIN, i == 0 ? Box::TOP : Box::BOTTOM, 0);
		}
		else
		{
			margins_auto[i] = false;
			box.SetEdge(Box::MARGIN, i == 0 ? Box::TOP : Box::BOTTOM, ResolveValue(margin_value, containing_block_height));
		}
	}

	const bool height_auto = (content_area.y < 0);

	// If the height is set to auto, we need to calculate the height
	if (height_auto)
	{
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
		float margin = 0;
		if (content_area.y >= 0)
			margin = (containing_block_height - box.GetSizeAcross(Box::VERTICAL, Box::MARGIN)) / num_auto_margins;

		if (margins_auto[0])
			box.SetEdge(Box::MARGIN, Box::TOP, margin);
		if (margins_auto[1])
			box.SetEdge(Box::MARGIN, Box::BOTTOM, margin);
	}

	if (content_area.y >= 0)
	{
		// Clamp the calculated height; if the height is changed by the clamp, then the margins need to be recalculated if
		// they were set to auto.
		float clamped_height = Math::Clamp(content_area.y, min_height, max_height);
		if (clamped_height != content_area.y)
		{
			content_area.y = clamped_height;
			box.SetContent(content_area);

			if (num_auto_margins > 0)
				BuildBoxHeight(box, computed, min_height, max_height, containing_block_height);

			return;
		}
	}

	box.SetContent(content_area);
}

} // namespace Rml
