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

#include "InlineBox.h"
#include "../../../Include/RmlUi/Core/Box.h"
#include "../../../Include/RmlUi/Core/Element.h"
#include "../../../Include/RmlUi/Core/FontMetrics.h"

namespace Rml {

static void ZeroBoxEdge(Box& box, BoxEdge edge)
{
	box.SetEdge(BoxArea::Padding, edge, 0.f);
	box.SetEdge(BoxArea::Border, edge, 0.f);
	box.SetEdge(BoxArea::Margin, edge, 0.f);
}

InlineLevelBox* InlineBoxBase::AddChild(UniquePtr<InlineLevelBox> child)
{
	auto result = child.get();
	children.push_back(std::move(child));
	return result;
}

void InlineBoxBase::GetStrut(float& out_total_height_above, float& out_total_depth_below) const
{
	const FontMetrics& font_metrics = GetFontMetrics();
	const float line_height = GetElement()->GetLineHeight();

	const float half_leading = 0.5f * (line_height - (font_metrics.ascent + font_metrics.descent));
	out_total_height_above = font_metrics.ascent + half_leading;
	out_total_depth_below = line_height - out_total_height_above;
}

String InlineBoxBase::DebugDumpTree(int depth) const
{
	String value = InlineLevelBox::DebugDumpTree(depth);
	for (auto&& child : children)
		value += child->DebugDumpTree(depth + 1);
	return value;
}

InlineBoxBase::InlineBoxBase(Element* element) : InlineLevelBox(element) {}

InlineBoxRoot::InlineBoxRoot(Element* element) : InlineBoxBase(element)
{
	float height_above_baseline, depth_below_baseline;
	GetStrut(height_above_baseline, depth_below_baseline);
	SetHeight(height_above_baseline, depth_below_baseline);
}

FragmentConstructor InlineBoxRoot::CreateFragment(InlineLayoutMode /*mode*/, float /*available_width*/, float /*right_spacing_width*/,
	bool /*first_box*/, LayoutOverflowHandle /*overflow_handle*/)
{
	RMLUI_ERROR;
	return {};
}

void InlineBoxRoot::Submit(const PlacedFragment& /*placed_fragment*/)
{
	RMLUI_ERROR;
}

InlineBox::InlineBox(const InlineLevelBox* parent, Element* element, const Box& _box) : InlineBoxBase(element), box(_box)
{
	RMLUI_ASSERT(box.GetSize().x < 0.f && box.GetSize().y < 0.f);

	const FontMetrics& font_metrics = GetFontMetrics();

	// The inline box content height does not depend on the 'line-height' property, only on the font, and is not exactly
	// specified by CSS. Here we choose to size the content height equal to the default line-height for the font-size.
	const float inner_height = 1.2f * (float)font_metrics.size;
	box.SetContent(Vector2f(-1.f, inner_height));

	float height_above_baseline, depth_below_baseline;
	GetStrut(height_above_baseline, depth_below_baseline);
	SetHeightAndVerticalAlignment(height_above_baseline, depth_below_baseline, parent);

	const float edge_left = box.GetCumulativeEdge(BoxArea::Padding, BoxEdge::Left);
	const float edge_right = box.GetCumulativeEdge(BoxArea::Padding, BoxEdge::Right);
	SetInlineBoxSpacing(edge_left, edge_right);

	// Vertically position the box so that its content box is equally spaced around its font ascent and descent metrics.
	const float half_leading = 0.5f * (inner_height - (font_metrics.ascent + font_metrics.descent));
	baseline_to_border_height =
		font_metrics.ascent + half_leading + box.GetEdge(BoxArea::Border, BoxEdge::Top) + box.GetEdge(BoxArea::Padding, BoxEdge::Top);
}

FragmentConstructor InlineBox::CreateFragment(InlineLayoutMode mode, float available_width, float right_spacing_width, bool /*first_box*/,
	LayoutOverflowHandle /*overflow_handle*/)
{
	if (mode != InlineLayoutMode::WrapAny || right_spacing_width <= available_width + GetSpacingLeft())
		return FragmentConstructor{FragmentType::InlineBox, -1.f, {}, {}};

	return {};
}

void InlineBox::Submit(const PlacedFragment& placed_fragment)
{
	Element* element = GetElement();
	RMLUI_ASSERT(element && element != placed_fragment.offset_parent);

	Box element_box = box;
	element_box.SetContent({placed_fragment.layout_width, element_box.GetSize().y});

	if (placed_fragment.split_left)
		ZeroBoxEdge(element_box, BoxEdge::Left);
	if (placed_fragment.split_right)
		ZeroBoxEdge(element_box, BoxEdge::Right);

	// In inline layout, fragments are positioned in terms of (x: margin edge, y: baseline), while element offsets are
	// specified relative to their border box. Thus, find the offset from the fragment position to the border edge.
	const Vector2f border_position =
		placed_fragment.position + Vector2f{element_box.GetEdge(BoxArea::Margin, BoxEdge::Left), -baseline_to_border_height};

	// We can determine the principal fragment based on its left split: Only the principal one has its left side intact,
	// subsequent fragments have their left side split.
	const bool principal_box = !placed_fragment.split_left;

	if (principal_box)
	{
		element_offset = border_position;
		element->SetOffset(border_position, placed_fragment.offset_parent);
		element->SetBox(element_box);
		SubmitElementOnLayout();
	}
	else
	{
		element->AddBox(element_box, border_position - element_offset);
	}
}

} // namespace Rml
