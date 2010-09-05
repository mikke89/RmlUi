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

#include "ElementGame.h"
#include <Rocket/Core/ElementDocument.h>
#include <Rocket/Core/Input.h>
#include "Defender.h"
#include "EventManager.h"
#include "Game.h"

ElementGame::ElementGame(const Rocket::Core::String& tag) : Rocket::Core::Element(tag)
{
	game = new Game();
}

ElementGame::~ElementGame()
{		
	delete game;
}

// Intercepts and handles key events.
void ElementGame::ProcessEvent(Rocket::Core::Event& event)
{
	Rocket::Core::Element::ProcessEvent(event);

	if (event == "keydown" ||
		event == "keyup")
	{
		bool key_down = event == "keydown";
		Rocket::Core::Input::KeyIdentifier key_identifier = (Rocket::Core::Input::KeyIdentifier) event.GetParameter< int >("key_identifier", 0);

		if (key_identifier == Rocket::Core::Input::KI_ESCAPE &&
			!key_down)
		{
			EventManager::LoadWindow("pause");
		}

		// Process left and right keys
		if (key_down)
		{
			if (key_identifier == Rocket::Core::Input::KI_LEFT)
				game->GetDefender()->StartMove(-1.0f);
			if (key_identifier == Rocket::Core::Input::KI_RIGHT)
				game->GetDefender()->StartMove(1.0f);
			if (key_identifier == Rocket::Core::Input::KI_SPACE)
				game->GetDefender()->Fire();
		}		
		else if (!key_down)
		{
			if (key_identifier == Rocket::Core::Input::KI_LEFT)
				game->GetDefender()->StopMove(-1.0f);
			if (key_identifier == Rocket::Core::Input::KI_RIGHT)
				game->GetDefender()->StopMove(1.0f);				
		}
	}

	if (event == "load")
	{
		game->Initialise();
	}
}

// Updates the game.
void ElementGame::OnUpdate()
{
	game->Update();
}

// Renders the game.
void ElementGame::OnRender()
{
	game->Render();
}

void ElementGame::OnChildAdd(Rocket::Core::Element* element)
{
	Rocket::Core::Element::OnChildAdd(element);

	if (element == this)
		GetOwnerDocument()->AddEventListener("load", this);
}
