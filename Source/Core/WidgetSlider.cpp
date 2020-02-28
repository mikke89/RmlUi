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

#include "WidgetSlider.h"
#include "Clock.h"
#include "LayoutEngine.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Event.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/Property.h"

namespace Rml {
namespace Core {

static const float DEFAULT_REPEAT_DELAY = 0.5f;
static const float DEFAULT_REPEAT_PERIOD = 0.1f;

WidgetSlider::WidgetSlider(Element* _parent)
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
}

WidgetSlider::~WidgetSlider()
{
	if (bar != nullptr)
	{
		bar->RemoveEventListener(Core::EventId::Drag, this);
		bar->RemoveEventListener(Core::EventId::Dragstart, this);
	}

	if (track != nullptr)
		track->RemoveEventListener(Core::EventId::Click, this);

	for (int i = 0; i < 2; i++)
	{
		if (arrows[i] != nullptr)
		{
			arrows[i]->RemoveEventListener(Core::EventId::Mousedown, this);
			arrows[i]->RemoveEventListener(Core::EventId::Mouseup, this);
			arrows[i]->RemoveEventListener(Core::EventId::Mouseout, this);
		}
	}
}

// Initialises the slider to a given orientation.
bool WidgetSlider::Initialise(Orientation _orientation)
{
	// Check that we haven't already been successfully initialised.
	if (orientation != UNKNOWN)
	{
		RMLUI_ERROR;
		return false;
	}

	// Check that a valid orientation has been passed in.
	if (_orientation != HORIZONTAL &&
		_orientation != VERTICAL)
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

	bar->SetProperty(Core::PropertyId::Drag, Core::Property(Core::Style::Drag::Drag));

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

// Updates the key repeats for the increment / decrement arrows.
void WidgetSlider::Update()
{
	for (int i = 0; i < 2; i++)
	{
		bool updated_time = false;
		float delta_time = 0;

		if (arrow_timers[i] > 0)
		{
			if (!updated_time)
			{
				double current_time = Clock::GetElapsedTime();
				delta_time = float(current_time - last_update_time);
				last_update_time = current_time;
			}

			arrow_timers[i] -= delta_time;
			while (arrow_timers[i] <= 0)
			{
				arrow_timers[i] += DEFAULT_REPEAT_PERIOD;
				SetBarPosition(i == 0 ? OnLineDecrement() : OnLineIncrement());
			}
		}
	}
}

// Sets the position of the bar.
void WidgetSlider::SetBarPosition(float _bar_position)
{
	bar_position = Math::Clamp(_bar_position, 0.0f, 1.0f);
	PositionBar();
	
	// 'parent' is the scrollbar element, its parent again is the actual element we want to scroll
	Element* element_scroll = parent->GetParentNode();
	if (!element_scroll)
	{
		RMLUI_ERROR;
		return;
	}
	
	if (orientation == VERTICAL)
		element_scroll->SetScrollTop(bar_position * (element_scroll->GetScrollHeight() - element_scroll->GetClientHeight()));
	else if (orientation == HORIZONTAL)
		element_scroll->SetScrollLeft(bar_position * (element_scroll->GetScrollWidth() - element_scroll->GetClientWidth()));
}

// Returns the current position of the bar.
float WidgetSlider::GetBarPosition()
{
	return bar_position;
}

// Returns the slider's orientation.
WidgetSlider::Orientation WidgetSlider::GetOrientation() const
{
	return orientation;
}

// Sets the dimensions to the size of the slider.
void WidgetSlider::GetDimensions(Vector2f& dimensions) const
{
	switch (orientation)
	{
		RMLUI_UNUSED_SWITCH_ENUM(UNKNOWN);
		case VERTICAL:   dimensions.x = 16; dimensions.y = 256; break;
		case HORIZONTAL: dimensions.x = 256; dimensions.y = 16; break;
	}
}

// Lays out and resizes the internal elements.
void WidgetSlider::FormatElements(const Vector2f& containing_block, bool resize_element, float slider_length, float bar_length)
{
	int length_axis = orientation == VERTICAL ? 1 : 0;

	// Build the box for the containing slider element. As the containing block is not guaranteed to have a defined
	// height, we must use the width for both axes.
	Box parent_box;
	LayoutEngine::BuildBox(parent_box, Vector2f(containing_block.x, containing_block.x), parent);
	slider_length -= orientation == VERTICAL ? (parent_box.GetCumulativeEdge(Box::CONTENT, Box::TOP) + parent_box.GetCumulativeEdge(Box::CONTENT, Box::BOTTOM)) :
											   (parent_box.GetCumulativeEdge(Box::CONTENT, Box::LEFT) + parent_box.GetCumulativeEdge(Box::CONTENT, Box::RIGHT));

	// Set the length of the slider.
	Vector2f content = parent_box.GetSize();
	content[length_axis] = slider_length;
	parent_box.SetContent(content);
	// And set it on the slider element!
	if (resize_element)
		parent->SetBox(parent_box);

	// Generate the initial dimensions for the track. It'll need to be cut down to fit the arrows.
	Box track_box;
	LayoutEngine::BuildBox(track_box, parent_box.GetSize(), track);
	content = track_box.GetSize();
	content[length_axis] = slider_length -= orientation == VERTICAL ? (track_box.GetCumulativeEdge(Box::CONTENT, Box::TOP) + track_box.GetCumulativeEdge(Box::CONTENT, Box::BOTTOM)) :
																	  (track_box.GetCumulativeEdge(Box::CONTENT, Box::LEFT) + track_box.GetCumulativeEdge(Box::CONTENT, Box::RIGHT));
	// If no height has been explicitly specified for the track, it'll be initialised to -1 as per normal block
	// elements. We'll fix that up here.
	if (orientation == HORIZONTAL &&
		content.y < 0)
		content.y = parent_box.GetSize().y;

	// Now we size the arrows.
	for (int i = 0; i < 2; i++)
	{
		Box arrow_box;
		LayoutEngine::BuildBox(arrow_box, parent_box.GetSize(), arrows[i]);

		// Clamp the size to (0, 0).
		Vector2f arrow_size = arrow_box.GetSize();
		if (arrow_size.x < 0 ||
			arrow_size.y < 0)
			arrow_box.SetContent(Vector2f(0, 0));

		arrows[i]->SetBox(arrow_box);

		// Shrink the track length by the arrow size.
		content[length_axis] -= arrow_box.GetSize(Box::MARGIN)[length_axis];
	}

	// Now the track has been sized, we can fix everything into position.
	track_box.SetContent(content);
	track->SetBox(track_box);

	if (orientation == VERTICAL)
	{
		Vector2f offset(arrows[0]->GetBox().GetEdge(Box::MARGIN, Box::LEFT), arrows[0]->GetBox().GetEdge(Box::MARGIN, Box::TOP));
		arrows[0]->SetOffset(offset, parent);

		offset.x = track->GetBox().GetEdge(Box::MARGIN, Box::LEFT);
		offset.y += arrows[0]->GetBox().GetSize(Box::BORDER).y + arrows[0]->GetBox().GetEdge(Box::MARGIN, Box::BOTTOM) + track->GetBox().GetEdge(Box::MARGIN, Box::TOP);
		track->SetOffset(offset, parent);

		offset.x = arrows[1]->GetBox().GetEdge(Box::MARGIN, Box::LEFT);
		offset.y += track->GetBox().GetSize(Box::BORDER).y + track->GetBox().GetEdge(Box::MARGIN, Box::BOTTOM) + arrows[1]->GetBox().GetEdge(Box::MARGIN, Box::TOP);
		arrows[1]->SetOffset(offset, parent);
	}
	else
	{
		Vector2f offset(arrows[0]->GetBox().GetEdge(Box::MARGIN, Box::LEFT), arrows[0]->GetBox().GetEdge(Box::MARGIN, Box::TOP));
		arrows[0]->SetOffset(offset, parent);

		offset.x += arrows[0]->GetBox().GetSize(Box::BORDER).x + arrows[0]->GetBox().GetEdge(Box::MARGIN, Box::RIGHT) + track->GetBox().GetEdge(Box::MARGIN, Box::LEFT);
		offset.y = track->GetBox().GetEdge(Box::MARGIN, Box::TOP);
		track->SetOffset(offset, parent);

		offset.x += track->GetBox().GetSize(Box::BORDER).x + track->GetBox().GetEdge(Box::MARGIN, Box::RIGHT) + arrows[1]->GetBox().GetEdge(Box::MARGIN, Box::LEFT);
		offset.y = arrows[1]->GetBox().GetEdge(Box::MARGIN, Box::TOP);
		arrows[1]->SetOffset(offset, parent);
	}

	FormatBar(bar_length);
}

// Lays out and positions the bar element.
void WidgetSlider::FormatBar(float bar_length)
{
	Box bar_box;
	LayoutEngine::BuildBox(bar_box, parent->GetBox().GetSize(), bar);

	const auto& computed = bar->GetComputedValues();

	const Style::Width width = computed.width;
	const Style::Height height = computed.height;

	Vector2f bar_box_content = bar_box.GetSize();
	if (orientation == HORIZONTAL)
	{
		if (height.type == height.Auto)
			bar_box_content.y = parent->GetBox().GetSize().y;
	}

	if (bar_length >= 0)
	{
		Vector2f track_size = track->GetBox().GetSize();

		if (orientation == VERTICAL)
		{
			float track_length = track_size.y - (bar_box.GetCumulativeEdge(Box::CONTENT, Box::TOP) + bar_box.GetCumulativeEdge(Box::CONTENT, Box::BOTTOM));

			if (height.type == height.Auto)
			{
				bar_box_content.y = track_length * bar_length;

				// Check for 'min-height' restrictions.
				float min_track_length = ResolveValue(computed.min_height, track_length);
				bar_box_content.y = Math::Max(min_track_length, bar_box_content.y);

				// Check for 'max-height' restrictions.
				float max_track_length = ResolveValue(computed.max_height, track_length);
				if (max_track_length > 0)
					bar_box_content.y = Math::Min(max_track_length, bar_box_content.y);
			}

			// Make sure we haven't gone further than we're allowed to (min-height may have made us too big).
			bar_box_content.y = Math::Min(bar_box_content.y, track_length);
		}
		else
		{
			float track_length = track_size.x - (bar_box.GetCumulativeEdge(Box::CONTENT, Box::LEFT) + bar_box.GetCumulativeEdge(Box::CONTENT, Box::RIGHT));

			if (width.type == width.Auto)
			{
				bar_box_content.x = track_length * bar_length;

				// Check for 'min-width' restrictions.
				float min_track_length = ResolveValue(computed.min_width, track_length);
				bar_box_content.x = Math::Max(min_track_length, bar_box_content.x);

				// Check for 'max-width' restrictions.
				float max_track_length = ResolveValue(computed.max_width, track_length);
				if (max_track_length > 0)
					bar_box_content.x = Math::Min(max_track_length, bar_box_content.x);
			}

			// Make sure we haven't gone further than we're allowed to (min-width may have made us too big).
			bar_box_content.x = Math::Min(bar_box_content.x, track_length);
		}
	}

	// Set the new dimensions on the bar to re-decorate it.
	bar_box.SetContent(bar_box_content);
	bar->SetBox(bar_box);

	// Now that it's been resized, re-position it.
	PositionBar();
}

// Returns the widget's parent element.
Element* WidgetSlider::GetParent() const
{
	return parent;
}

// Handles events coming through from the slider's components.
void WidgetSlider::ProcessEvent(Event& event)
{
	if (event.GetTargetElement() == bar)
	{
		if (event == EventId::Drag)
		{
			if (orientation == HORIZONTAL)
			{
				float traversable_track_length = track->GetBox().GetSize(Box::CONTENT).x - bar->GetBox().GetSize(Box::CONTENT).x;
				if (traversable_track_length > 0)
				{
					float traversable_track_origin = track->GetAbsoluteOffset().x + bar_drag_anchor;
					float new_bar_position = (event.GetParameter< float >("mouse_x", 0) - traversable_track_origin) / traversable_track_length;
					new_bar_position = Math::Clamp(new_bar_position, 0.0f, 1.0f);

					SetBarPosition(OnBarChange(new_bar_position));
				}
			}
			else
			{
				float traversable_track_length = track->GetBox().GetSize(Box::CONTENT).y - bar->GetBox().GetSize(Box::CONTENT).y;
				if (traversable_track_length > 0)
				{
					float traversable_track_origin = track->GetAbsoluteOffset().y + bar_drag_anchor;
					float new_bar_position = (event.GetParameter< float >("mouse_y", 0) - traversable_track_origin) / traversable_track_length;
					new_bar_position = Math::Clamp(new_bar_position, 0.0f, 1.0f);

					SetBarPosition(OnBarChange(new_bar_position));
				}
			}
		}
		else if (event == EventId::Dragstart)
		{
			if (orientation == HORIZONTAL)
				bar_drag_anchor = event.GetParameter< int >("mouse_x", 0) - Math::RealToInteger(bar->GetAbsoluteOffset().x);
			else
				bar_drag_anchor = event.GetParameter< int >("mouse_y", 0) - Math::RealToInteger(bar->GetAbsoluteOffset().y);
		}
	}
	else if (event.GetTargetElement() == track)
	{
		if (event == EventId::Click)
		{
			if (orientation == HORIZONTAL)
			{
				float mouse_position = event.GetParameter< float >("mouse_x", 0);
				float click_position = (mouse_position - track->GetAbsoluteOffset().x) / track->GetBox().GetSize().x;

				SetBarPosition(click_position <= bar_position ? OnPageDecrement(click_position) : OnPageIncrement(click_position));
			}
			else
			{
				float mouse_position = event.GetParameter< float >("mouse_y", 0);
				float click_position = (mouse_position - track->GetAbsoluteOffset().y) / track->GetBox().GetSize().y;

				SetBarPosition(click_position <= bar_position ? OnPageDecrement(click_position) : OnPageIncrement(click_position));
			}
		}
	}

	if (event == EventId::Mousedown)
	{
		if (event.GetTargetElement() == arrows[0])
		{
			arrow_timers[0] = DEFAULT_REPEAT_DELAY;
			last_update_time = Clock::GetElapsedTime();
			SetBarPosition(OnLineDecrement());
		}
		else if (event.GetTargetElement() == arrows[1])
		{
			arrow_timers[1] = DEFAULT_REPEAT_DELAY;
			last_update_time = Clock::GetElapsedTime();
			SetBarPosition(OnLineIncrement());
		}
	}
	else if (event == EventId::Mouseup ||
			 event == EventId::Mouseout)
	{
		if (event.GetTargetElement() == arrows[0])
			arrow_timers[0] = -1;
		else if (event.GetTargetElement() == arrows[1])
			arrow_timers[1] = -1;
	}
}

void WidgetSlider::PositionBar()
{
	const Vector2f& track_dimensions = track->GetBox().GetSize();
	const Vector2f& bar_dimensions = bar->GetBox().GetSize(Box::BORDER);

	if (orientation == VERTICAL)
	{
		float traversable_track_length = track_dimensions.y - bar_dimensions.y;
		bar->SetOffset(Vector2f(bar->GetBox().GetEdge(Box::MARGIN, Box::LEFT), track->GetRelativeOffset().y + traversable_track_length * bar_position), parent);
	}
	else
	{
		float traversable_track_length = track_dimensions.x - bar_dimensions.x;
		bar->SetOffset(Vector2f(track->GetRelativeOffset().x + traversable_track_length * bar_position, bar->GetBox().GetEdge(Box::MARGIN, Box::TOP)), parent);
	}
}

}
}
