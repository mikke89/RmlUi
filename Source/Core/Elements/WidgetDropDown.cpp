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

#include "WidgetDropDown.h"
#include "../../../Include/RmlUi/Core/ComputedValues.h"
#include "../../../Include/RmlUi/Core/Context.h"
#include "../../../Include/RmlUi/Core/ElementDocument.h"
#include "../../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../../Include/RmlUi/Core/Elements/ElementFormControl.h"
#include "../../../Include/RmlUi/Core/Event.h"
#include "../../../Include/RmlUi/Core/Factory.h"
#include "../../../Include/RmlUi/Core/Input.h"
#include "../../../Include/RmlUi/Core/Math.h"
#include "../../../Include/RmlUi/Core/Profiling.h"
#include "../../../Include/RmlUi/Core/Property.h"
#include "../DataModel.h"

namespace Rml {

WidgetDropDown::WidgetDropDown(ElementFormControl* element)
{
	parent_element = element;

	lock_selection = false;
	selection_dirty = false;
	box_layout_dirty = false;
	value_rml_dirty = false;
	value_layout_dirty = false;
	box_visible = false;

	// Create the button and selection elements.
	button_element = parent_element->AppendChild(Factory::InstanceElement(parent_element, "*", "selectarrow", XMLAttributes()), false);
	value_element = parent_element->AppendChild(Factory::InstanceElement(parent_element, "*", "selectvalue", XMLAttributes()), false);
	selection_element = parent_element->AppendChild(Factory::InstanceElement(parent_element, "*", "selectbox", XMLAttributes()), false);

	value_element->SetProperty(PropertyId::OverflowX, Property(Style::Overflow::Hidden));
	value_element->SetProperty(PropertyId::OverflowY, Property(Style::Overflow::Hidden));

	selection_element->SetProperty(PropertyId::Visibility, Property(Style::Visibility::Hidden));
	selection_element->SetProperty(PropertyId::ZIndex, Property(1.0f, Unit::NUMBER));
	selection_element->SetProperty(PropertyId::Clip, Property(Style::Clip::Type::None));
	selection_element->SetProperty(PropertyId::OverflowY, Property(Style::Overflow::Auto));

	// Prevent scrolling in the parent document when the mouse is inside the selection box.
	selection_element->SetProperty(PropertyId::OverscrollBehavior, Property(Style::OverscrollBehavior::Contain));

	parent_element->AddEventListener(EventId::Click, this, true);
	parent_element->AddEventListener(EventId::Blur, this);
	parent_element->AddEventListener(EventId::Focus, this);
	parent_element->AddEventListener(EventId::Keydown, this, true);
}

WidgetDropDown::~WidgetDropDown()
{
	// We shouldn't clear the options ourselves, as removing the element will automatically clear children.
	//   However, we do need to remove events of children.
	const int num_options = selection_element->GetNumChildren();
	for (int i = 0; i < num_options; i++)
		selection_element->GetChild(i)->RemoveEventListener(EventId::Click, this);

	parent_element->RemoveEventListener(EventId::Click, this, true);
	parent_element->RemoveEventListener(EventId::Blur, this);
	parent_element->RemoveEventListener(EventId::Focus, this);
	parent_element->RemoveEventListener(EventId::Keydown, this, true);

	DetachScrollEvent();
}

void WidgetDropDown::OnUpdate()
{
	if (selection_dirty)
	{
		// Find the best option element to select in the following priority:
		//  1. First option with 'selected' attribute.
		//  2. An option whose 'value' attribute matches the select 'value' attribute.
		//  3. The first option.
		// The select element's value may change as a result of this.
		const String select_value = parent_element->GetAttribute("value", String());
		Element* select_option = selection_element->GetFirstChild();

		const int num_options = selection_element->GetNumChildren();
		for (int i = 0; i < num_options; i++)
		{
			Element* option = selection_element->GetChild(i);
			if (option->HasAttribute("selected"))
			{
				select_option = option;
				break;
			}
			else if (!select_value.empty() && select_value == option->GetAttribute("value", String()))
			{
				select_option = option;
			}
		}

		if (select_option)
			SetSelection(select_option);

		selection_dirty = false;
	}

	if (value_rml_dirty)
	{
		String value_rml;
		const int selection = GetSelection();

		if (Element* option = selection_element->GetChild(selection))
		{
			option->GetInnerRML(value_rml);
			if (auto model = value_element->GetDataModel())
				model->CopyAliases(option, value_element);
		}
		else
		{
			if (auto model = value_element->GetDataModel())
				model->EraseAliases(value_element);
			value_rml = parent_element->GetValue();
		}

		value_element->SetInnerRML(value_rml);

		value_rml_dirty = false;
		value_layout_dirty = true;
	}
}

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
		if (selection_element->GetLocalStyleProperties().count(PropertyId::Height) == 1)
		{
			selection_element->RemoveProperty(PropertyId::Height);
			selection_element->GetOwnerDocument()->UpdateDocument();
		}

		Box box;
		ElementUtilities::BuildBox(box, parent_element->GetBox().GetSize(), selection_element);

		// The user can use 'margin-left/top/bottom' to offset the box away from the 'select' element, respectively
		// horizontally, vertically when box below, and vertically when box above.
		const float offset_x = box.GetEdge(BoxArea::Margin, BoxEdge::Left);
		const float offset_y_below = parent_element->GetBox().GetSize(BoxArea::Border).y + box.GetEdge(BoxArea::Margin, BoxEdge::Top);
		const float offset_y_above = -box.GetEdge(BoxArea::Margin, BoxEdge::Bottom);

		float window_height = 100'000.f;
		if (Context* context = parent_element->GetContext())
			window_height = float(context->GetDimensions().y);

		const float absolute_y = parent_element->GetAbsoluteOffset(BoxArea::Border).y;

		const float height_below = window_height - absolute_y - offset_y_below;
		const float height_above = absolute_y + offset_y_above;

		// Format the selection box and retrieve the 'native' height occupied by all the options, while respecting
		// the 'min/max-height' properties.
		ElementUtilities::FormatElement(selection_element, parent_element->GetBox().GetSize(BoxArea::Border));
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
			const float padding_border_size = box.GetEdge(BoxArea::Border, BoxEdge::Top) + box.GetEdge(BoxArea::Border, BoxEdge::Bottom) +
				box.GetEdge(BoxArea::Padding, BoxEdge::Top) + box.GetEdge(BoxArea::Padding, BoxEdge::Bottom);

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
			selection_element->SetProperty(PropertyId::Height, Property(height, Unit::PX));
			selection_element->GetOwnerDocument()->UpdateDocument();
			ElementUtilities::FormatElement(selection_element, parent_element->GetBox().GetSize(BoxArea::Border));

			selection_element->SetOffset(Vector2f(offset_x, offset_y), parent_element);
		}

		box_layout_dirty = false;
	}

	if (value_layout_dirty)
	{
		ElementUtilities::FormatElement(value_element, parent_element->GetBox().GetSize(BoxArea::Border));
		value_element->SetOffset(parent_element->GetBox().GetPosition(BoxArea::Content), parent_element);

		value_layout_dirty = false;
	}
}

void WidgetDropDown::OnLayout()
{
	RMLUI_ZoneScopedNC("DropDownLayout", 0x7FFF00);

	if (parent_element->IsDisabled())
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
	size.x = parent_element->GetBox().GetSize(BoxArea::Content).x - button_element->GetBox().GetSize(BoxArea::Margin).x;
	size.y = parent_element->GetBox().GetSize(BoxArea::Content).y;

	value_element->SetOffset(parent_element->GetBox().GetPosition(BoxArea::Content), parent_element);
	value_element->SetBox(Box(size));

	box_layout_dirty = true;
	value_layout_dirty = true;
}

void WidgetDropDown::OnValueChange(const String& value)
{
	if (!lock_selection)
	{
		Element* select_option = nullptr;
		const int num_options = selection_element->GetNumChildren();
		for (int i = 0; i < num_options; i++)
		{
			Element* option = selection_element->GetChild(i);
			Variant* variant = option->GetAttribute("value");
			if (variant && variant->Get<String>() == value)
			{
				select_option = option;
				break;
			}
		}

		if (select_option && !select_option->HasAttribute("selected"))
			SetSelection(select_option);
	}

	Dictionary parameters;
	parameters["value"] = value;
	parent_element->DispatchEvent(EventId::Change, parameters);

	value_rml_dirty = true;
}

void WidgetDropDown::SetSelection(Element* select_option, bool force)
{
	const String old_value = parent_element->GetAttribute("value", String());
	const String new_value = select_option ? select_option->GetAttribute("value", String()) : String();

	bool newly_selected = false;
	const int num_options = selection_element->GetNumChildren();
	for (int i = 0; i < num_options; i++)
	{
		Element* option = selection_element->GetChild(i);

		if (select_option == option)
		{
			if (!option->IsPseudoClassSet("checked"))
				newly_selected = true;
			option->SetAttribute("selected", String());
			option->SetPseudoClass("checked", true);
		}
		else
		{
			option->RemoveAttribute("selected");
			option->SetPseudoClass("checked", false);
		}
	}

	if (force || newly_selected || (old_value != new_value))
	{
		lock_selection = true;
		parent_element->SetAttribute("value", new_value);
		lock_selection = false;
	}

	value_rml_dirty = true;
}

void WidgetDropDown::SeekSelection(bool seek_forward)
{
	const int selected_option = GetSelection();
	const int num_options = selection_element->GetNumChildren();
	const int seek_direction = (seek_forward ? 1 : -1);

	for (int i = selected_option + seek_direction; i >= 0 && i < num_options; i += seek_direction)
	{
		Element* element = selection_element->GetChild(i);

		if (!element->HasAttribute("disabled") && element->IsVisible())
		{
			SetSelection(element);
			return;
		}
	}

	// No valid option found, leave selection unchanged.
}

int WidgetDropDown::GetSelection() const
{
	const int num_options = selection_element->GetNumChildren();
	for (int i = 0; i < num_options; i++)
	{
		if (selection_element->GetChild(i)->HasAttribute("selected"))
			return i;
	}

	return -1;
}

int WidgetDropDown::AddOption(const String& rml, const String& option_value, int before, bool select, bool selectable)
{
	ElementPtr element = Factory::InstanceElement(selection_element, "*", "option", XMLAttributes());
	element->SetInnerRML(rml);

	element->SetAttribute("value", option_value);

	if (select)
		element->SetAttribute("selected", String());
	if (!selectable)
		element->SetAttribute("disabled", String());

	int result = AddOption(std::move(element), before);

	return result;
}

int WidgetDropDown::AddOption(ElementPtr element, int before)
{
	if (element->GetTagName() != "option")
	{
		Log::Message(Log::LT_WARNING, "A child of '%s' must be of type 'option' but '%s' was given. See element '%s'.",
			parent_element->GetTagName().c_str(), element->GetTagName().c_str(), parent_element->GetAddress().c_str());
		return -1;
	}

	const int num_children_before = selection_element->GetNumChildren();
	int option_index;
	if (before < 0 || before >= num_children_before)
	{
		selection_element->AppendChild(std::move(element));
		option_index = num_children_before;
	}
	else
	{
		selection_element->InsertBefore(std::move(element), selection_element->GetChild(before));
		option_index = before;
	}

	return option_index;
}

void WidgetDropDown::RemoveOption(int index)
{
	Element* element = selection_element->GetChild(index);
	if (!element)
		return;

	selection_element->RemoveChild(element);
}

void WidgetDropDown::ClearOptions()
{
	while (Element* element = selection_element->GetLastChild())
		selection_element->RemoveChild(element);
}

Element* WidgetDropDown::GetOption(int index)
{
	return selection_element->GetChild(index);
}

int WidgetDropDown::GetNumOptions() const
{
	return selection_element->GetNumChildren();
}

void WidgetDropDown::OnChildAdd(Element* element)
{
	// We have a special case for 'data-for' here, since that element must remain hidden.
	if (element->GetParentNode() != selection_element || element->HasAttribute("data-for") || element->GetTagName() != "option")
		return;

	// Force to block display. Register a click handler so we can be notified of selection.
	element->SetProperty(PropertyId::Display, Property(Style::Display::Block));
	element->SetProperty(PropertyId::Clip, Property(Style::Clip::Type::Auto));
	element->AddEventListener(EventId::Click, this);

	// Select the option if appropriate.
	if (element->HasAttribute("selected"))
		SetSelection(element, true);

	selection_dirty = true;
	box_layout_dirty = true;
}

void WidgetDropDown::OnChildRemove(Element* element)
{
	if (element->GetParentNode() != selection_element)
		return;

	element->RemoveEventListener(EventId::Click, this);

	if (element->HasAttribute("selected"))
		SetSelection(nullptr);

	selection_dirty = true;
	box_layout_dirty = true;
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
			const int num_options = selection_element->GetNumChildren();
			// Find the element in the options and fire the selection event
			for (int i = 0; i < num_options; i++)
			{
				Element* current_element = event.GetCurrentElement();
				if (selection_element->GetChild(i) == current_element)
				{
					if (!event.GetCurrentElement()->HasAttribute("disabled"))
					{
						SetSelection(current_element);
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

			if (selection_element->GetComputedValues().visibility() == Style::Visibility::Hidden)
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
		Input::KeyIdentifier key_identifier = (Input::KeyIdentifier)event.GetParameter<int>("key_identifier", 0);

		auto HasVerticalNavigation = [this](PropertyId id) {
			if (const Property* p = parent_element->GetProperty(id))
			{
				if (p->unit != Unit::KEYWORD)
					return true;
				const Style::Nav nav = static_cast<Style::Nav>(p->Get<int>());
				if (nav == Style::Nav::Auto || nav == Style::Nav::Vertical)
					return true;
			}
			return false;
		};

		switch (key_identifier)
		{
		case Input::KI_UP:
			if (!box_visible && HasVerticalNavigation(PropertyId::NavUp))
				break;
			SeekSelection(false);
			event.StopPropagation();
			break;
		case Input::KI_DOWN:
			if (!box_visible && HasVerticalNavigation(PropertyId::NavDown))
				break;
			SeekSelection(true);
			event.StopPropagation();
			break;
		case Input::KI_RETURN:
		case Input::KI_NUMPADENTER:
			parent_element->Click();
			event.StopPropagation();
			break;
		default: break;
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
	default: break;
	}
}

void WidgetDropDown::ShowSelectBox(bool show)
{
	if (show)
	{
		selection_element->SetProperty(PropertyId::Visibility, Property(Style::Visibility::Visible));
		selection_element->SetPseudoClass("checked", true);
		value_element->SetPseudoClass("checked", true);
		button_element->SetPseudoClass("checked", true);
		box_layout_dirty = true;
		AttachScrollEvent();
	}
	else
	{
		selection_element->SetProperty(PropertyId::Visibility, Property(Style::Visibility::Hidden));
		selection_element->RemoveProperty(PropertyId::Height);
		selection_element->SetPseudoClass("checked", false);
		value_element->SetPseudoClass("checked", false);
		button_element->SetPseudoClass("checked", false);
		DetachScrollEvent();
	}

	box_visible = show;
}

} // namespace Rml
