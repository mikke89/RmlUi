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

#include "ElementGame.h"
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/Factory.h>
#include "Defender.h"
#include "Game.h"

ElementGame::ElementGame(const Rml::Core::String& tag) : Rml::Core::Element(tag)
{
	game = new Game();
}

ElementGame::~ElementGame()
{		
	delete game;
}

// Intercepts and handles key events.
void ElementGame::ProcessEvent(Rml::Core::Event& event)
{
	if (event == Rml::Core::EventId::Keydown ||
		event == Rml::Core::EventId::Keyup)
	{
		bool key_down = (event == Rml::Core::EventId::Keydown);
		Rml::Core::Input::KeyIdentifier key_identifier = (Rml::Core::Input::KeyIdentifier) event.GetParameter< int >("key_identifier", 0);		

		// Process left and right keys
		if (key_down)
		{
			if (key_identifier == Rml::Core::Input::KI_LEFT)
				game->GetDefender()->StartMove(-1.0f);
			if (key_identifier == Rml::Core::Input::KI_RIGHT)
				game->GetDefender()->StartMove(1.0f);
			if (key_identifier == Rml::Core::Input::KI_SPACE)
				game->GetDefender()->Fire();
		}		
		else if (!key_down)
		{
			if (key_identifier == Rml::Core::Input::KI_LEFT)
				game->GetDefender()->StopMove(-1.0f);
			if (key_identifier == Rml::Core::Input::KI_RIGHT)
				game->GetDefender()->StopMove(1.0f);				
		}
	}

	if (event == Rml::Core::EventId::Load)
	{
		game->Initialise();
	}
}

// Updates the game.
void ElementGame::OnUpdate()
{
	game->Update();

	if (game->IsGameOver())
		DispatchEvent("gameover", Rml::Core::Dictionary());
}

// Renders the game.
void ElementGame::OnRender()
{
	game->Render();
}

void ElementGame::OnChildAdd(Rml::Core::Element* element)
{
	Rml::Core::Element::OnChildAdd(element);

	if (element == this)
	{
		GetOwnerDocument()->AddEventListener(Rml::Core::EventId::Load, this);
		GetOwnerDocument()->AddEventListener(Rml::Core::EventId::Keydown, this);
		GetOwnerDocument()->AddEventListener(Rml::Core::EventId::Keyup, this);
	}
}
