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

#include "../../Include/RmlUi/Core/ElementScroll.h"
#include "LayoutDetails.h"
#include "WidgetScroll.h"
#include "../../Include/RmlUi/Core/Context.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../Include/RmlUi/Core/Event.h"
#include "../../Include/RmlUi/Core/Factory.h"

namespace Rml {

ElementScroll::ElementScroll(Element* _element)
{
	element = _element;
	corner = nullptr;
}

ElementScroll::~ElementScroll()
{}

// Updates the increment / decrement arrows.
void ElementScroll::Update()
{
	for (int i = 0; i < 2; i++)
	{
		if (scrollbars[i].widget != nullptr)
			scrollbars[i].widget->Update();
	}
}

// Enables and sizes one of the scrollbars.
void ElementScroll::EnableScrollbar(Orientation orientation, float element_width)
{
	if (!scrollbars[orientation].enabled)
	{
		CreateScrollbar(orientation);
		scrollbars[orientation].element->SetProperty(PropertyId::Visibility, Property(Style::Visibility::Visible));
		scrollbars[orientation].enabled = true;
	}

	// Determine the size of the scrollbar.
	Box box;
	LayoutDetails::BuildBox(box, Vector2f(element_width, element_width), scrollbars[orientation].element);

	if (orientation == VERTICAL)
		scrollbars[orientation].size = box.GetSize(Box::MARGIN).x;
	if (orientation == HORIZONTAL)
	{
		if (box.GetSize().y < 0)
			scrollbars[orientation].size = box.GetCumulativeEdge(Box::CONTENT, Box::LEFT) +
										   box.GetCumulativeEdge(Box::CONTENT, Box::RIGHT) +
										   ResolveValue(scrollbars[orientation].element->GetComputedValues().height, element_width);
		else
			scrollbars[orientation].size = box.GetSize(Box::MARGIN).y;
	}
}

// Disables and hides one of the scrollbars.
void ElementScroll::DisableScrollbar(Orientation orientation)
{
	if (scrollbars[orientation].enabled)
	{
		scrollbars[orientation].element->SetProperty(PropertyId::Visibility, Property(Style::Visibility::Hidden));
		scrollbars[orientation].enabled = false;
	}
}

// Updates the position of the scrollbar.
void ElementScroll::UpdateScrollbar(Orientation orientation)
{
	float bar_position;
	float traversable_track;
	if (orientation == VERTICAL)
	{
		bar_position = element->GetScrollTop();
		traversable_track = element->GetScrollHeight() - element->GetClientHeight();
	}
	else
	{
		bar_position = element->GetScrollLeft();
		traversable_track = element->GetScrollWidth() - element->GetClientWidth();
	}

	if (traversable_track <= 0)
		bar_position = 0;
	else
		bar_position /= traversable_track;

	if (scrollbars[orientation].widget != nullptr)
	{
		bar_position = Math::Clamp(bar_position, 0.0f, 1.0f);

		if (scrollbars[orientation].widget->GetBarPosition() != bar_position)
			scrollbars[orientation].widget->SetBarPosition(bar_position);
	}
}

// Returns one of the scrollbar elements.
Element* ElementScroll::GetScrollbar(Orientation orientation)
{
	return scrollbars[orientation].element;
}

// Returns the size, in pixels, of one of the scrollbars; for a vertical scrollbar, this is width, for a horizontal scrollbar, this is height.
float ElementScroll::GetScrollbarSize(Orientation orientation)
{
	if (!scrollbars[orientation].enabled)
		return 0;

	return scrollbars[orientation].size;
}

// Formats the enabled scrollbars based on the current size of the host element.
void ElementScroll::FormatScrollbars()
{
	const Box& element_box = element->GetBox();
	const Vector2f containing_block = element_box.GetSize(Box::PADDING);

	for (int i = 0; i < 2; i++)
	{
		if (!scrollbars[i].enabled)
			continue;

		if (i == VERTICAL)
		{
			scrollbars[i].widget->SetBarLength(element->GetClientHeight());
			scrollbars[i].widget->SetTrackLength(element->GetScrollHeight());

			float traversable_track = element->GetScrollHeight() - element->GetClientHeight();
			if (traversable_track > 0)
				scrollbars[i].widget->SetBarPosition(element->GetScrollTop() / traversable_track);
			else
				scrollbars[i].widget->SetBarPosition(0);
		}
		else
		{
			scrollbars[i].widget->SetBarLength(element->GetClientWidth());
			scrollbars[i].widget->SetTrackLength(element->GetScrollWidth());

			float traversable_track = element->GetScrollWidth() - element->GetClientWidth();
			if (traversable_track > 0)
				scrollbars[i].widget->SetBarPosition(element->GetScrollLeft() / traversable_track);
			else
				scrollbars[i].widget->SetBarPosition(0);
		}

		float slider_length = containing_block[1 - i];
		float user_scrollbar_margin = scrollbars[i].element->GetComputedValues().scrollbar_margin;
		float min_scrollbar_margin = GetScrollbarSize(i == VERTICAL ? HORIZONTAL : VERTICAL);
		slider_length -= Math::Max(user_scrollbar_margin, min_scrollbar_margin);

		scrollbars[i].widget->FormatElements(containing_block, slider_length);
		scrollbars[i].widget->SetLineHeight(element->GetLineHeight());

		int variable_axis = i == VERTICAL ? 0 : 1;
		Vector2f offset = element_box.GetPosition(Box::PADDING);
		offset[variable_axis] += containing_block[variable_axis] - (scrollbars[i].element->GetBox().GetSize(Box::BORDER)[variable_axis] + scrollbars[i].element->GetBox().GetEdge(Box::MARGIN, i == VERTICAL ? Box::RIGHT : Box::BOTTOM));
		// Add the top or left margin (as appropriate) onto the scrollbar's position.
		offset[1 - variable_axis] += scrollbars[i].element->GetBox().GetEdge(Box::MARGIN, i == VERTICAL ? Box::TOP : Box::LEFT);
		scrollbars[i].element->SetOffset(offset, element, true);
	}

	// Format the corner, if it is necessary.
	if (scrollbars[0].enabled &&
		scrollbars[1].enabled)
	{
		CreateCorner();

		Box corner_box;
		corner_box.SetContent(Vector2f(scrollbars[VERTICAL].size, scrollbars[HORIZONTAL].size));
		corner->SetBox(corner_box);
		corner->SetOffset(containing_block + element_box.GetPosition(Box::PADDING) - Vector2f(scrollbars[VERTICAL].size, scrollbars[HORIZONTAL].size), element, true);
		corner->SetProperty(PropertyId::Clip, Property(1, Property::NUMBER));

		corner->SetProperty(PropertyId::Visibility, Property(Style::Visibility::Visible));
	}
	else
	{
		if (corner != nullptr)
			corner->SetProperty(PropertyId::Visibility, Property(Style::Visibility::Hidden));
	}
}


// Creates one of the scroll component's scrollbar.
bool ElementScroll::CreateScrollbar(Orientation orientation)
{
	if (scrollbars[orientation].element &&
		scrollbars[orientation].widget)
		return true;

	ElementPtr scrollbar_element = Factory::InstanceElement(element, "*", orientation == VERTICAL ? "scrollbarvertical" : "scrollbarhorizontal", XMLAttributes());
	scrollbars[orientation].element = scrollbar_element.get();
	scrollbars[orientation].element->SetProperty(PropertyId::Clip, Property(1, Property::NUMBER));

	scrollbars[orientation].widget = MakeUnique<WidgetScroll>(scrollbars[orientation].element);
	scrollbars[orientation].widget->Initialise(orientation == VERTICAL ? WidgetScroll::VERTICAL : WidgetScroll::HORIZONTAL);

	Element* child = element->AppendChild(std::move(scrollbar_element), false);

	// The construction of scrollbars can occur during layouting, then we need some properties and computed values straight away.
	Context* context = element->GetContext();
	
	const float dp_ratio = (context ? context->GetDensityIndependentPixelRatio() : 1.0f);
	const Vector2f vp_dimensions = (context ? Vector2f(context->GetDimensions()) : Vector2f(1.0f));
	child->Update(dp_ratio, vp_dimensions);

	return true;
}

// Creates the scrollbar corner.
bool ElementScroll::CreateCorner()
{
	if (corner != nullptr)
		return true;

	ElementPtr corner_element = Factory::InstanceElement(element, "*", "scrollbarcorner", XMLAttributes());
	corner = corner_element.get();
	element->AppendChild(std::move(corner_element), false);

	return true;
}

ElementScroll::Scrollbar::Scrollbar()
{}

ElementScroll::Scrollbar::~Scrollbar()
{}

} // namespace Rml
