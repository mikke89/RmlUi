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
#include "../../Include/RmlUi/Core/Math.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../Include/RmlUi/Core/Event.h"
#include "../../Include/RmlUi/Core/Input.h"
#include "../../Include/RmlUi/Core/Property.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Controls/ElementFormControl.h"

namespace Rml {
namespace Controls {

WidgetDropDown::WidgetDropDown(ElementFormControl* element)
{
	parent_element = element;

	box_layout_dirty = false;
	value_layout_dirty = false;
	box_visible = false;

	selected_option = -1;

	// Create the button and selection elements.
	button_element = parent_element->AppendChild(Core::Factory::InstanceElement(parent_element, "*", "selectarrow", Rml::Core::XMLAttributes()), false);
	value_element = parent_element->AppendChild(Core::Factory::InstanceElement(parent_element, "*", "selectvalue", Rml::Core::XMLAttributes()), false);
	selection_element = parent_element->AppendChild(Core::Factory::InstanceElement(parent_element, "*", "selectbox", Rml::Core::XMLAttributes()), false);

	value_element->SetProperty(Core::PropertyId::OverflowX, Core::Property(Core::Style::Overflow::Hidden));
	value_element->SetProperty(Core::PropertyId::OverflowY, Core::Property(Core::Style::Overflow::Hidden));

	selection_element->SetProperty(Core::PropertyId::Visibility, Core::Property(Core::Style::Visibility::Hidden));
	selection_element->SetProperty(Core::PropertyId::ZIndex, Core::Property(1.0f, Core::Property::NUMBER));
	selection_element->SetProperty(Core::PropertyId::Clip, Core::Property(Core::Style::Clip::Type::None));

	parent_element->AddEventListener(Core::EventId::Click, this, true);
	parent_element->AddEventListener(Core::EventId::Blur, this);
	parent_element->AddEventListener(Core::EventId::Focus, this);
	parent_element->AddEventListener(Core::EventId::Keydown, this, true);
}

WidgetDropDown::~WidgetDropDown()
{
	// We shouldn't clear the options ourselves, as removing the element will automatically clear children.
	//   However, we do need to remove events of children.
	for(auto& option : options)
		option.GetElement()->RemoveEventListener(Core::EventId::Click, this);

	parent_element->RemoveEventListener(Core::EventId::Click, this, true);
	parent_element->RemoveEventListener(Core::EventId::Blur, this);
	parent_element->RemoveEventListener(Core::EventId::Focus, this);
	parent_element->RemoveEventListener(Core::EventId::Keydown, this, true);
}

// Updates the selection box layout if necessary.
void WidgetDropDown::OnRender()
{
	if (box_visible && box_layout_dirty)
	{
		Core::Box box;
		Core::ElementUtilities::BuildBox(box, parent_element->GetBox().GetSize(), selection_element);

		// Layout the selection box.
		Core::ElementUtilities::FormatElement(selection_element, parent_element->GetBox().GetSize(Core::Box::BORDER));
		selection_element->SetOffset(Rml::Core::Vector2f(box.GetEdge(Core::Box::MARGIN, Core::Box::LEFT), parent_element->GetBox().GetSize(Core::Box::BORDER).y + box.GetEdge(Core::Box::MARGIN, Core::Box::TOP)), parent_element);

		box_layout_dirty = false;
	}

	if (value_layout_dirty)
	{
		Core::ElementUtilities::FormatElement(value_element, parent_element->GetBox().GetSize(Core::Box::BORDER));
		value_element->SetOffset(parent_element->GetBox().GetPosition(Core::Box::CONTENT), parent_element);

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
	Core::Box parent_box = parent_element->GetBox();

	Core::ElementUtilities::PositionElement(button_element, Rml::Core::Vector2f(0, 0), Core::ElementUtilities::TOP_RIGHT);
	Core::ElementUtilities::PositionElement(selection_element, Rml::Core::Vector2f(0, 0), Core::ElementUtilities::TOP_LEFT);

	// Calculate the value element position and size.
	Rml::Core::Vector2f size;
	size.x = parent_element->GetBox().GetSize(Core::Box::CONTENT).x - button_element->GetBox().GetSize(Core::Box::MARGIN).x;
	size.y = parent_element->GetBox().GetSize(Core::Box::CONTENT).y;

	value_element->SetOffset(parent_element->GetBox().GetPosition(Core::Box::CONTENT), parent_element);
	value_element->SetBox(Core::Box(size));

	box_layout_dirty = true;
	value_layout_dirty = true;
}

// Sets the value of the widget.
void WidgetDropDown::SetValue(const Rml::Core::String& _value)
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
const Rml::Core::String& WidgetDropDown::GetValue() const
{
	return value;
}

// Sets the index of the selection. If the new index lies outside of the bounds, it will be clamped.
void WidgetDropDown::SetSelection(int selection, bool force)
{
	Rml::Core::String new_value;

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

		Rml::Core::String value_rml;
		if (selected_option >= 0) 
		{
			auto* el = options[selected_option].GetElement();
			el->GetInnerRML(value_rml);
			el->SetPseudoClass("checked", true);
		}


		value_element->SetInnerRML(value_rml);
		value_layout_dirty = true;

		Rml::Core::Dictionary parameters;
		parameters["value"] = value;
		parent_element->DispatchEvent(Core::EventId::Change, parameters);
	}
}

// Returns the index of the currently selected item.
int WidgetDropDown::GetSelection() const
{
	return selected_option;
}

// Adds a new option to the select control.
int WidgetDropDown::AddOption(const Rml::Core::String& rml, const Rml::Core::String& new_value, int before, bool select, bool selectable)
{
	Core::ElementPtr element = Core::Factory::InstanceElement(selection_element, "*", "option", Rml::Core::XMLAttributes());
	element->SetInnerRML(rml);

	int result = AddOption(std::move(element), new_value, before, select, selectable);

	return result;
}

int WidgetDropDown::AddOption(Rml::Core::ElementPtr element, const Rml::Core::String& new_value, int before, bool select, bool selectable)
{
	static const Core::String str_option = "option";

	if (element->GetTagName() != str_option)
	{
		Core::Log::Message(Core::Log::LT_WARNING, "A child of '%s' must be of type 'option' but '%s' was given. See element '%s'.", parent_element->GetTagName().c_str(), element->GetTagName().c_str(), parent_element->GetAddress().c_str());
		return -1;
	}

	// Force to block display. Register a click handler so we can be notified of selection.
	element->SetProperty(Core::PropertyId::Display, Core::Property(Core::Style::Display::Block));
	element->SetProperty(Core::PropertyId::Clip, Core::Property(Core::Style::Clip::Type::Auto));
	element->AddEventListener(Core::EventId::Click, this);

	int option_index;
	if (before < 0 || before >= (int)options.size())
	{
		Core::Element* ptr = selection_element->AppendChild(std::move(element));
		options.push_back(SelectOption(ptr, new_value, selectable));
		option_index = (int)options.size() - 1;
	}
	else
	{
		Core::Element* ptr = selection_element->InsertBefore(std::move(element), selection_element->GetChild(before));
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
	options[index].GetElement()->RemoveEventListener(Core::EventId::Click, this);
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

void WidgetDropDown::ProcessEvent(Core::Event& event)
{
	if (parent_element->IsDisabled()) 
		return;

	// Process the button onclick
	switch (event.GetId())
	{
	case Core::EventId::Click:
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
			Core::Element* element = event.GetTargetElement();
			while (element && element != parent_element)
			{
				if (element == selection_element)
					return;
				element = element->GetParentNode();
			}

			if (selection_element->GetComputedValues().visibility == Core::Style::Visibility::Hidden)
				ShowSelectBox(true);
			else
				ShowSelectBox(false);
		}
	}
	break;
	case Core::EventId::Focus:
	{
		if (event.GetTargetElement() == parent_element)
		{
			value_element->SetPseudoClass("focus", true);
			button_element->SetPseudoClass("focus", true);
		}
	}
	break;
	case Core::EventId::Blur:
	{
		if (event.GetTargetElement() == parent_element)
		{
			ShowSelectBox(false);
			value_element->SetPseudoClass("focus", false);
			button_element->SetPseudoClass("focus", false);
		}
	}
	break;
	case Core::EventId::Keydown:
	{
		Core::Input::KeyIdentifier key_identifier = (Core::Input::KeyIdentifier) event.GetParameter< int >("key_identifier", 0);

		switch (key_identifier)
		{
		case Core::Input::KI_UP:
			SetSelection((selected_option - 1 + (int)options.size()) % (int)options.size());
			break;
		case Core::Input::KI_DOWN:
			SetSelection((selected_option + 1) % (int)options.size());
			break;
		default:
			break;
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
		selection_element->SetProperty(Core::PropertyId::Visibility, Core::Property(Core::Style::Visibility::Visible));
		value_element->SetPseudoClass("checked", true);
		button_element->SetPseudoClass("checked", true);
	}
	else
	{
		selection_element->SetProperty(Core::PropertyId::Visibility, Core::Property(Core::Style::Visibility::Hidden));
		value_element->SetPseudoClass("checked", false);
		button_element->SetPseudoClass("checked", false);
	}
	box_visible = show;
}

}
}
