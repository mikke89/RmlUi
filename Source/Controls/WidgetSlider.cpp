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
#include "../../Include/RmlUi/Controls/ElementFormControl.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/Input.h"
#include "../Core/Clock.h"

namespace Rml {
namespace Controls {

static const float DEFAULT_REPEAT_DELAY = 0.5f;
static const float DEFAULT_REPEAT_PERIOD = 0.1f;

WidgetSlider::WidgetSlider(ElementFormControl* _parent)
{
	parent = _parent;

	orientation = HORIZONTAL;

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
		parent->RemoveChild(bar);
	}

	if (track != nullptr)
	{
		parent->RemoveChild(track);
	}

	using Core::EventId;
	parent->RemoveEventListener(EventId::Blur, this);
	parent->RemoveEventListener(EventId::Focus, this);
	parent->RemoveEventListener(EventId::Keydown, this, true);
	parent->RemoveEventListener(EventId::Mousedown, this);
	parent->RemoveEventListener(EventId::Mouseup, this);
	parent->RemoveEventListener(EventId::Mouseout, this);
	parent->RemoveEventListener(EventId::Drag, this);
	parent->RemoveEventListener(EventId::Dragstart, this);
	parent->RemoveEventListener(EventId::Dragend, this);

	for (int i = 0; i < 2; i++)
	{
		if (arrows[i] != nullptr)
		{
			parent->RemoveChild(arrows[i]);
		}
	}
}

// Initialises the slider to a given orientation.
bool WidgetSlider::Initialise()
{
	Core::Property drag_property = Core::Property(Core::Style::Drag::Drag);
	parent->SetProperty(Core::PropertyId::Drag, drag_property);

	// Create all of our child elements as standard elements, and abort if we can't create them.
	Core::ElementPtr track_element = Core::Factory::InstanceElement(parent, "*", "slidertrack", Core::XMLAttributes());
	Core::ElementPtr bar_element = Core::Factory::InstanceElement(parent, "*", "sliderbar", Core::XMLAttributes());
	Core::ElementPtr arrow0_element = Core::Factory::InstanceElement(parent, "*", "sliderarrowdec", Core::XMLAttributes());
	Core::ElementPtr arrow1_element = Core::Factory::InstanceElement(parent, "*", "sliderarrowinc", Core::XMLAttributes());

	if (!track_element || !bar_element || !arrow0_element || !arrow1_element)
	{
		return false;
	}

	// Add them as non-DOM elements.
	track = parent->AppendChild(std::move(track_element), false);
	bar = parent->AppendChild(std::move(bar_element), false);
	arrows[0] = parent->AppendChild(std::move(arrow0_element), false);
	arrows[1] = parent->AppendChild(std::move(arrow1_element), false);

	arrows[0]->SetProperty(Core::PropertyId::Drag, drag_property);
	arrows[1]->SetProperty(Core::PropertyId::Drag, drag_property);

	// Attach the listeners
	// All listeners are attached to parent, ensuring that we don't get duplicate events when it bubbles from child to parent
	using Core::EventId;
	parent->AddEventListener(EventId::Blur, this);
	parent->AddEventListener(EventId::Focus, this);
	parent->AddEventListener(EventId::Keydown, this, true);
	parent->AddEventListener(EventId::Mousedown, this);
	parent->AddEventListener(EventId::Mouseup, this);
	parent->AddEventListener(EventId::Mouseout, this);
	parent->AddEventListener(EventId::Drag, this);
	parent->AddEventListener(EventId::Dragstart, this);
	parent->AddEventListener(EventId::Dragend, this);

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
				double current_time = Core::Clock::GetElapsedTime();
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
	bar_position = Rml::Core::Math::Clamp(_bar_position, 0.0f, 1.0f);
	PositionBar();
}

// Returns the current position of the bar.
float WidgetSlider::GetBarPosition()
{
	return bar_position;
}

// Sets the orientation of the slider.
void WidgetSlider::SetOrientation(Orientation _orientation)
{
	orientation = _orientation;
}

// Returns the slider's orientation.
WidgetSlider::Orientation WidgetSlider::GetOrientation() const
{
	return orientation;
}

// Sets the dimensions to the size of the slider.
void WidgetSlider::GetDimensions(Rml::Core::Vector2f& dimensions) const
{
	switch (orientation)
	{
		case VERTICAL:   dimensions.x = 16; dimensions.y = 256; break;
		case HORIZONTAL: dimensions.x = 256; dimensions.y = 16; break;
	}
}

// Lays out and resizes the internal elements.
void WidgetSlider::FormatElements(const Rml::Core::Vector2f& containing_block, float slider_length, float bar_length)
{
	int length_axis = orientation == VERTICAL ? 1 : 0;

	// Build the box for the containing slider element. As the containing block is not guaranteed to have a defined
	// height, we must use the width for both axes.
	Core::Box parent_box;
	Core::ElementUtilities::BuildBox(parent_box, Rml::Core::Vector2f(containing_block.x, containing_block.x), parent);

	// Set the length of the slider.
	Rml::Core::Vector2f content = parent_box.GetSize();
	content[length_axis] = slider_length;
	parent_box.SetContent(content);

	// Generate the initial dimensions for the track. It'll need to be cut down to fit the arrows.
	Core::Box track_box;
	Core::ElementUtilities::BuildBox(track_box, parent_box.GetSize(), track);
	content = track_box.GetSize();
	content[length_axis] = slider_length -= orientation == VERTICAL ? (track_box.GetCumulativeEdge(Core::Box::CONTENT, Core::Box::TOP) + track_box.GetCumulativeEdge(Core::Box::CONTENT, Core::Box::BOTTOM)) :
																	  (track_box.GetCumulativeEdge(Core::Box::CONTENT, Core::Box::LEFT) + track_box.GetCumulativeEdge(Core::Box::CONTENT, Core::Box::RIGHT));
	// If no height has been explicitly specified for the track, it'll be initialised to -1 as per normal block
	// elements. We'll fix that up here.
	if (orientation == HORIZONTAL &&
		content.y < 0)
		content.y = parent_box.GetSize().y;

	// Now we size the arrows.
	for (int i = 0; i < 2; i++)
	{
		Core::Box arrow_box;
		Core::ElementUtilities::BuildBox(arrow_box, parent_box.GetSize(), arrows[i]);

		// Clamp the size to (0, 0).
		Rml::Core::Vector2f arrow_size = arrow_box.GetSize();
		if (arrow_size.x < 0 ||
			arrow_size.y < 0)
			arrow_box.SetContent(Rml::Core::Vector2f(0, 0));

		arrows[i]->SetBox(arrow_box);

		// Shrink the track length by the arrow size.
		content[length_axis] -= arrow_box.GetSize(Core::Box::MARGIN)[length_axis];
	}

	// Now the track has been sized, we can fix everything into position.
	track_box.SetContent(content);
	track->SetBox(track_box);

	if (orientation == VERTICAL)
	{
		Rml::Core::Vector2f offset(arrows[0]->GetBox().GetEdge(Core::Box::MARGIN, Core::Box::LEFT), arrows[0]->GetBox().GetEdge(Core::Box::MARGIN, Core::Box::TOP));
		arrows[0]->SetOffset(offset, parent);

		offset.x = track->GetBox().GetEdge(Core::Box::MARGIN, Core::Box::LEFT);
		offset.y += arrows[0]->GetBox().GetSize(Core::Box::BORDER).y + arrows[0]->GetBox().GetEdge(Core::Box::MARGIN, Core::Box::BOTTOM) + track->GetBox().GetEdge(Core::Box::MARGIN, Core::Box::TOP);
		track->SetOffset(offset, parent);

		offset.x = arrows[1]->GetBox().GetEdge(Core::Box::MARGIN, Core::Box::LEFT);
		offset.y += track->GetBox().GetSize(Core::Box::BORDER).y + track->GetBox().GetEdge(Core::Box::MARGIN, Core::Box::BOTTOM) + arrows[1]->GetBox().GetEdge(Core::Box::MARGIN, Core::Box::TOP);
		arrows[1]->SetOffset(offset, parent);
	}
	else
	{
		Rml::Core::Vector2f offset(arrows[0]->GetBox().GetEdge(Core::Box::MARGIN, Core::Box::LEFT), arrows[0]->GetBox().GetEdge(Core::Box::MARGIN, Core::Box::TOP));
		arrows[0]->SetOffset(offset, parent);

		offset.x += arrows[0]->GetBox().GetSize(Core::Box::BORDER).x + arrows[0]->GetBox().GetEdge(Core::Box::MARGIN, Core::Box::RIGHT) + track->GetBox().GetEdge(Core::Box::MARGIN, Core::Box::LEFT);
		offset.y = track->GetBox().GetEdge(Core::Box::MARGIN, Core::Box::TOP);
		track->SetOffset(offset, parent);

		offset.x += track->GetBox().GetSize(Core::Box::BORDER).x + track->GetBox().GetEdge(Core::Box::MARGIN, Core::Box::RIGHT) + arrows[1]->GetBox().GetEdge(Core::Box::MARGIN, Core::Box::LEFT);
		offset.y = arrows[1]->GetBox().GetEdge(Core::Box::MARGIN, Core::Box::TOP);
		arrows[1]->SetOffset(offset, parent);
	}

	FormatBar(bar_length);

	if (parent->IsDisabled())
	{
	    // Propagate disabled state to child elements
	    bar->SetPseudoClass("disabled", true);
	    track->SetPseudoClass("disabled", true);
	    arrows[0]->SetPseudoClass("disabled", true);
	    arrows[1]->SetPseudoClass("disabled", true);
	}
}

// Lays out and positions the bar element.
void WidgetSlider::FormatBar(float bar_length)
{
	Core::Box bar_box;
	Core::ElementUtilities::BuildBox(bar_box, parent->GetBox().GetSize(), bar);
	auto& computed = bar->GetComputedValues();

	Rml::Core::Vector2f bar_box_content = bar_box.GetSize();
	if (orientation == HORIZONTAL)
	{
		if (computed.height.value == Core::Style::Height::Auto)
			bar_box_content.y = parent->GetBox().GetSize().y;
	}

	if (bar_length >= 0)
	{
		Rml::Core::Vector2f track_size = track->GetBox().GetSize();

		if (orientation == VERTICAL)
		{
			float track_length = track_size.y - (bar_box.GetCumulativeEdge(Core::Box::CONTENT, Core::Box::TOP) + bar_box.GetCumulativeEdge(Core::Box::CONTENT, Core::Box::BOTTOM));

			if (computed.height.value == Core::Style::Height::Auto)
			{
				bar_box_content.y = track_length * bar_length;

				// Check for 'min-height' restrictions.
				float min_track_length = Core::ResolveValue(computed.min_height, track_length);
				bar_box_content.y = Rml::Core::Math::Max(min_track_length, bar_box_content.y);

				// Check for 'max-height' restrictions.
				float max_track_length = Core::ResolveValue(computed.max_height, track_length);
				if (max_track_length > 0)
					bar_box_content.y = Rml::Core::Math::Min(max_track_length, bar_box_content.y);
			}

			// Make sure we haven't gone further than we're allowed to (min-height may have made us too big).
			bar_box_content.y = Rml::Core::Math::Min(bar_box_content.y, track_length);
		}
		else
		{
			float track_length = track_size.x - (bar_box.GetCumulativeEdge(Core::Box::CONTENT, Core::Box::LEFT) + bar_box.GetCumulativeEdge(Core::Box::CONTENT, Core::Box::RIGHT));

			if (computed.width.value == Core::Style::Width::Auto)
			{
				bar_box_content.x = track_length * bar_length;

				// Check for 'min-width' restrictions.
				float min_track_length = Core::ResolveValue(computed.min_width, track_length);
				bar_box_content.x = Rml::Core::Math::Max(min_track_length, bar_box_content.x);

				// Check for 'max-width' restrictions.
				float max_track_length = Core::ResolveValue(computed.max_width, track_length);
				if (max_track_length > 0)
					bar_box_content.x = Rml::Core::Math::Min(max_track_length, bar_box_content.x);
			}

			// Make sure we haven't gone further than we're allowed to (min-width may have made us too big).
			bar_box_content.x = Rml::Core::Math::Min(bar_box_content.x, track_length);
		}
	}

	// Set the new dimensions on the bar to re-decorate it.
	bar_box.SetContent(bar_box_content);
	bar->SetBox(bar_box);

	// Now that it's been resized, re-position it.
	PositionBar();
}

// Returns the widget's parent element.
Core::Element* WidgetSlider::GetParent() const
{
	return parent;
}

// Handles events coming through from the slider's components.
void WidgetSlider::ProcessEvent(Core::Event& event)
{
	if (parent->IsDisabled())
		return;

	using Rml::Core::EventId;

	switch (event.GetId())
	{
	case EventId::Mousedown:
	{
		if (event.GetTargetElement() == parent || event.GetTargetElement() == track)
		{
			float mouse_position, bar_halfsize;

			if (orientation == HORIZONTAL)
			{
				mouse_position = event.GetParameter< float >("mouse_x", 0);
				bar_halfsize = 0.5f * bar->GetBox().GetSize(Core::Box::BORDER).x;
			}
			else
			{
				mouse_position = event.GetParameter< float >("mouse_y", 0);
				bar_halfsize = 0.5f * bar->GetBox().GetSize(Core::Box::BORDER).y;
			}

			float new_bar_position = AbsolutePositionToBarPosition(mouse_position - bar_halfsize);
			SetBarPosition(OnBarChange(new_bar_position));
		}
		else if (event.GetTargetElement() == arrows[0])
		{
			arrow_timers[0] = DEFAULT_REPEAT_DELAY;
			last_update_time = Core::Clock::GetElapsedTime();
			SetBarPosition(OnLineDecrement());
		}
		else if (event.GetTargetElement() == arrows[1])
		{
			arrow_timers[1] = DEFAULT_REPEAT_DELAY;
			last_update_time = Core::Clock::GetElapsedTime();
			SetBarPosition(OnLineIncrement());
		}
	}
	break;

	case EventId::Mouseup:
	case EventId::Mouseout:
	{
		if (event.GetTargetElement() == arrows[0])
			arrow_timers[0] = -1;
		else if (event.GetTargetElement() == arrows[1])
			arrow_timers[1] = -1;
	}
	break;

	case EventId::Dragstart:
	{
		if (event.GetTargetElement() == parent)
		{
			bar->SetPseudoClass("active", true);

			if (orientation == HORIZONTAL)
				bar_drag_anchor = event.GetParameter< float >("mouse_x", 0) - bar->GetAbsoluteOffset().x;
			else
				bar_drag_anchor = event.GetParameter< float >("mouse_y", 0) - bar->GetAbsoluteOffset().y;
		}
	}
	break;
	case EventId::Drag:
	{
		if (event.GetTargetElement() == parent)
		{
			float new_bar_offset = event.GetParameter< float >((orientation == HORIZONTAL ? "mouse_x" : "mouse_y"), 0) - bar_drag_anchor;
			float new_bar_position = AbsolutePositionToBarPosition(new_bar_offset);

			SetBarPosition(OnBarChange(new_bar_position));
		}
	}
	break;
	case EventId::Dragend:
	{
		if (event.GetTargetElement() == parent)
		{
			bar->SetPseudoClass("active", false);
		}
	}
	break;


	case EventId::Keydown:
	{
		Core::Input::KeyIdentifier key_identifier = (Core::Input::KeyIdentifier) event.GetParameter< int >("key_identifier", 0);

		switch (key_identifier)
		{
		case Core::Input::KI_LEFT:
			if (orientation == HORIZONTAL) SetBarPosition(OnLineDecrement());
			break;
		case Core::Input::KI_UP:
			if (orientation == VERTICAL) SetBarPosition(OnLineDecrement());
			break;
		case Core::Input::KI_RIGHT:
			if (orientation == HORIZONTAL) SetBarPosition(OnLineIncrement());
			break;
		case Core::Input::KI_DOWN:
			if (orientation == VERTICAL) SetBarPosition(OnLineIncrement());
			break;
		default:
			break;
		}
	}
	break;

	case EventId::Focus:
	{
		if (event.GetTargetElement() == parent)
			bar->SetPseudoClass("focus", true);
	}
	break;
	case EventId::Blur:
	{
		if (event.GetTargetElement() == parent)
			bar->SetPseudoClass("focus", false);
	}
	break;

	default:
		break;
	}
}


float WidgetSlider::AbsolutePositionToBarPosition(float absolute_position) const
{
	float new_bar_position = bar_position;

	if (orientation == HORIZONTAL)
	{
		const float edge_left = bar->GetBox().GetEdge(Core::Box::MARGIN, Core::Box::LEFT);
		const float edge_right = bar->GetBox().GetEdge(Core::Box::MARGIN, Core::Box::RIGHT);

		float traversable_track_length = track->GetBox().GetSize(Core::Box::CONTENT).x - bar->GetBox().GetSize(Core::Box::BORDER).x - edge_left - edge_right;
		if (traversable_track_length > 0)
		{
			float traversable_track_origin = track->GetAbsoluteOffset().x + edge_left;
			new_bar_position = (absolute_position - traversable_track_origin) / traversable_track_length;
			new_bar_position = Rml::Core::Math::Clamp(new_bar_position, 0.0f, 1.0f);
		}
	}
	else
	{
		const float edge_top = bar->GetBox().GetEdge(Core::Box::MARGIN, Core::Box::TOP);
		const float edge_bottom = bar->GetBox().GetEdge(Core::Box::MARGIN, Core::Box::BOTTOM);

		float traversable_track_length = track->GetBox().GetSize(Core::Box::CONTENT).y - bar->GetBox().GetSize(Core::Box::BORDER).y - edge_top - edge_bottom;
		if (traversable_track_length > 0)
		{
			float traversable_track_origin = track->GetAbsoluteOffset().y + edge_top;
			new_bar_position = (absolute_position - traversable_track_origin) / traversable_track_length;
			new_bar_position = Rml::Core::Math::Clamp(new_bar_position, 0.0f, 1.0f);
		}
	}

	return new_bar_position;
}


void WidgetSlider::PositionBar()
{
	const Rml::Core::Vector2f track_dimensions = track->GetBox().GetSize();
	const Rml::Core::Vector2f bar_dimensions = bar->GetBox().GetSize(Core::Box::BORDER);

	if (orientation == VERTICAL)
	{
		const float edge_top = bar->GetBox().GetEdge(Core::Box::MARGIN, Core::Box::TOP);
		const float edge_bottom = bar->GetBox().GetEdge(Core::Box::MARGIN, Core::Box::BOTTOM);

		float traversable_track_length = track_dimensions.y - bar_dimensions.y - edge_top - edge_bottom;
		bar->SetOffset(
			Core::Vector2f(
				bar->GetBox().GetEdge(Core::Box::MARGIN, Core::Box::LEFT),
				track->GetRelativeOffset().y + edge_top + traversable_track_length * bar_position
			),
			parent
		);
	}
	else
	{
		const float edge_left = bar->GetBox().GetEdge(Core::Box::MARGIN, Core::Box::LEFT);
		const float edge_right = bar->GetBox().GetEdge(Core::Box::MARGIN, Core::Box::RIGHT);

		float traversable_track_length = track_dimensions.x - bar_dimensions.x - edge_left - edge_right;
		bar->SetOffset(
			Core::Vector2f(
				track->GetRelativeOffset().x + edge_left + traversable_track_length * bar_position,
				bar->GetBox().GetEdge(Core::Box::MARGIN, Core::Box::TOP)
			), 
			parent
		);
	}
}

}
}
