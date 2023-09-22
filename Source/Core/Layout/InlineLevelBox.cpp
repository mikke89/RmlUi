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

#include "InlineLevelBox.h"
#include "../../../Include/RmlUi/Core/ComputedValues.h"
#include "../../../Include/RmlUi/Core/Core.h"
#include "../../../Include/RmlUi/Core/ElementText.h"
#include "../../../Include/RmlUi/Core/FontEngineInterface.h"
#include "LayoutDetails.h"
#include "LayoutPools.h"

namespace Rml {

void* InlineLevelBox::operator new(size_t size)
{
	return LayoutPools::AllocateLayoutChunk(size);
}

void InlineLevelBox::operator delete(void* chunk, size_t size)
{
	LayoutPools::DeallocateLayoutChunk(chunk, size);
}

InlineLevelBox::~InlineLevelBox() {}

void InlineLevelBox::SubmitElementOnLayout()
{
	element->OnLayout();
}

const FontMetrics& InlineLevelBox::GetFontMetrics() const
{
	if (FontFaceHandle handle = element->GetFontFaceHandle())
		return GetFontEngineInterface()->GetFontMetrics(handle);

	// If there is no font face defined then we provide zero'd out font metrics. This situation can affect the layout,
	// in particular in terms of inline box sizing and vertical alignment. Thus, this is potentially a situation where
	// we might want to log a warning. However, in many cases it will produce the same layout with or without the font,
	// so in that sense the warnings can produce false positives.
	//
	// For now, we wait until we try to actually place text before producing any warnings, since that is a clear
	// erroneous situation producing no text. See 'LogMissingFontFace' in ElementText.cpp, which also lists some
	// possible reasons for the missing font face.

	static const FontMetrics font_metrics = {};
	return font_metrics;
}

void InlineLevelBox::SetHeightAndVerticalAlignment(float _height_above_baseline, float _depth_below_baseline, const InlineLevelBox* parent)
{
	RMLUI_ASSERT(parent);
	using Style::VerticalAlign;

	SetHeight(_height_above_baseline, _depth_below_baseline);

	const Style::VerticalAlign vertical_align = element->GetComputedValues().vertical_align();
	vertical_align_type = vertical_align.type;

	// Determine the offset from the parent baseline.
	float parent_baseline_offset = 0.f; // The anchor on the parent, as an offset from its baseline.
	float self_baseline_offset = 0.f;   // The offset from the parent anchor to our baseline.

	switch (vertical_align.type)
	{
	case VerticalAlign::Baseline: parent_baseline_offset = 0.f; break;
	case VerticalAlign::Length: parent_baseline_offset = -vertical_align.value; break;
	case VerticalAlign::Sub: parent_baseline_offset = (1.f / 5.f) * (float)parent->GetFontMetrics().size; break;
	case VerticalAlign::Super: parent_baseline_offset = (-1.f / 3.f) * (float)parent->GetFontMetrics().size; break;
	case VerticalAlign::TextTop:
		parent_baseline_offset = -parent->GetFontMetrics().ascent;
		self_baseline_offset = height_above_baseline;
		break;
	case VerticalAlign::TextBottom:
		parent_baseline_offset = parent->GetFontMetrics().descent;
		self_baseline_offset = -depth_below_baseline;
		break;
	case VerticalAlign::Middle:
		parent_baseline_offset = -0.5f * parent->GetFontMetrics().x_height;
		self_baseline_offset = 0.5f * (height_above_baseline - depth_below_baseline);
		break;
	case VerticalAlign::Top:
	case VerticalAlign::Center:
	case VerticalAlign::Bottom:
		// These are relative to the line box and handled later.
		break;
	}

	vertical_offset_from_parent = parent_baseline_offset + self_baseline_offset;
}

void InlineLevelBox::SetHeight(float _height_above_baseline, float _depth_below_baseline)
{
	height_above_baseline = _height_above_baseline;
	depth_below_baseline = _depth_below_baseline;
}

void InlineLevelBox::SetInlineBoxSpacing(float _spacing_left, float _spacing_right)
{
	spacing_left = _spacing_left;
	spacing_right = _spacing_right;
}

String InlineLevelBox::DebugDumpTree(int depth) const
{
	String value = String(depth * 2, ' ') + DebugDumpNameValue() + " | " + LayoutDetails::GetDebugElementName(GetElement()) + '\n';
	return value;
}

InlineLevelBox_Atomic::InlineLevelBox_Atomic(const InlineLevelBox* parent, Element* element, const Box& box) : InlineLevelBox(element), box(box)
{
	RMLUI_ASSERT(parent && element);
	RMLUI_ASSERT(box.GetSize().x >= 0.f && box.GetSize().y >= 0.f);

	const float outer_height = box.GetSizeAcross(BoxDirection::Vertical, BoxArea::Margin);

	const float descent = GetElement()->GetBaseline();
	const float ascent = outer_height - descent;
	SetHeightAndVerticalAlignment(ascent, descent, parent);
}

FragmentConstructor InlineLevelBox_Atomic::CreateFragment(InlineLayoutMode mode, float available_width, float right_spacing_width, bool /*first_box*/,
	LayoutOverflowHandle /*overflow_handle*/)
{
	const float outer_width = box.GetSizeAcross(BoxDirection::Horizontal, BoxArea::Margin);

	if (mode != InlineLayoutMode::WrapAny || outer_width + right_spacing_width <= available_width)
		return FragmentConstructor{FragmentType::SizedBox, outer_width, {}, {}};

	return {};
}

void InlineLevelBox_Atomic::Submit(const PlacedFragment& placed_fragment)
{
	const Vector2f margin_position = {placed_fragment.position.x, placed_fragment.position.y - GetHeightAboveBaseline()};
	const Vector2f margin_edge = {box.GetEdge(BoxArea::Margin, BoxEdge::Left), box.GetEdge(BoxArea::Margin, BoxEdge::Top)};
	const Vector2f border_position = margin_position + margin_edge;

	GetElement()->SetOffset(border_position, placed_fragment.offset_parent);
	GetElement()->SetBox(box);
	SubmitElementOnLayout();
}

InlineLevelBox_Text::InlineLevelBox_Text(ElementText* element) : InlineLevelBox(element) {}

FragmentConstructor InlineLevelBox_Text::CreateFragment(InlineLayoutMode mode, float available_width, float right_spacing_width, bool first_box,
	LayoutOverflowHandle in_overflow_handle)
{
	ElementText* text_element = GetTextElement();

	const bool allow_empty = (mode == InlineLayoutMode::WrapAny);
	const bool decode_escape_characters = true;

	String line_contents;
	int line_begin = in_overflow_handle;
	int line_length = 0;
	float line_width = 0.f;
	bool overflow = !text_element->GenerateLine(line_contents, line_length, line_width, line_begin, available_width, right_spacing_width, first_box,
		decode_escape_characters, allow_empty);

	if (overflow && line_contents.empty())
		// We couldn't fit anything on this line.
		return {};

	LayoutOverflowHandle out_overflow_handle = {};
	if (overflow)
		out_overflow_handle = line_begin + line_length;

	LayoutFragmentHandle fragment_handle = (LayoutFragmentHandle)fragments.size();
	fragments.push_back(std::move(line_contents));

	return FragmentConstructor{FragmentType::TextRun, line_width, fragment_handle, out_overflow_handle};
}

void InlineLevelBox_Text::Submit(const PlacedFragment& placed_fragment)
{
	RMLUI_ASSERT((size_t)placed_fragment.handle < fragments.size());

	const int fragment_index = (int)placed_fragment.handle;
	const bool principal_box = (fragment_index == 0);

	ElementText* text_element = GetTextElement();
	Vector2f line_offset;

	if (principal_box)
	{
		element_offset = placed_fragment.position;
		text_element->SetOffset(placed_fragment.position, placed_fragment.offset_parent);
		text_element->ClearLines();
	}
	else
	{
		line_offset = placed_fragment.position - element_offset;
	}

	text_element->AddLine(line_offset, std::move(fragments[fragment_index]));
}

String InlineLevelBox_Text::DebugDumpNameValue() const
{
	return "InlineLevelBox_Text";
}

ElementText* InlineLevelBox_Text::GetTextElement()
{
	return rmlui_static_cast<ElementText*>(GetElement());
}
} // namespace Rml
