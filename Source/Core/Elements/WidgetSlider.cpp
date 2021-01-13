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
#include "../../../Include/RmlUi/Core/Elements/ElementFormControl.h"
#include "../../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../../Include/RmlUi/Core/Factory.h"
#include "../../../Include/RmlUi/Core/Input.h"
#include "../../../Include/RmlUi/Core/Profiling.h"
#include "../Clock.h"

namespace Rml {

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

	value = 0;
	min_value = 0;
	max_value = 100;
	step = 1;
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
	Property drag_property = Property(Style::Drag::Drag);
	parent->SetProperty(PropertyId::Drag, drag_property);

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

	arrows[0]->SetProperty(PropertyId::Drag, drag_property);
	arrows[1]->SetProperty(PropertyId::Drag, drag_property);

	// Attach the listeners
	// All listeners are attached to parent, ensuring that we don't get duplicate events when it bubbles from child to parent
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
void WidgetSlider::GetDimensions(Vector2f& dimensions) const
{
	switch (orientation)
	{
		case VERTICAL:   dimensions.x = 16; dimensions.y = 256; break;
		case HORIZONTAL: dimensions.x = 256; dimensions.y = 16; break;
	}
}


void WidgetSlider::SetValue(float target_value)
{
	float num_steps = (target_value - min_value) / step;
	float new_value = min_value + Math::RoundFloat(num_steps) * step;

	if (new_value != value)
		SetBarPosition(SetValueInternal(new_value));
}

float WidgetSlider::GetValue() const
{
	return value;
}

// Sets the minimum value of the slider.
void WidgetSlider::SetMinValue(float _min_value)
{
	min_value = _min_value;
}

// Sets the maximum value of the slider.
void WidgetSlider::SetMaxValue(float _max_value)
{
	max_value = _max_value;
}

// Sets the slider's value increment.
void WidgetSlider::SetStep(float _step)
{
	// Can't have a zero step!
	if (_step == 0)
		return;

	step = _step;
}

// Formats the slider's elements.
void WidgetSlider::FormatElements()
{
	RMLUI_ZoneScopedNC("RangeOnResize", 0x228044);

	Vector2f box = GetParent()->GetBox().GetSize();
	WidgetSlider::FormatElements(box, GetOrientation() == VERTICAL ? box.y : box.x);
}


// Lays out and resizes the internal elements.
void WidgetSlider::FormatElements(const Vector2f containing_block, float slider_length)
{
	int length_axis = orientation == VERTICAL ? 1 : 0;

	// Build the box for the containing slider element. As the containing block is not guaranteed to have a defined
	// height, we must use the width for both axes.
	Box parent_box;
	ElementUtilities::BuildBox(parent_box, Vector2f(containing_block.x, containing_block.x), parent);

	// Set the length of the slider.
	Vector2f content = parent_box.GetSize();
	content[length_axis] = slider_length;
	parent_box.SetContent(content);

	// Generate the initial dimensions for the track. It'll need to be cut down to fit the arrows.
	Box track_box;
	ElementUtilities::BuildBox(track_box, parent_box.GetSize(), track);
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
		ElementUtilities::BuildBox(arrow_box, parent_box.GetSize(), arrows[i]);

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

	FormatBar();

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
void WidgetSlider::FormatBar()
{
	Box bar_box;
	ElementUtilities::BuildBox(bar_box, parent->GetBox().GetSize(), bar);
	auto& computed = bar->GetComputedValues();

	Vector2f bar_box_content = bar_box.GetSize();
	if (orientation == HORIZONTAL)
	{
		if (computed.height.type == Style::Height::Auto)
			bar_box_content.y = parent->GetBox().GetSize().y;
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
	if (parent->IsDisabled())
		return;

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
				bar_halfsize = 0.5f * bar->GetBox().GetSize(Box::BORDER).x;
			}
			else
			{
				mouse_position = event.GetParameter< float >("mouse_y", 0);
				bar_halfsize = 0.5f * bar->GetBox().GetSize(Box::BORDER).y;
			}

			float new_bar_position = AbsolutePositionToBarPosition(mouse_position - bar_halfsize);
			SetBarPosition(OnBarChange(new_bar_position));
		}
		else if (event.GetTargetElement() == arrows[0])
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
		const Input::KeyIdentifier key_identifier = (Input::KeyIdentifier) event.GetParameter< int >("key_identifier", 0);

		const bool increment = (key_identifier == Input::KI_RIGHT && orientation == HORIZONTAL) || (key_identifier == Input::KI_DOWN && orientation == VERTICAL);
		const bool decrement = (key_identifier == Input::KI_LEFT  && orientation == HORIZONTAL) || (key_identifier == Input::KI_UP   && orientation == VERTICAL);

		if (increment || decrement)
		{
			SetBarPosition(decrement ? OnLineDecrement() : OnLineIncrement());
			event.StopPropagation();
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

// Called when the slider's bar position is set or dragged.
float WidgetSlider::OnBarChange(float bar_position)
{
	float new_value = min_value + bar_position * (max_value - min_value);
	int num_steps = Math::RoundToInteger((new_value - value) / step);

	return SetValueInternal(value + num_steps * step);
}

// Called when the slider is incremented by one 'line'.
float WidgetSlider::OnLineIncrement()
{
	return SetValueInternal(value + step);
}

// Called when the slider is decremented by one 'line'.
float WidgetSlider::OnLineDecrement()
{
	return SetValueInternal(value - step);
}

float WidgetSlider::AbsolutePositionToBarPosition(float absolute_position) const
{
	float new_bar_position = bar_position;

	if (orientation == HORIZONTAL)
	{
		const float edge_left = bar->GetBox().GetEdge(Box::MARGIN, Box::LEFT);
		const float edge_right = bar->GetBox().GetEdge(Box::MARGIN, Box::RIGHT);

		float traversable_track_length = track->GetBox().GetSize(Box::CONTENT).x - bar->GetBox().GetSize(Box::BORDER).x - edge_left - edge_right;
		if (traversable_track_length > 0)
		{
			float traversable_track_origin = track->GetAbsoluteOffset().x + edge_left;
			new_bar_position = (absolute_position - traversable_track_origin) / traversable_track_length;
			new_bar_position = Math::Clamp(new_bar_position, 0.0f, 1.0f);
		}
	}
	else
	{
		const float edge_top = bar->GetBox().GetEdge(Box::MARGIN, Box::TOP);
		const float edge_bottom = bar->GetBox().GetEdge(Box::MARGIN, Box::BOTTOM);

		float traversable_track_length = track->GetBox().GetSize(Box::CONTENT).y - bar->GetBox().GetSize(Box::BORDER).y - edge_top - edge_bottom;
		if (traversable_track_length > 0)
		{
			float traversable_track_origin = track->GetAbsoluteOffset().y + edge_top;
			new_bar_position = (absolute_position - traversable_track_origin) / traversable_track_length;
			new_bar_position = Math::Clamp(new_bar_position, 0.0f, 1.0f);
		}
	}

	return new_bar_position;
}


void WidgetSlider::PositionBar()
{
	const Vector2f track_dimensions = track->GetBox().GetSize();
	const Vector2f bar_dimensions = bar->GetBox().GetSize(Box::BORDER);

	if (orientation == VERTICAL)
	{
		const float edge_top = bar->GetBox().GetEdge(Box::MARGIN, Box::TOP);
		const float edge_bottom = bar->GetBox().GetEdge(Box::MARGIN, Box::BOTTOM);

		float traversable_track_length = track_dimensions.y - bar_dimensions.y - edge_top - edge_bottom;
		bar->SetOffset(
			Vector2f(
				bar->GetBox().GetEdge(Box::MARGIN, Box::LEFT),
				track->GetRelativeOffset().y + edge_top + traversable_track_length * bar_position
			),
			parent
		);
	}
	else
	{
		const float edge_left = bar->GetBox().GetEdge(Box::MARGIN, Box::LEFT);
		const float edge_right = bar->GetBox().GetEdge(Box::MARGIN, Box::RIGHT);

		float traversable_track_length = track_dimensions.x - bar_dimensions.x - edge_left - edge_right;
		bar->SetOffset(
			Vector2f(
				track->GetRelativeOffset().x + edge_left + traversable_track_length * bar_position,
				bar->GetBox().GetEdge(Box::MARGIN, Box::TOP)
			), 
			parent
		);
	}
}

// Clamps the new value, sets it on the slider.
float WidgetSlider::SetValueInternal(float new_value)
{
	if (min_value < max_value)
	{
		value = Math::Clamp(new_value, min_value, max_value);
	}
	else if (min_value > max_value)
	{
		value = Math::Clamp(new_value, max_value, min_value);
	}
	else
	{
		value = min_value;
		return 0;
	}

	Dictionary parameters;
	parameters["value"] = value;
	GetParent()->DispatchEvent(EventId::Change, parameters);


	// TODO: This might not be the safest approach as this will call SetValue(), 
	// thus, a slight mismatch will result in infinite recursion.
	GetParent()->SetAttribute("value", value);

	return (value - min_value) / (max_value - min_value);
}

} // namespace Rml
