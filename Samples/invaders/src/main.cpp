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

#include <Rocket/Core.h>
#include <Rocket/Controls.h>
#include <Rocket/Debugger.h>
#include <Shell.h>
#include <Input.h>
#include "DecoratorInstancerDefender.h"
#include "DecoratorInstancerStarfield.h"
#include "ElementGame.h"
#include "EventHandlerHighScore.h"
#include "EventHandlerOptions.h"
#include "EventHandlerStartGame.h"
#include "EventInstancer.h"
#include "EventManager.h"
#include "HighScores.h"
#include "HighScoresNameFormatter.h"
#include "HighScoresShipFormatter.h"

Rocket::Core::Context* context = NULL;

void GameLoop()
{
	context->Update();

	glClear(GL_COLOR_BUFFER_BIT);
	context->Render();
	Shell::FlipBuffers();
}

#if defined ROCKET_PLATFORM_WIN32
#include <windows.h>
int APIENTRY WinMain(HINSTANCE, HINSTANCE, char*, int)
#else
int main(int, char**)
#endif
{
	// Generic OS initialisation, creates a window and attaches OpenGL.
	if (!Shell::Initialise("../Samples/invaders/") ||
		!Shell::OpenWindow("Rocket Invaders from Mars", true))
	{
		Shell::Shutdown();
		return -1;
	}

	// Rocket initialisation.
	ShellRenderInterfaceOpenGL opengl_renderer;
	Rocket::Core::SetRenderInterface(&opengl_renderer);
    opengl_renderer.SetViewport(1024,768);

	ShellSystemInterface system_interface;
	Rocket::Core::SetSystemInterface(&system_interface);

	Rocket::Core::Initialise();
	// Initialise the Rocket Controls library.
	Rocket::Controls::Initialise();

	// Create the main Rocket context and set it on the shell's input layer.
	context = Rocket::Core::CreateContext("main", Rocket::Core::Vector2i(1024, 768));
	if (context == NULL)
	{
		Rocket::Core::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	// Initialise the Rocket debugger.
	Rocket::Debugger::Initialise(context);
	Input::SetContext(context);

	// Load the font faces required for Invaders.
	Shell::LoadFonts("../assets/");

	// Register Invader's custom element and decorator instancers.
	Rocket::Core::ElementInstancer* element_instancer = new Rocket::Core::ElementInstancerGeneric< ElementGame >();
	Rocket::Core::Factory::RegisterElementInstancer("game", element_instancer);
	element_instancer->RemoveReference();

	Rocket::Core::DecoratorInstancer* decorator_instancer = new DecoratorInstancerStarfield();
	Rocket::Core::Factory::RegisterDecoratorInstancer("starfield", decorator_instancer);
	decorator_instancer->RemoveReference();

	decorator_instancer = new DecoratorInstancerDefender();
	Rocket::Core::Factory::RegisterDecoratorInstancer("defender", decorator_instancer);
	decorator_instancer->RemoveReference();

	// Register Invader's data formatters
	HighScoresNameFormatter name_formatter;
	HighScoresShipFormatter ship_formatter;

	// Construct the game singletons.
	HighScores::Initialise();

	// Initialise the event instancer and handlers.
	EventInstancer* event_instancer = new EventInstancer();
	Rocket::Core::Factory::RegisterEventListenerInstancer(event_instancer);
	event_instancer->RemoveReference();

	EventManager::RegisterEventHandler("start_game", new EventHandlerStartGame());
	EventManager::RegisterEventHandler("high_score", new EventHandlerHighScore());
	EventManager::RegisterEventHandler("options", new EventHandlerOptions());

	// Start the game.
	if (EventManager::LoadWindow("background") &&
		EventManager::LoadWindow("main_menu"))
		Shell::EventLoop(GameLoop);
		
	// Shut down the game singletons.
	HighScores::Shutdown();

	// Release the event handlers.
	EventManager::Shutdown();

	// Shutdown Rocket.
	context->RemoveReference();
	Rocket::Core::Shutdown();

	Shell::CloseWindow();
	Shell::Shutdown();
	
	return 0;
}
