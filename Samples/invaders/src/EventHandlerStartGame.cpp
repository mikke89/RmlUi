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

#include "EventHandlerStartGame.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/ElementUtilities.h>
#include <RmlUi/Core/Event.h>
#include "EventManager.h"
#include "GameDetails.h"

// The game's element context (declared in main.cpp).
extern Rml::Context* context;

EventHandlerStartGame::EventHandlerStartGame()
{
}

EventHandlerStartGame::~EventHandlerStartGame()
{
}

void EventHandlerStartGame::ProcessEvent(Rml::Event& event, const Rml::String& value)
{
	if (value == "start")
	{
		// Set the difficulty.
		Rml::String difficulty = event.GetParameter< Rml::String >("difficulty", "easy");
		if (difficulty == "hard")
			GameDetails::SetDifficulty(GameDetails::HARD);
		else
			GameDetails::SetDifficulty(GameDetails::EASY);

		// Set the defender colour.
		Rml::StringList colour_components;
		Rml::StringUtilities::ExpandString(colour_components, event.GetParameter< Rml::String >("colour", "255,255,255"));

		Rml::Colourb colour(255, 255, 255);
		for (size_t i = 0; i < colour_components.size() && i < 3; ++i)
		{
			int colour_component = atoi(colour_components[i].c_str());
			if (colour_component < 0)
				colour_component = 0;
			if (colour_component > 255)
				colour_component = 255;

			colour[i] = (Rml::byte) colour_component;
		}

		GameDetails::SetDefenderColour(colour);

		// Go to the game window and close the start game window.
		if (EventManager::LoadWindow("game"))
		{
			Rml::ElementUtilities::GetElementById(context->GetFocusElement(), "game")->Focus();
			event.GetTargetElement()->GetOwnerDocument()->Close();
		}
	}
}
