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

#include "EventHandlerStartGame.h"
#include <Rocket/Core/Context.h>
#include <Rocket/Core/ElementDocument.h>
#include <Rocket/Core/ElementUtilities.h>
#include <Rocket/Core/Event.h>
#include "EventManager.h"
#include "GameDetails.h"

// The game's element context (declared in main.cpp).
extern Rocket::Core::Context* context;

EventHandlerStartGame::EventHandlerStartGame()
{
}

EventHandlerStartGame::~EventHandlerStartGame()
{
}

void EventHandlerStartGame::ProcessEvent(Rocket::Core::Event& event, const Rocket::Core::String& value)
{
	if (value == "start")
	{
		// Set the difficulty.
		Rocket::Core::String difficulty = event.GetParameter< Rocket::Core::String >("difficulty", "easy");
		if (difficulty == "hard")
			GameDetails::SetDifficulty(GameDetails::HARD);
		else
			GameDetails::SetDifficulty(GameDetails::EASY);

		// Set the defender colour.
		Rocket::Core::StringList colour_components;
		Rocket::Core::StringUtilities::ExpandString(colour_components, event.GetParameter< Rocket::Core::String >("colour", "255,255,255"));

		Rocket::Core::Colourb colour(255, 255, 255);
		for (size_t i = 0; i < colour_components.size() && i < 3; ++i)
		{
			int colour_component = atoi(colour_components[i].CString());
			if (colour_component < 0)
				colour_component = 0;
			if (colour_component > 255)
				colour_component = 255;

			colour[i] = (Rocket::Core::byte) colour_component;
		}

		GameDetails::SetDefenderColour(colour);

		// Go to the game window and close the start game window.
		if (EventManager::LoadWindow("game"))
		{
			Rocket::Core::ElementUtilities::GetElementById(context->GetFocusElement(), "game")->Focus();
			event.GetTargetElement()->GetOwnerDocument()->Close();
		}
	}
}
