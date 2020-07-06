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

#include "EventHandlerOptions.h"
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/ElementUtilities.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include "EventManager.h"
#include "GameDetails.h"

EventHandlerOptions::EventHandlerOptions()
{
}

EventHandlerOptions::~EventHandlerOptions()
{
}

void EventHandlerOptions::ProcessEvent(Rml::Event& event, const Rml::String& value)
{
	// Sent from the 'onload' of the options screen; we set the options on the interface to match those previously set
	// this game session.
	if (value == "restore")
	{
		// Fetch the document from the target of the 'onload' event. From here we can fetch the options elements by ID
		// to manipulate them directly.
		Rml::ElementDocument* options_body = event.GetTargetElement()->GetOwnerDocument();
		if (options_body == nullptr)
			return;

		// Get the current graphics setting, and translate that into the ID of the radio button we need to set.
		Rml::String graphics_option_id;
		switch (GameDetails::GetGraphicsQuality())
		{
			case GameDetails::GOOD:		graphics_option_id = "good"; break;
			case GameDetails::OK:		graphics_option_id = "ok"; break;
			case GameDetails::BAD:		graphics_option_id = "bad"; break;
			default:					break;
		}

		// Fetch the radio button from the document by ID, cast it to a radio button interface and set it as checked.
		// This will automatically pop the other radio buttons in the set. Note that we could have not cast and called
		// the 'Click()' function instead, but this method will avoid event overhead.
		Rml::ElementFormControlInput* graphics_option = rmlui_dynamic_cast< Rml::ElementFormControlInput* >(options_body->GetElementById(graphics_option_id));
		if (graphics_option != nullptr)
			graphics_option->SetAttribute("checked", "");

		// Fetch the reverb option by ID and set its checked status from the game options.
		Rml::ElementFormControlInput* reverb_option = rmlui_dynamic_cast< Rml::ElementFormControlInput* >(options_body->GetElementById("reverb"));
		if (reverb_option != nullptr)
		{
			if (GameDetails::GetReverb())
				reverb_option->SetAttribute("checked", "");
			else
				reverb_option->RemoveAttribute("checked");
		}

		// Similarly, fetch the 3D spatialisation option by ID and set its checked status.
		Rml::ElementFormControlInput* spatialisation_option = rmlui_dynamic_cast< Rml::ElementFormControlInput* >(options_body->GetElementById("3d"));
		if (spatialisation_option != nullptr)
		{
			if (GameDetails::Get3DSpatialisation())
				spatialisation_option->SetAttribute("checked", "");
			else
				spatialisation_option->RemoveAttribute("checked");
		}

		// Disable the accept button when default values are given
		Rml::ElementFormControlInput* accept = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(options_body->GetElementById("accept"));
		if (accept != nullptr)
		{
			accept->SetDisabled(true);
		}
	}

	// Sent from the 'onsubmit' action of the options menu; we read the values sent from the form and make the
	// necessary changes on the game details structure.
	else if (value == "store")
	{
		// First check which button was clicked to submit the form; if it was 'cancel', then we don't want to
		// propagate the changes.
		if (event.GetParameter< Rml::String >("submit", "cancel") == "accept")
		{
			// Fetch the results of the form submission. These are stored as parameters directly on the event itself.
			// Like HTML form events, the name of the parameter is the 'name' attribute of the control, and the value
			// is whatever was put into the 'value' attribute. Checkbox values are only sent through if the box was
			// clicked. Radio buttons send through one value for the active button.
			Rml::String graphics = event.GetParameter< Rml::String >("graphics", "ok");
			bool reverb = event.GetParameter< Rml::String >("reverb", "") == "true";
			bool spatialisation = event.GetParameter< Rml::String >("3d", "") == "true";

			if (graphics == "good")
				GameDetails::SetGraphicsQuality(GameDetails::GOOD);
			else if (graphics == "ok")
				GameDetails::SetGraphicsQuality(GameDetails::OK);
			else if (graphics == "bad")
				GameDetails::SetGraphicsQuality(GameDetails::BAD);

			GameDetails::SetReverb(reverb);
			GameDetails::Set3DSpatialisation(spatialisation);
		}
	}

	// This event is sent from the 'onchange' of the bad graphics radio button. It shows or hides the bad graphics
	// warning message.
	else if (value == "bad_graphics")
	{
		using namespace Rml;
		ElementDocument* options_body = event.GetTargetElement()->GetOwnerDocument();
		if (options_body == nullptr)
			return;

		Element* bad_warning = options_body->GetElementById("bad_warning");
		if (bad_warning)
		{
			// The 'value' parameter of an onchange event is set to the value the control would send if it was
			// submitted; so, the empty string if it is clear or to the 'value' attribute of the control if it is set.
			if (event.GetParameter< String >("value", "").empty())
				bad_warning->SetProperty(PropertyId::Display, Property(Style::Display::None));
			else
				bad_warning->SetProperty(PropertyId::Display, Property(Style::Display::Block));
		}
	}
	else if (value == "enable_accept")
	{
		Rml::ElementDocument* options_body = event.GetTargetElement()->GetOwnerDocument();
		if (options_body == nullptr)
			return;

		// Enable the accept button when values are changed
		Rml::ElementFormControlInput* accept = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(options_body->GetElementById("accept"));
		if (accept != nullptr)
		{
			accept->SetDisabled(false);
		}
	}
}
