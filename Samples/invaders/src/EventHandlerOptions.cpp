/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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
#include <Rocket/Core/ElementDocument.h>
#include <Rocket/Core/ElementUtilities.h>
#include <Rocket/Core/Event.h>
#include <Rocket/Controls/ElementFormControlInput.h>
#include "EventManager.h"
#include "GameDetails.h"

EventHandlerOptions::EventHandlerOptions()
{
}

EventHandlerOptions::~EventHandlerOptions()
{
}

void EventHandlerOptions::ProcessEvent(Rocket::Core::Event& event, const Rocket::Core::String& value)
{
	// Sent from the 'onload' of the options screen; we set the options on the interface to match those previously set
	// this game session.
	if (value == "restore")
	{
		// Fetch the document from the target of the 'onload' event. From here we can fetch the options elements by ID
		// to manipulate them directly.
		Rocket::Core::ElementDocument* options_body = event.GetTargetElement()->GetOwnerDocument();
		if (options_body == NULL)
			return;

		// Get the current graphics setting, and translate that into the ID of the radio button we need to set.
		Rocket::Core::String graphics_option_id;
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
		Rocket::Controls::ElementFormControlInput* graphics_option = dynamic_cast< Rocket::Controls::ElementFormControlInput* >(options_body->GetElementById(graphics_option_id));
		if (graphics_option != NULL)
			graphics_option->SetAttribute("checked", "");

		// Fetch the reverb option by ID and set its checked status from the game options.
		Rocket::Controls::ElementFormControlInput* reverb_option = dynamic_cast< Rocket::Controls::ElementFormControlInput* >(options_body->GetElementById("reverb"));
		if (reverb_option != NULL)
		{
			if (GameDetails::GetReverb())
				reverb_option->SetAttribute("checked", "");
			else
				reverb_option->RemoveAttribute("checked");
		}

		// Similarly, fetch the 3D spatialisation option by ID and set its checked status.
		Rocket::Controls::ElementFormControlInput* spatialisation_option = dynamic_cast< Rocket::Controls::ElementFormControlInput* >(options_body->GetElementById("3d"));
		if (spatialisation_option != NULL)
		{
			if (GameDetails::Get3DSpatialisation())
				spatialisation_option->SetAttribute("checked", "");
			else
				spatialisation_option->RemoveAttribute("checked");
		}
	}

	// Sent from the 'onsubmit' action of the options menu; we read the values sent from the form and make the
	// necessary changes on the game details structure.
	else if (value == "store")
	{
		// First check which button was clicked to submit the form; if it was 'cancel', then we don't want to
		// propagate the changes.
		if (event.GetParameter< Rocket::Core::String >("submit", "cancel") == "accept")
		{
			// Fetch the results of the form submission. These are stored as parameters directly on the event itself.
			// Like HTML form events, the name of the parameter is the 'name' attribute of the control, and the value
			// is whatever was put into the 'value' attribute. Checkbox values are only sent through if the box was
			// clicked. Radio buttons send through one value for the active button.
			Rocket::Core::String graphics = event.GetParameter< Rocket::Core::String >("graphics", "ok");
			bool reverb = event.GetParameter< Rocket::Core::String >("reverb", "") == "true";
			bool spatialisation = event.GetParameter< Rocket::Core::String >("3d", "") == "true";

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
		Rocket::Core::ElementDocument* options_body = event.GetTargetElement()->GetOwnerDocument();
		if (options_body == NULL)
			return;

		Rocket::Core::Element* bad_warning = options_body->GetElementById("bad_warning");
		if (bad_warning)
		{
			// The 'value' parameter of an onchange event is set to the value the control would send if it was
			// submitted; so, the empty string if it is clear or to the 'value' attribute of the control if it is set.
			if (event.GetParameter< Rocket::Core::String >("value", "").Empty())
				bad_warning->SetProperty("display", "none");
			else
				bad_warning->SetProperty("display", "block");
		}
	}
}
