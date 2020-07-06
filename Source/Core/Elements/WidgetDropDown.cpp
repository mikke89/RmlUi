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

#include "WidgetDropDown.h"
#include "../../../Include/RmlUi/Core/Context.h"
#include "../../../Include/RmlUi/Core/ElementDocument.h"
#include "../../../Include/RmlUi/Core/Math.h"
#include "../../../Include/RmlUi/Core/Factory.h"
#include "../../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../../Include/RmlUi/Core/Event.h"
#include "../../../Include/RmlUi/Core/Input.h"
#include "../../../Include/RmlUi/Core/Property.h"
#include "../../../Include/RmlUi/Core/Profiling.h"
#include "../../../Include/RmlUi/Core/Elements/ElementFormControl.h"

namespace Rml {

WidgetDropDown::WidgetDropDown(ElementFormControl* element)
{
	parent_element = element;

	box_layout_dirty = false;
	value_layout_dirty = false;
	box_visible = false;

	selected_option = -1;

	// Create the button and selection elements.
	button_element = parent_element->AppendChild(Factory::InstanceElement(parent_element, "*", "selectarrow", XMLAttributes()), false);
	value_element = parent_element->AppendChild(Factory::InstanceElement(parent_element, "*", "selectvalue", XMLAttributes()), false);
	selection_element = parent_element->AppendChild(Factory::InstanceElement(parent_element, "*", "selectbox", XMLAttributes()), false);

	value_element->SetProperty(PropertyId::OverflowX, Property(Style::Overflow::Hidden));
	value_element->SetProperty(PropertyId::OverflowY, Property(Style::Overflow::Hidden));

	selection_element->SetProperty(PropertyId::Visibility, Property(Style::Visibility::Hidden));
	selection_element->SetProperty(PropertyId::ZIndex, Property(1.0f, Property::NUMBER));
	selection_element->SetProperty(PropertyId::Clip, Property(Style::Clip::Type::None));
	selection_element->SetProperty(PropertyId::OverflowY, Property(Style::Overflow::Auto));

	parent_element->AddEventListener(EventId::Click, this, true);
	parent_element->AddEventListener(EventId::Blur, this);
	parent_element->AddEventListener(EventId::Focus, this);
	parent_element->AddEventListener(EventId::Keydown, this, true);

	selection_element->AddEventListener(EventId::Mousescroll, this);
}

WidgetDropDown::~WidgetDropDown()
{
	// We shouldn't clear the options ourselves, as removing the element will automatically clear children.
	//   However, we do need to remove events of children.
	for(auto& option : options)
		option.GetElement()->RemoveEventListener(EventId::Click, this);

	parent_element->RemoveEventListener(EventId::Click, this, true);
	parent_element->RemoveEventListener(EventId::Blur, this);
	parent_element->RemoveEventListener(EventId::Focus, this);
	parent_element->RemoveEventListener(EventId::Keydown, this, true);
	
	selection_element->RemoveEventListener(EventId::Mousescroll, this);

	DetachScrollEvent();
}

// Updates the selection box layout if necessary.
void WidgetDropDown::OnRender()
{
	if (box_visible && box_layout_dirty)
	{
		// Layout the selection box. 
		// The following procedure should ensure that the selection box is never (partly) outside of the context's window.
		// This is achieved by positioning the box either above or below the 'select' element, and possibly shrinking
		// the element's height.
		// We try to respect user values of 'height', 'min-height', and 'max-height'. However, when we need to shrink the box
		// we will override the 'height' property.

		// Previously set 'height' property from this procedure must be removed for the calculations below to work as intended.
		if(selection_element->GetLocalStyleProperties().count(PropertyId::Height) == 1)
		{
			selection_element->RemoveProperty(PropertyId::Height);
			selection_element->GetOwnerDocument()->UpdateDocument();
		}

		Box box;
		ElementUtilities::BuildBox(box, parent_element->GetBox().GetSize(), selection_element);

		// The user can use 'margin-left/top/bottom' to offset the box away from the 'select' element, respectively
		// horizontally, vertically when box below, and vertically when box above.
		const float offset_x = box.GetEdge(Box::MARGIN, Box::LEFT);
		const float offset_y_below = parent_element->GetBox().GetSize(Box::BORDER).y + box.GetEdge(Box::MARGIN, Box::TOP);
		const float offset_y_above = -box.GetEdge(Box::MARGIN, Box::BOTTOM);

		float window_height = 100'000.f;
		if (Context* context = parent_element->GetContext())
			window_height = float(context->GetDimensions().y);

		const float absolute_y = parent_element->GetAbsoluteOffset(Box::BORDER).y;

		const float height_below = window_height - absolute_y - offset_y_below;
		const float height_above = absolute_y + offset_y_above;

		// Format the selection box and retrieve the 'native' height occupied by all the options, while respecting
		// the 'min/max-height' properties.
		ElementUtilities::FormatElement(selection_element, parent_element->GetBox().GetSize(Box::BORDER));
		const float content_height = selection_element->GetOffsetHeight();

		if (content_height < height_below)
		{
			// Position box below
			selection_element->SetOffset(Vector2f(offset_x, offset_y_below), parent_element);
		}
		else if (content_height < height_above)
		{
			// Position box above
			selection_element->SetOffset(Vector2f(offset_x, -content_height + offset_y_above), parent_element);
		}
		else 
		{
			// Shrink box and position either below or above
			const float padding_border_size =
				box.GetEdge(Box::BORDER, Box::TOP) + box.GetEdge(Box::BORDER, Box::BOTTOM) +
				box.GetEdge(Box::PADDING, Box::TOP) + box.GetEdge(Box::PADDING, Box::BOTTOM);

			float height = 0.f;
			float offset_y = 0.f;

			if (height_below > height_above)
			{
				// Position below
				height = height_below - padding_border_size;
				offset_y = offset_y_below;
			}
			else
			{
				// Position above
				height = height_above - padding_border_size;
				offset_y = offset_y_above - height_above;
			}

			// Set the height and re-format the selection box.
			selection_element->SetProperty(PropertyId::Height, Property(height, Property::PX));
			selection_element->GetOwnerDocument()->UpdateDocument();
			ElementUtilities::FormatElement(selection_element, parent_element->GetBox().GetSize(Box::BORDER));

			selection_element->SetOffset(Vector2f(offset_x, offset_y), parent_element);
		}

		box_layout_dirty = false;
	}

	if (value_layout_dirty)
	{
		ElementUtilities::FormatElement(value_element, parent_element->GetBox().GetSize(Box::BORDER));
		value_element->SetOffset(parent_element->GetBox().GetPosition(Box::CONTENT), parent_element);

		value_layout_dirty = false;
	}
}

void WidgetDropDown::OnLayout()
{
	RMLUI_ZoneScopedNC("DropDownLayout", 0x7FFF00);

	if(parent_element->IsDisabled())
	{
		// Propagate disabled state to selectvalue and selectarrow
		value_element->SetPseudoClass("disabled", true);
		button_element->SetPseudoClass("disabled", true);
	}

	// Layout the button and selection boxes.
	Box parent_box = parent_element->GetBox();

	ElementUtilities::PositionElement(button_element, Vector2f(0, 0), ElementUtilities::TOP_RIGHT);
	ElementUtilities::PositionElement(selection_element, Vector2f(0, 0), ElementUtilities::TOP_LEFT);

	// Calculate the value element position and size.
	Vector2f size;
	size.x = parent_element->GetBox().GetSize(Box::CONTENT).x - button_element->GetBox().GetSize(Box::MARGIN).x;
	size.y = parent_element->GetBox().GetSize(Box::CONTENT).y;

	value_element->SetOffset(parent_element->GetBox().GetPosition(Box::CONTENT), parent_element);
	value_element->SetBox(Box(size));

	box_layout_dirty = true;
	value_layout_dirty = true;
}

// Sets the value of the widget.
void WidgetDropDown::SetValue(const String& _value)
{
	for (size_t i = 0; i < options.size(); ++i)
	{
		if (options[i].GetValue() == _value)
		{
			SetSelection((int) i);
			return;
		}
	}

	if (selected_option >= 0 && selected_option < (int)options.size())
		options[selected_option].GetElement()->SetPseudoClass("checked", false);

	value = _value;
	value_element->SetInnerRML(value);
	value_layout_dirty = true;

	selected_option = -1;
}

// Returns the current value of the widget.
const String& WidgetDropDown::GetValue() const
{
	return value;
}

// Sets the index of the selection. If the new index lies outside of the bounds, it will be clamped.
void WidgetDropDown::SetSelection(int selection, bool force)
{
	String new_value;

	if (selection < 0 ||
		selection >= (int) options.size())
	{
		selection = -1;
	}
	else
	{
		new_value = options[selection].GetValue();
	}

	if (force ||
		selection != selected_option ||
		value != new_value)
	{
		if (selected_option >= 0 && selected_option < (int)options.size())
			options[selected_option].GetElement()->SetPseudoClass("checked", false);
		
		selected_option = selection;
		value = new_value;

		String value_rml;
		if (selected_option >= 0) 
		{
			auto* el = options[selected_option].GetElement();
			el->GetInnerRML(value_rml);
			el->SetPseudoClass("checked", true);
		}


		value_element->SetInnerRML(value_rml);
		value_layout_dirty = true;

		Dictionary parameters;
		parameters["value"] = value;
		parent_element->DispatchEvent(EventId::Change, parameters);
	}
}

// Returns the index of the currently selected item.
int WidgetDropDown::GetSelection() const
{
	return selected_option;
}

// Adds a new option to the select control.
int WidgetDropDown::AddOption(const String& rml, const String& new_value, int before, bool select, bool selectable)
{
	ElementPtr element = Factory::InstanceElement(selection_element, "*", "option", XMLAttributes());
	element->SetInnerRML(rml);

	int result = AddOption(std::move(element), new_value, before, select, selectable);

	return result;
}

int WidgetDropDown::AddOption(ElementPtr element, const String& new_value, int before, bool select, bool selectable)
{
	static const String str_option = "option";

	if (element->GetTagName() != str_option)
	{
		Log::Message(Log::LT_WARNING, "A child of '%s' must be of type 'option' but '%s' was given. See element '%s'.", parent_element->GetTagName().c_str(), element->GetTagName().c_str(), parent_element->GetAddress().c_str());
		return -1;
	}

	// Force to block display. Register a click handler so we can be notified of selection.
	element->SetProperty(PropertyId::Display, Property(Style::Display::Block));
	element->SetProperty(PropertyId::Clip, Property(Style::Clip::Type::Auto));
	element->AddEventListener(EventId::Click, this);

	int option_index;
	if (before < 0 || before >= (int)options.size())
	{
		Element* ptr = selection_element->AppendChild(std::move(element));
		options.push_back(SelectOption(ptr, new_value, selectable));
		option_index = (int)options.size() - 1;
	}
	else
	{
		Element* ptr = selection_element->InsertBefore(std::move(element), selection_element->GetChild(before));
		options.insert(options.begin() + before, SelectOption(ptr, new_value, selectable));
		option_index = before;
	}

	// Select the option if appropriate.
	if (select)
		SetSelection(option_index);

	box_layout_dirty = true;
	return option_index;
}

// Removes an option from the select control.
void WidgetDropDown::RemoveOption(int index)
{
	if (index < 0 ||
		index >= (int) options.size())
		return;

	// Remove the listener and delete the option element.
	options[index].GetElement()->RemoveEventListener(EventId::Click, this);
	selection_element->RemoveChild(options[index].GetElement());
	options.erase(options.begin() + index);

	box_layout_dirty = true;
}

// Removes all options from the list.
void WidgetDropDown::ClearOptions()
{
	while (!options.empty())
		RemoveOption((int) options.size() - 1);
}

// Returns on of the widget's options.
SelectOption* WidgetDropDown::GetOption(int index)
{
	if (index < 0 ||
		index >= GetNumOptions())
		return nullptr;

	return &options[index];
}

// Returns the number of options in the widget.
int WidgetDropDown::GetNumOptions() const
{
	return (int) options.size();
}

void WidgetDropDown::AttachScrollEvent()
{
	if (ElementDocument* document = parent_element->GetOwnerDocument())
		document->AddEventListener(EventId::Scroll, this, true);
}

void WidgetDropDown::DetachScrollEvent()
{
	if (ElementDocument* document = parent_element->GetOwnerDocument())
		document->RemoveEventListener(EventId::Scroll, this, true);
}

void WidgetDropDown::ProcessEvent(Event& event)
{
	if (parent_element->IsDisabled()) 
		return;

	// Process the button onclick
	switch (event.GetId())
	{
	case EventId::Click:
	{

		if (event.GetCurrentElement()->GetParentNode() == selection_element)
		{
			// Find the element in the options and fire the selection event
			for (size_t i = 0; i < options.size(); i++)
			{
				if (options[i].GetElement() == event.GetCurrentElement())
				{
					if (options[i].IsSelectable())
					{
						SetSelection((int)i);
						event.StopPropagation();

						ShowSelectBox(false);
						parent_element->Focus();
					}
				}
			}
		}
		else
		{
			// We have to check that this event isn't targeted to an element
			// inside the selection box as we'll get all events coming from our
			// root level select element as well as the ones coming from options (which
			// get caught in the above if)
			Element* element = event.GetTargetElement();
			while (element && element != parent_element)
			{
				if (element == selection_element)
					return;
				element = element->GetParentNode();
			}

			if (selection_element->GetComputedValues().visibility == Style::Visibility::Hidden)
				ShowSelectBox(true);
			else
				ShowSelectBox(false);
		}
	}
	break;
	case EventId::Focus:
	{
		if (event.GetTargetElement() == parent_element)
		{
			value_element->SetPseudoClass("focus", true);
			button_element->SetPseudoClass("focus", true);
		}
	}
	break;
	case EventId::Blur:
	{
		if (event.GetTargetElement() == parent_element)
		{
			ShowSelectBox(false);
			value_element->SetPseudoClass("focus", false);
			button_element->SetPseudoClass("focus", false);
		}
	}
	break;
	case EventId::Keydown:
	{
		Input::KeyIdentifier key_identifier = (Input::KeyIdentifier) event.GetParameter< int >("key_identifier", 0);

		switch (key_identifier)
		{
		case Input::KI_UP:
			SetSelection((selected_option - 1 + (int)options.size()) % (int)options.size());
			break;
		case Input::KI_DOWN:
			SetSelection((selected_option + 1) % (int)options.size());
			break;
		default:
			break;
		}
	}
	break;
	case EventId::Mousescroll:
	{
		if (event.GetCurrentElement() == selection_element)
		{
			// Prevent scrolling in the parent window when mouse is inside the selection box.
			event.StopPropagation();
			// Stopping propagation also stops all default scrolling actions. However, we still want to be able
			// to scroll in the selection box, so call the default action manually.
			selection_element->ProcessDefaultAction(event);
		}
	}
	break;
	case EventId::Scroll:
	{
		if (box_visible)
		{
			// Close the select box if we scroll outside the select box.
			bool scrolls_selection_box = false;

			for (Element* element = event.GetTargetElement(); element; element = element->GetParentNode())
			{
				if (element == selection_element)
				{
					scrolls_selection_box = true;
					break;
				}
			}

			if (!scrolls_selection_box)
				ShowSelectBox(false);
		}
	}
	break;
	default:
		break;
	}
}

// Shows or hides the selection box.
void WidgetDropDown::ShowSelectBox(bool show)
{
	if (show)
	{
		selection_element->SetProperty(PropertyId::Visibility, Property(Style::Visibility::Visible));
		value_element->SetPseudoClass("checked", true);
		button_element->SetPseudoClass("checked", true);
		box_layout_dirty = true;
		AttachScrollEvent();
	}
	else
	{
		selection_element->SetProperty(PropertyId::Visibility, Property(Style::Visibility::Hidden));
		selection_element->RemoveProperty(PropertyId::Height);
		value_element->SetPseudoClass("checked", false);
		button_element->SetPseudoClass("checked", false);
		DetachScrollEvent();
	}
	box_visible = show;
}

} // namespace Rml
