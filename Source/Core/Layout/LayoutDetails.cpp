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

#include "LayoutDetails.h"
#include "../../../Include/RmlUi/Core/ComputedValues.h"
#include "../../../Include/RmlUi/Core/Element.h"
#include "../../../Include/RmlUi/Core/ElementScroll.h"
#include "../../../Include/RmlUi/Core/ElementText.h"
#include "../../../Include/RmlUi/Core/Math.h"
#include "../../../Include/RmlUi/Core/Profiling.h"
#include "ContainerBox.h"
#include "FormattingContext.h"
#include "LayoutEngine.h"
#include <float.h>

namespace Rml {

// Convert width or height of a border box to the width or height of its corresponding content box.
static inline float BorderSizeToContentSize(float border_size, float border_padding_edges_size)
{
	if (border_size < 0.0f || border_size >= FLT_MAX)
		return border_size;

	return Math::Max(0.0f, border_size - border_padding_edges_size);
}

void LayoutDetails::BuildBox(Box& box, Vector2f containing_block, Element* element, BuildBoxMode box_context)
{
	if (!element)
	{
		box.SetContent(containing_block);
		return;
	}

	const ComputedValues& computed = element->GetComputedValues();

	// Calculate the padding area.
	box.SetEdge(BoxArea::Padding, BoxEdge::Top, Math::Max(0.0f, ResolveValue(computed.padding_top(), containing_block.x)));
	box.SetEdge(BoxArea::Padding, BoxEdge::Right, Math::Max(0.0f, ResolveValue(computed.padding_right(), containing_block.x)));
	box.SetEdge(BoxArea::Padding, BoxEdge::Bottom, Math::Max(0.0f, ResolveValue(computed.padding_bottom(), containing_block.x)));
	box.SetEdge(BoxArea::Padding, BoxEdge::Left, Math::Max(0.0f, ResolveValue(computed.padding_left(), containing_block.x)));

	// Calculate the border area.
	box.SetEdge(BoxArea::Border, BoxEdge::Top, Math::Max(0.0f, computed.border_top_width()));
	box.SetEdge(BoxArea::Border, BoxEdge::Right, Math::Max(0.0f, computed.border_right_width()));
	box.SetEdge(BoxArea::Border, BoxEdge::Bottom, Math::Max(0.0f, computed.border_bottom_width()));
	box.SetEdge(BoxArea::Border, BoxEdge::Left, Math::Max(0.0f, computed.border_left_width()));

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
	if (box_context == BuildBoxMode::Block || box_context == BuildBoxMode::UnalignedBlock || replaced_element)
	{
		content_area.x = ResolveValueOr(computed.width(), containing_block.x, -1.f);
		content_area.y = ResolveValueOr(computed.height(), containing_block.y, -1.f);

		min_size = Vector2f{
			ResolveValueOr(computed.min_width(), containing_block.x, 0.f),
			ResolveValueOr(computed.min_height(), containing_block.y, 0.f),
		};
		max_size = Vector2f{
			ResolveValueOr(computed.max_width(), containing_block.x, FLT_MAX),
			ResolveValueOr(computed.max_height(), containing_block.y, FLT_MAX),
		};

		// Adjust sizes for the given box sizing model.
		if (computed.box_sizing() == Style::BoxSizing::BorderBox)
		{
			const float border_padding_width = box.GetSizeAcross(BoxDirection::Horizontal, BoxArea::Border, BoxArea::Padding);
			const float border_padding_height = box.GetSizeAcross(BoxDirection::Vertical, BoxArea::Border, BoxArea::Padding);

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
	BuildBoxSizeAndMargins(box, min_size, max_size, containing_block, element, box_context, replaced_element);
}

void LayoutDetails::GetMinMaxWidth(float& min_width, float& max_width, const ComputedValues& computed, const Box& box, float containing_block_width)
{
	min_width = ResolveValueOr(computed.min_width(), containing_block_width, 0.f);
	max_width = ResolveValueOr(computed.max_width(), containing_block_width, FLT_MAX);

	if (computed.box_sizing() == Style::BoxSizing::BorderBox)
	{
		const float border_padding_width = box.GetSizeAcross(BoxDirection::Horizontal, BoxArea::Border, BoxArea::Padding);
		min_width = BorderSizeToContentSize(min_width, border_padding_width);
		max_width = BorderSizeToContentSize(max_width, border_padding_width);
	}
}

void LayoutDetails::GetMinMaxHeight(float& min_height, float& max_height, const ComputedValues& computed, const Box& box,
	float containing_block_height)
{
	min_height = ResolveValueOr(computed.min_height(), containing_block_height, 0.f);
	max_height = ResolveValueOr(computed.max_height(), containing_block_height, FLT_MAX);

	if (computed.box_sizing() == Style::BoxSizing::BorderBox)
	{
		const float border_padding_height = box.GetSizeAcross(BoxDirection::Vertical, BoxArea::Border, BoxArea::Padding);
		min_height = BorderSizeToContentSize(min_height, border_padding_height);
		max_height = BorderSizeToContentSize(max_height, border_padding_height);
	}
}

void LayoutDetails::GetDefiniteMinMaxHeight(float& min_height, float& max_height, const ComputedValues& computed, const Box& box,
	float containing_block_height)
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

ContainingBlock LayoutDetails::GetContainingBlock(ContainerBox* parent_container, const Style::Position position)
{
	RMLUI_ASSERT(parent_container);
	using Style::Position;

	ContainerBox* container = parent_container;
	BoxArea area = BoxArea::Content;

	// For absolutely positioned boxes we look for the first positioned ancestor. We deviate from the CSS specs by using
	// the same rules for fixed boxes, as that is particularly helpful on handles and other widgets that should not
	// scroll with the window. This is a common design pattern in target applications for this library, although this
	// behavior may be reconsidered in the future.
	if (position == Position::Absolute || position == Position::Fixed)
	{
		area = BoxArea::Padding;

		while (container->GetParent() && !container->IsAbsolutePositioningContainingBlock())
			container = container->GetParent();
	}

	const Box* box = container->GetIfBox();
	if (!box)
	{
		RMLUI_ERROR;
		return {container, {}};
	}

	Vector2f containing_block = box->GetSize(area);

	if (position == Position::Static || position == Position::Relative)
	{
		// For static elements we subtract the scrollbar size so that elements normally don't overlap their parent's
		// scrollbars. In CSS, this would also be done for absolutely positioned elements, we might want to copy that
		// behavior in the future. If so, we would also need to change the element offset behavior, and ideally also
		// make positioned boxes contribute to the scrollable area.
		if (Element* element = container->GetElement())
		{
			ElementScroll* element_scroll = element->GetElementScroll();
			if (containing_block.x >= 0.f)
				containing_block.x = Math::Max(containing_block.x - element_scroll->GetScrollbarSize(ElementScroll::VERTICAL), 0.f);
			if (containing_block.y >= 0.f)
				containing_block.y = Math::Max(containing_block.y - element_scroll->GetScrollbarSize(ElementScroll::HORIZONTAL), 0.f);
		}
	}

	return {container, containing_block};
}

void LayoutDetails::BuildBoxSizeAndMargins(Box& box, Vector2f min_size, Vector2f max_size, Vector2f containing_block, Element* element,
	BuildBoxMode box_context, bool replaced_element)
{
	const ComputedValues& computed = element->GetComputedValues();

	if (box_context == BuildBoxMode::Inline || box_context == BuildBoxMode::UnalignedBlock)
	{
		// For inline elements, their calculations are straightforward. No worrying about auto margins and dimensions, etc.
		// Evaluate the margins. Any declared as 'auto' will resolve to 0.
		box.SetEdge(BoxArea::Margin, BoxEdge::Top, ResolveValue(computed.margin_top(), containing_block.x));
		box.SetEdge(BoxArea::Margin, BoxEdge::Right, ResolveValue(computed.margin_right(), containing_block.x));
		box.SetEdge(BoxArea::Margin, BoxEdge::Bottom, ResolveValue(computed.margin_bottom(), containing_block.x));
		box.SetEdge(BoxArea::Margin, BoxEdge::Left, ResolveValue(computed.margin_left(), containing_block.x));
	}
	else
	{
		// The element is block, so we need to run the box through the ringer to potentially evaluate auto margins and dimensions.
		BuildBoxWidth(box, computed, min_size.x, max_size.x, containing_block, element, replaced_element);
		BuildBoxHeight(box, computed, min_size.y, max_size.y, containing_block.y);
	}
}

float LayoutDetails::GetShrinkToFitWidth(Element* element, Vector2f containing_block)
{
	RMLUI_ASSERT(element);

	// @performance Can we lay out the elements directly using a fit-content size mode, instead of fetching the
	// shrink-to-fit width first? Use a non-definite placeholder for the box content width, and available width as a
	// maximum constraint.
	Box box;
	float min_height, max_height;
	LayoutDetails::BuildBox(box, containing_block, element, BuildBoxMode::UnalignedBlock);
	LayoutDetails::GetDefiniteMinMaxHeight(min_height, max_height, element->GetComputedValues(), box, containing_block.y);

	// Currently we don't support shrink-to-fit width for tables. Just return a zero-sized width.
	const Style::Display display = element->GetDisplay();
	if (display == Style::Display::Table || display == Style::Display::InlineTable)
	{
		return 0.f;
	}

	// Use a large size for the box content width, so that it is practically unconstrained. This makes the formatting
	// procedure act as if under a maximum content constraint. Children with percentage sizing values may be scaled
	// based on this width (such as 'width' or 'margin'), if so, the layout is considered undefined like in CSS 2.
	const float max_content_constraint_width = containing_block.x + 10000.f;
	box.SetContent({max_content_constraint_width, box.GetSize().y});

	// First, format the element under the above generated box. Then we ask the resulting box for its shrink-to-fit
	// width. For block containers, this is essentially its largest line or child box.
	// @performance. Some formatting can be simplified, e.g. absolute elements do not contribute to the shrink-to-fit
	// width. Also, children of elements with a fixed width and height don't need to be formatted further.
	RootBox root(Math::Max(containing_block, Vector2f(0.f)));
	UniquePtr<LayoutBox> layout_box = FormattingContext::FormatIndependent(&root, element, &box, FormattingContextType::Block);

	float shrink_to_fit_width = layout_box->GetShrinkToFitWidth();
	if (containing_block.x >= 0)
	{
		const float available_width =
			Math::Max(0.f, containing_block.x - box.GetSizeAcross(BoxDirection::Horizontal, BoxArea::Margin, BoxArea::Padding));
		shrink_to_fit_width = Math::Min(shrink_to_fit_width, available_width);
	}
	return shrink_to_fit_width;
}

ComputedAxisSize LayoutDetails::BuildComputedHorizontalSize(const ComputedValues& computed)
{
	return ComputedAxisSize{computed.width(), computed.min_width(), computed.max_width(), computed.padding_left(), computed.padding_right(),
		computed.margin_left(), computed.margin_right(), computed.border_left_width(), computed.border_right_width(), computed.box_sizing()};
}

ComputedAxisSize LayoutDetails::BuildComputedVerticalSize(const ComputedValues& computed)
{
	return ComputedAxisSize{computed.height(), computed.min_height(), computed.max_height(), computed.padding_top(), computed.padding_bottom(),
		computed.margin_top(), computed.margin_bottom(), computed.border_top_width(), computed.border_bottom_width(), computed.box_sizing()};
}

void LayoutDetails::GetEdgeSizes(float& margin_a, float& margin_b, float& padding_border_a, float& padding_border_b,
	const ComputedAxisSize& computed_size, const float base_value)
{
	margin_a = ResolveValue(computed_size.margin_a, base_value);
	margin_b = ResolveValue(computed_size.margin_b, base_value);

	padding_border_a = Math::Max(0.0f, ResolveValue(computed_size.padding_a, base_value)) + Math::Max(0.0f, computed_size.border_a);
	padding_border_b = Math::Max(0.0f, ResolveValue(computed_size.padding_b, base_value)) + Math::Max(0.0f, computed_size.border_b);
}

String LayoutDetails::GetDebugElementName(Element* element)
{
	if (!element)
		return "nullptr";
	if (!element->GetId().empty())
		return '#' + element->GetId();
	if (auto element_text = rmlui_dynamic_cast<ElementText*>(element))
		return '\"' + StringUtilities::StripWhitespace(element_text->GetText()).substr(0, 20) + '\"';
	return element->GetAddress(false, false);
}

Vector2f LayoutDetails::CalculateSizeForReplacedElement(const Vector2f specified_content_size, const Vector2f min_size, const Vector2f max_size,
	const Vector2f intrinsic_size, const float intrinsic_ratio)
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

void LayoutDetails::BuildBoxWidth(Box& box, const ComputedValues& computed, float min_width, float max_width, Vector2f containing_block,
	Element* element, bool replaced_element, float override_shrink_to_fit_width)
{
	RMLUI_ZoneScoped;

	Vector2f content_area = box.GetSize();

	// Determine if the element has automatic margins.
	bool margins_auto[2];
	int num_auto_margins = 0;

	for (int i = 0; i < 2; ++i)
	{
		const Style::Margin margin_value = (i == 0 ? computed.margin_left() : computed.margin_right());
		if (margin_value.type == Style::Margin::Auto)
		{
			margins_auto[i] = true;
			num_auto_margins++;
			box.SetEdge(BoxArea::Margin, i == 0 ? BoxEdge::Left : BoxEdge::Right, 0);
		}
		else
		{
			margins_auto[i] = false;
			box.SetEdge(BoxArea::Margin, i == 0 ? BoxEdge::Left : BoxEdge::Right, ResolveValue(margin_value, containing_block.x));
		}
	}

	const bool absolutely_positioned = (computed.position() == Style::Position::Absolute || computed.position() == Style::Position::Fixed);
	const bool inset_auto = (computed.left().type == Style::Left::Auto || computed.right().type == Style::Right::Auto);
	const bool width_auto = (content_area.x < 0);

	auto GetInsetWidth = [&] {
		// For absolutely positioned elements (and only those), the 'left' and 'right' values are part of the box's width constraint.
		if (absolutely_positioned)
			return ResolveValue(computed.left(), containing_block.x) + ResolveValue(computed.right(), containing_block.x);
		return 0.f;
	};

	// If the width is set to auto, we need to calculate the width.
	if (width_auto)
	{
		// Apply the shrink-to-fit algorithm here to find the width of the element.
		// See CSS 2.1 section 10.3.7 for when this should be applied.
		const bool shrink_to_fit = !replaced_element &&
			((computed.float_() != Style::Float::None) || (absolutely_positioned && inset_auto) ||
				(computed.display() == Style::Display::InlineBlock || computed.display() == Style::Display::InlineFlex));

		if (!shrink_to_fit)
		{
			// The width is set to whatever remains of the containing block.
			content_area.x = containing_block.x - (GetInsetWidth() + box.GetSizeAcross(BoxDirection::Horizontal, BoxArea::Margin, BoxArea::Padding));
			content_area.x = Math::Max(0.0f, content_area.x);
		}
		else if (override_shrink_to_fit_width >= 0)
		{
			content_area.x = override_shrink_to_fit_width;
		}
		else
		{
			content_area.x = GetShrinkToFitWidth(element, containing_block);
			override_shrink_to_fit_width = content_area.x;
		}
	}
	// Otherwise, the margins that are set to auto will pick up the remaining width of the containing block.
	else if (num_auto_margins > 0)
	{
		const float margin =
			(containing_block.x - (GetInsetWidth() + box.GetSizeAcross(BoxDirection::Horizontal, BoxArea::Margin))) / float(num_auto_margins);

		if (margins_auto[0])
			box.SetEdge(BoxArea::Margin, BoxEdge::Left, margin);
		if (margins_auto[1])
			box.SetEdge(BoxArea::Margin, BoxEdge::Right, margin);
	}

	// Clamp the calculated width; if the width is changed by the clamp, then the margins need to be recalculated if
	// they were set to auto.
	const float clamped_width = Math::Clamp(content_area.x, min_width, max_width);
	if (clamped_width != content_area.x)
	{
		content_area.x = clamped_width;
		box.SetContent(content_area);

		if (num_auto_margins > 0)
			BuildBoxWidth(box, computed, min_width, max_width, containing_block, element, replaced_element, clamped_width);
	}
	else
		box.SetContent(content_area);
}

void LayoutDetails::BuildBoxHeight(Box& box, const ComputedValues& computed, float min_height, float max_height, float containing_block_height)
{
	RMLUI_ZoneScoped;

	Vector2f content_area = box.GetSize();

	// Determine if the element has automatic margins.
	bool margins_auto[2];
	int num_auto_margins = 0;

	for (int i = 0; i < 2; ++i)
	{
		const Style::Margin margin_value = (i == 0 ? computed.margin_top() : computed.margin_bottom());
		if (margin_value.type == Style::Margin::Auto)
		{
			margins_auto[i] = true;
			num_auto_margins++;
			box.SetEdge(BoxArea::Margin, i == 0 ? BoxEdge::Top : BoxEdge::Bottom, 0);
		}
		else
		{
			margins_auto[i] = false;
			box.SetEdge(BoxArea::Margin, i == 0 ? BoxEdge::Top : BoxEdge::Bottom, ResolveValue(margin_value, containing_block_height));
		}
	}

	const bool absolutely_positioned = (computed.position() == Style::Position::Absolute || computed.position() == Style::Position::Fixed);
	const bool inset_auto = (computed.top().type == Style::Top::Auto || computed.bottom().type == Style::Bottom::Auto);
	const bool height_auto = (content_area.y < 0);

	auto GetInsetHeight = [&] {
		// For absolutely positioned elements (and only those), the 'top' and 'bottom' values are part of the box's height constraint.
		if (absolutely_positioned)
			return ResolveValue(computed.top(), containing_block_height) + ResolveValue(computed.bottom(), containing_block_height);
		return 0.f;
	};

	// If the height is set to auto, we need to calculate the height.
	if (height_auto)
	{
		// If the height is set to auto for a box in normal flow, the height is set to -1, representing indefinite height.
		content_area.y = -1;

		// But if we are dealing with an absolutely positioned element we need to consider if the top and bottom
		// properties are set, since the height can be affected.
		if (absolutely_positioned && !inset_auto)
		{
			// The height is set to whatever remains of the containing block.
			content_area.y =
				containing_block_height - (GetInsetHeight() + box.GetSizeAcross(BoxDirection::Vertical, BoxArea::Margin, BoxArea::Padding));
			content_area.y = Math::Max(0.0f, content_area.y);
		}
	}
	// Otherwise, the margins that are set to auto will pick up the remaining height of the containing block.
	else if (num_auto_margins > 0)
	{
		const float margin =
			(containing_block_height - (GetInsetHeight() + box.GetSizeAcross(BoxDirection::Vertical, BoxArea::Margin))) / float(num_auto_margins);

		if (margins_auto[0])
			box.SetEdge(BoxArea::Margin, BoxEdge::Top, margin);
		if (margins_auto[1])
			box.SetEdge(BoxArea::Margin, BoxEdge::Bottom, margin);
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
