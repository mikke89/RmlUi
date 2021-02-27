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

#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include <Shell.h>
#include <ShellRenderInterfaceOpenGL.h>
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

Rml::Context* context = nullptr;

ShellRenderInterfaceExtensions *shell_renderer;

void GameLoop()
{
	context->Update();

	shell_renderer->PrepareRenderBuffer();
	context->Render();
	shell_renderer->PresentRenderBuffer();
}

#if defined RMLUI_PLATFORM_WIN32
#include <windows.h>
int APIENTRY WinMain(HINSTANCE RMLUI_UNUSED_PARAMETER(instance_handle), HINSTANCE RMLUI_UNUSED_PARAMETER(previous_instance_handle), char* RMLUI_UNUSED_PARAMETER(command_line), int RMLUI_UNUSED_PARAMETER(command_show))
#else
int main(int RMLUI_UNUSED_PARAMETER(argc), char** RMLUI_UNUSED_PARAMETER(argv))
#endif
{
#ifdef RMLUI_PLATFORM_WIN32
	RMLUI_UNUSED(instance_handle);
	RMLUI_UNUSED(previous_instance_handle);
	RMLUI_UNUSED(command_line);
	RMLUI_UNUSED(command_show);
#else
	RMLUI_UNUSED(argc);
	RMLUI_UNUSED(argv);
#endif

#ifdef RMLUI_PLATFORM_WIN32
	AllocConsole();
#endif

	int window_width = 1024;
	int window_height = 768;

	ShellRenderInterfaceOpenGL opengl_renderer;
	shell_renderer = &opengl_renderer;

	// Generic OS initialisation, creates a window and attaches OpenGL.
	if (!Shell::Initialise() ||
		!Shell::OpenWindow("RmlUi Invaders from Mars", shell_renderer, window_width, window_height, false))
	{
		Shell::Shutdown();
		return -1;
	}

	// RmlUi initialisation.
	Rml::SetRenderInterface(&opengl_renderer);
	opengl_renderer.SetViewport(window_width, window_height);

	ShellSystemInterface system_interface;
	Rml::SetSystemInterface(&system_interface);

	Rml::Initialise();

	// Create the main RmlUi context and set it on the shell's input layer.
	context = Rml::CreateContext("main", Rml::Vector2i(window_width, window_height));
	if (context == nullptr)
	{
		Rml::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	// Initialise the RmlUi debugger.
	Rml::Debugger::Initialise(context);
	Input::SetContext(context);
	Shell::SetContext(context);

	// Load the font faces required for Invaders.
	Shell::LoadFonts("assets/");

	// Register Invader's custom element and decorator instancers.
	Rml::ElementInstancerGeneric< ElementGame > element_instancer_game;
	Rml::Factory::RegisterElementInstancer("game", &element_instancer_game);

	DecoratorInstancerStarfield decorator_instancer_starfield;
	DecoratorInstancerDefender decorator_instancer_defender;
	Rml::Factory::RegisterDecoratorInstancer("starfield", &decorator_instancer_starfield);
	Rml::Factory::RegisterDecoratorInstancer("defender", &decorator_instancer_defender);

	// Construct the game singletons.
	HighScores::Initialise(context);

	// Initialise the event instancer and handlers.
	EventInstancer event_listener_instancer;
	Rml::Factory::RegisterEventListenerInstancer(&event_listener_instancer);

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

	// Shutdown RmlUi.
	Rml::Shutdown();

	Shell::CloseWindow();
	Shell::Shutdown();

	return 0;
}
