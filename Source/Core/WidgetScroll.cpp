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

#include "WidgetScroll.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Context.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../Include/RmlUi/Core/Event.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/Property.h"
#include "Clock.h"
#include "Layout/LayoutDetails.h"
#include <algorithm>

namespace Rml {

static constexpr float DEFAULT_REPEAT_DELAY = 0.5f;
static constexpr float DEFAULT_REPEAT_PERIOD = 0.1f;

static constexpr float SCROLL_LINE_LENGTH = 30.f; // [dp]
static constexpr float SCROLL_PAGE_FACTOR = 0.8f;

WidgetScroll::WidgetScroll(Element* _parent)
{
	parent = _parent;

	orientation = UNKNOWN;

	track = nullptr;
	bar = nullptr;
	arrows[0] = nullptr;
	arrows[1] = nullptr;

	bar_position = 0;
	bar_drag_anchor = 0;

	arrow_timers[0] = -1;
	arrow_timers[1] = -1;
	last_update_time = 0;

	track_length = 0;
	bar_length = 0;
}

WidgetScroll::~WidgetScroll()
{
	if (bar != nullptr)
	{
		bar->RemoveEventListener(EventId::Drag, this);
		bar->RemoveEventListener(EventId::Dragstart, this);
	}

	if (track != nullptr)
		track->RemoveEventListener(EventId::Click, this);

	for (int i = 0; i < 2; i++)
	{
		if (arrows[i] != nullptr)
		{
			arrows[i]->RemoveEventListener(EventId::Mousedown, this);
			arrows[i]->RemoveEventListener(EventId::Mouseup, this);
			arrows[i]->RemoveEventListener(EventId::Mouseout, this);
		}
	}
}

bool WidgetScroll::Initialise(Orientation _orientation)
{
	// Check that we haven't already been successfully initialised.
	if (orientation != UNKNOWN)
	{
		RMLUI_ERROR;
		return false;
	}

	// Check that a valid orientation has been passed in.
	if (_orientation != HORIZONTAL && _orientation != VERTICAL)
	{
		RMLUI_ERROR;
		return false;
	}

	orientation = _orientation;

	// Create all of our child elements as standard elements, and abort if we can't create them.
	ElementPtr track_element = Factory::InstanceElement(parent, "*", "slidertrack", XMLAttributes());
	ElementPtr bar_element = Factory::InstanceElement(parent, "*", "sliderbar", XMLAttributes());
	ElementPtr arrow0_element = Factory::InstanceElement(parent, "*", "sliderarrowdec", XMLAttributes());
	ElementPtr arrow1_element = Factory::InstanceElement(parent, "*", "sliderarrowinc", XMLAttributes());

	if (!track_element || !bar_element || !arrow0_element || !arrow1_element)
	{
		return false;
	}

	// Add them as non-DOM elements.
	track = parent->AppendChild(std::move(track_element), false);
	bar = parent->AppendChild(std::move(bar_element), false);
	arrows[0] = parent->AppendChild(std::move(arrow0_element), false);
	arrows[1] = parent->AppendChild(std::move(arrow1_element), false);

	bar->SetProperty(PropertyId::Drag, Property(Style::Drag::Drag));

	// Attach the listeners as appropriate.
	bar->AddEventListener(EventId::Drag, this);
	bar->AddEventListener(EventId::Dragstart, this);

	track->AddEventListener(EventId::Click, this);

	for (int i = 0; i < 2; i++)
	{
		arrows[i]->AddEventListener(EventId::Mousedown, this);
		arrows[i]->AddEventListener(EventId::Mouseup, this);
		arrows[i]->AddEventListener(EventId::Mouseout, this);
	}

	return true;
}

void WidgetScroll::Update()
{
	if (!std::any_of(std::begin(arrow_timers), std::end(arrow_timers), [](float timer) { return timer > 0; }))
		return;

	double current_time = Clock::GetElapsedTime();
	const float delta_time = float(current_time - last_update_time);
	last_update_time = current_time;

	for (int i = 0; i < 2; i++)
	{
		if (arrow_timers[i] > 0)
		{
			arrow_timers[i] -= delta_time;
			while (arrow_timers[i] <= 0)
			{
				arrow_timers[i] += DEFAULT_REPEAT_PERIOD;
				if (i == 0)
					ScrollLineUp();
				else
					ScrollLineDown();
			}

			if (Context* ctx = parent->GetContext())
				ctx->RequestNextUpdate(arrow_timers[i]);
		}
	}
}

void WidgetScroll::SetBarPosition(float _bar_position)
{
	bar_position = Math::Clamp(_bar_position, 0.0f, 1.0f);
	PositionBar();
}

float WidgetScroll::GetBarPosition() const
{
	return bar_position;
}

WidgetScroll::Orientation WidgetScroll::GetOrientation() const
{
	return orientation;
}

void WidgetScroll::GetDimensions(Vector2f& dimensions) const
{
	switch (orientation)
	{
	case VERTICAL:
		dimensions.x = 16;
		dimensions.y = 256;
		break;
	case HORIZONTAL:
		dimensions.x = 256;
		dimensions.y = 16;
		break;
	case UNKNOWN: break;
	}
}

void WidgetScroll::FormatElements(const Vector2f containing_block, float slider_length)
{
	int length_axis = orientation == VERTICAL ? 1 : 0;

	// Build the box for the containing slider element.
	Box parent_box;
	LayoutDetails::BuildBox(parent_box, containing_block, parent);
	slider_length -= orientation == VERTICAL
		? (parent_box.GetCumulativeEdge(BoxArea::Content, BoxEdge::Top) + parent_box.GetCumulativeEdge(BoxArea::Content, BoxEdge::Bottom))
		: (parent_box.GetCumulativeEdge(BoxArea::Content, BoxEdge::Left) + parent_box.GetCumulativeEdge(BoxArea::Content, BoxEdge::Right));

	// Set the length of the slider.
	Vector2f content = parent_box.GetSize();
	content[length_axis] = slider_length;
	parent_box.SetContent(content);
	// And set it on the slider element!
	parent->SetBox(parent_box);

	// Generate the initial dimensions for the track. It'll need to be cut down to fit the arrows.
	Box track_box;
	LayoutDetails::BuildBox(track_box, parent_box.GetSize(), track);
	content = track_box.GetSize();
	content[length_axis] = slider_length -= orientation == VERTICAL
		? (track_box.GetCumulativeEdge(BoxArea::Content, BoxEdge::Top) + track_box.GetCumulativeEdge(BoxArea::Content, BoxEdge::Bottom))
		: (track_box.GetCumulativeEdge(BoxArea::Content, BoxEdge::Left) + track_box.GetCumulativeEdge(BoxArea::Content, BoxEdge::Right));
	// If no height has been explicitly specified for the track, it'll be initialised to -1 as per normal block
	// elements. We'll fix that up here.
	if (orientation == HORIZONTAL && content.y < 0)
		content.y = parent_box.GetSize().y;

	// Now we size the arrows.
	for (int i = 0; i < 2; i++)
	{
		Box arrow_box;
		LayoutDetails::BuildBox(arrow_box, parent_box.GetSize(), arrows[i]);

		// Clamp the size to (0, 0).
		Vector2f arrow_size = arrow_box.GetSize();
		if (arrow_size.x < 0 || arrow_size.y < 0)
			arrow_box.SetContent(Vector2f(0, 0));

		arrows[i]->SetBox(arrow_box);

		// Shrink the track length by the arrow size.
		content[length_axis] -= arrow_box.GetSize(BoxArea::Margin)[length_axis];
	}

	// Now the track has been sized, we can fix everything into position.
	track_box.SetContent(content);
	track->SetBox(track_box);

	if (orientation == VERTICAL)
	{
		Vector2f offset(arrows[0]->GetBox().GetEdge(BoxArea::Margin, BoxEdge::Left), arrows[0]->GetBox().GetEdge(BoxArea::Margin, BoxEdge::Top));
		arrows[0]->SetOffset(offset, parent);

		offset.x = track->GetBox().GetEdge(BoxArea::Margin, BoxEdge::Left);
		offset.y += arrows[0]->GetBox().GetSize(BoxArea::Border).y + arrows[0]->GetBox().GetEdge(BoxArea::Margin, BoxEdge::Bottom) +
			track->GetBox().GetEdge(BoxArea::Margin, BoxEdge::Top);
		track->SetOffset(offset, parent);

		offset.x = arrows[1]->GetBox().GetEdge(BoxArea::Margin, BoxEdge::Left);
		offset.y += track->GetBox().GetSize(BoxArea::Border).y + track->GetBox().GetEdge(BoxArea::Margin, BoxEdge::Bottom) +
			arrows[1]->GetBox().GetEdge(BoxArea::Margin, BoxEdge::Top);
		arrows[1]->SetOffset(offset, parent);
	}
	else
	{
		Vector2f offset(arrows[0]->GetBox().GetEdge(BoxArea::Margin, BoxEdge::Left), arrows[0]->GetBox().GetEdge(BoxArea::Margin, BoxEdge::Top));
		arrows[0]->SetOffset(offset, parent);

		offset.x += arrows[0]->GetBox().GetSize(BoxArea::Border).x + arrows[0]->GetBox().GetEdge(BoxArea::Margin, BoxEdge::Right) +
			track->GetBox().GetEdge(BoxArea::Margin, BoxEdge::Left);
		offset.y = track->GetBox().GetEdge(BoxArea::Margin, BoxEdge::Top);
		track->SetOffset(offset, parent);

		offset.x += track->GetBox().GetSize(BoxArea::Border).x + track->GetBox().GetEdge(BoxArea::Margin, BoxEdge::Right) +
			arrows[1]->GetBox().GetEdge(BoxArea::Margin, BoxEdge::Left);
		offset.y = arrows[1]->GetBox().GetEdge(BoxArea::Margin, BoxEdge::Top);
		arrows[1]->SetOffset(offset, parent);
	}

	FormatBar();
}

void WidgetScroll::FormatBar()
{
	Box bar_box;
	LayoutDetails::BuildBox(bar_box, parent->GetBox().GetSize(), bar);

	const auto& computed = bar->GetComputedValues();

	const Style::Width width = computed.width();
	const Style::Height height = computed.height();

	Vector2f bar_box_content = bar_box.GetSize();
	if (orientation == HORIZONTAL)
	{
		if (height.type == height.Auto)
			bar_box_content.y = parent->GetBox().GetSize().y;
	}

	float relative_bar_length;
	if (track_length <= 0)
		relative_bar_length = 1;
	else if (bar_length <= 0)
		relative_bar_length = 0;
	else
		relative_bar_length = bar_length / track_length;

	Vector2f track_size = track->GetBox().GetSize();

	if (orientation == VERTICAL)
	{
		float track_length = track_size.y - bar_box.GetSizeAcross(BoxDirection::Vertical, BoxArea::Margin, BoxArea::Padding);

		if (height.type == height.Auto)
		{
			bar_box_content.y = track_length * relative_bar_length;

			// Check for 'min-height' restrictions.
			float min_track_length = ResolveValue(computed.min_height(), track_length);
			bar_box_content.y = Math::Max(min_track_length, bar_box_content.y);

			// Check for 'max-height' restrictions.
			float max_track_length = ResolveValue(computed.max_height(), track_length);
			bar_box_content.y = Math::Min(max_track_length, bar_box_content.y);
		}

		// Make sure we haven't gone further than we're allowed to (min-height may have made us too big).
		bar_box_content.y = Math::Min(bar_box_content.y, track_length);
	}
	else
	{
		float track_length = track_size.x - bar_box.GetSizeAcross(BoxDirection::Horizontal, BoxArea::Margin, BoxArea::Padding);

		if (width.type == width.Auto)
		{
			bar_box_content.x = track_length * relative_bar_length;

			// Check for 'min-width' restrictions.
			float min_track_length = ResolveValue(computed.min_width(), track_length);
			bar_box_content.x = Math::Max(min_track_length, bar_box_content.x);

			// Check for 'max-width' restrictions.
			float max_track_length = ResolveValue(computed.max_width(), track_length);
			bar_box_content.x = Math::Min(max_track_length, bar_box_content.x);
		}

		// Make sure we haven't gone further than we're allowed to (min-width may have made us too big).
		bar_box_content.x = Math::Min(bar_box_content.x, track_length);
	}

	// Set the new dimensions on the bar to re-decorate it.
	bar_box.SetContent(bar_box_content.Round());
	bar->SetBox(bar_box);

	// Now that it's been resized, re-position it.
	PositionBar();
}

void WidgetScroll::ProcessEvent(Event& event)
{
	if (event.GetTargetElement() == bar)
	{
		if (event == EventId::Drag)
		{
			float new_bar_position = 0.f;
			if (orientation == HORIZONTAL)
			{
				float traversable_track_length = track->GetBox().GetSize().x - bar->GetBox().GetSize().x;
				if (traversable_track_length > 0)
				{
					float traversable_track_origin = track->GetAbsoluteOffset().x + bar_drag_anchor;
					new_bar_position = (event.GetParameter("mouse_x", 0.f) - traversable_track_origin) / traversable_track_length;
				}
			}
			else
			{
				float traversable_track_length = track->GetBox().GetSize().y - bar->GetBox().GetSize().y;
				if (traversable_track_length > 0)
				{
					float traversable_track_origin = track->GetAbsoluteOffset().y + bar_drag_anchor;
					new_bar_position = (event.GetParameter("mouse_y", 0.f) - traversable_track_origin) / traversable_track_length;
				}
			}

			SetBarPosition(new_bar_position);
			Scroll(0.f, ScrollBehavior::Instant);
		}
		else if (event == EventId::Dragstart)
		{
			if (orientation == HORIZONTAL)
				bar_drag_anchor = event.GetParameter("mouse_x", 0.f) - bar->GetAbsoluteOffset().x;
			else
				bar_drag_anchor = event.GetParameter("mouse_y", 0.f) - bar->GetAbsoluteOffset().y;
		}
	}
	else if (event.GetTargetElement() == track)
	{
		if (event == EventId::Click)
		{
			float click_position = 0.f;
			if (orientation == HORIZONTAL)
			{
				float mouse_position = event.GetParameter("mouse_x", 0.f);
				click_position = (mouse_position - track->GetAbsoluteOffset().x) / track->GetBox().GetSize().x;
			}
			else
			{
				float mouse_position = event.GetParameter<float>("mouse_y", 0);
				click_position = (mouse_position - track->GetAbsoluteOffset().y) / track->GetBox().GetSize().y;
			}

			if (click_position <= bar_position)
				ScrollPageUp();
			else
				ScrollPageDown();
		}
	}

	if (event == EventId::Mousedown)
	{
		if (event.GetTargetElement() == arrows[0])
		{
			arrow_timers[0] = DEFAULT_REPEAT_DELAY;
			last_update_time = Clock::GetElapsedTime();
			ScrollLineUp();
		}
		else if (event.GetTargetElement() == arrows[1])
		{
			arrow_timers[1] = DEFAULT_REPEAT_DELAY;
			last_update_time = Clock::GetElapsedTime();
			ScrollLineDown();
		}
	}
	else if (event == EventId::Mouseup || event == EventId::Mouseout)
	{
		if (event.GetTargetElement() == arrows[0])
			arrow_timers[0] = -1;
		else if (event.GetTargetElement() == arrows[1])
			arrow_timers[1] = -1;
	}
}

void WidgetScroll::PositionBar()
{
	const Vector2f track_dimensions = track->GetBox().GetSize();
	const Vector2f bar_dimensions = bar->GetBox().GetSize(BoxArea::Border);

	if (orientation == VERTICAL)
	{
		float traversable_track_length = track_dimensions.y - bar_dimensions.y;
		bar->SetOffset(
			Vector2f(bar->GetBox().GetEdge(BoxArea::Margin, BoxEdge::Left), track->GetRelativeOffset().y + traversable_track_length * bar_position),
			parent);
	}
	else
	{
		float traversable_track_length = track_dimensions.x - bar_dimensions.x;
		bar->SetOffset(
			Vector2f(track->GetRelativeOffset().x + traversable_track_length * bar_position, bar->GetBox().GetEdge(BoxArea::Margin, BoxEdge::Top)),
			parent);
	}
}

void WidgetScroll::SetTrackLength(float _track_length)
{
	track_length = _track_length;
}

void WidgetScroll::SetBarLength(float _bar_length)
{
	bar_length = _bar_length;
}

void WidgetScroll::ScrollLineDown()
{
	Scroll(SCROLL_LINE_LENGTH * ElementUtilities::GetDensityIndependentPixelRatio(parent), ScrollBehavior::Auto);
}

void WidgetScroll::ScrollLineUp()
{
	Scroll(-SCROLL_LINE_LENGTH * ElementUtilities::GetDensityIndependentPixelRatio(parent), ScrollBehavior::Auto);
}

void WidgetScroll::ScrollPageDown()
{
	Scroll(SCROLL_PAGE_FACTOR * bar_length, ScrollBehavior::Auto);
}

void WidgetScroll::ScrollPageUp()
{
	Scroll(-SCROLL_PAGE_FACTOR * bar_length, ScrollBehavior::Auto);
}

void WidgetScroll::Scroll(float distance, ScrollBehavior behavior)
{
	float traversable_track_length = (track_length - bar_length);

	float new_bar_position = bar_position;
	if (traversable_track_length > 0.f)
		new_bar_position = Math::Clamp((bar_position * traversable_track_length + distance) / traversable_track_length, 0.f, 1.f);

	// 'parent' is the scrollbar element, its parent again is the actual element we want to scroll
	Element* element_scroll = parent->GetParentNode();
	if (!element_scroll)
	{
		RMLUI_ERROR;
		return;
	}

	Vector2f scroll_offset = {element_scroll->GetScrollLeft(), element_scroll->GetScrollTop()};
	if (orientation == HORIZONTAL)
		scroll_offset.x = new_bar_position * (element_scroll->GetScrollWidth() - element_scroll->GetClientWidth());
	else
		scroll_offset.y = new_bar_position * (element_scroll->GetScrollHeight() - element_scroll->GetClientHeight());

	element_scroll->ScrollTo(scroll_offset, behavior);
}

} // namespace Rml
