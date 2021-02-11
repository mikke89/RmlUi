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
#include <RmlUi/Lua.h>

#include <Input.h>
#include <Shell.h>
#include <ShellRenderInterfaceOpenGL.h>
#include "DecoratorInstancerDefender.h"
#include "DecoratorInstancerStarfield.h"
#include "ElementGame.h"
#include "HighScores.h"
#include "LuaInterface.h"

Rml::Context* context = nullptr;

void DoAllocConsole();

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
int APIENTRY WinMain(HINSTANCE, HINSTANCE, char*, int)
#else
int main(int, char**)
#endif
{

#ifdef RMLUI_PLATFORM_WIN32
	DoAllocConsole();
#endif

	int window_width = 1024;
	int window_height = 768;

	ShellRenderInterfaceOpenGL opengl_renderer;
	shell_renderer = &opengl_renderer;

	// Generic OS initialisation, creates a window and attaches OpenGL.
	if (!Shell::Initialise() ||
		!Shell::OpenWindow("RmlUi Invaders from Mars (Lua Powered)", shell_renderer, window_width, window_height, false))
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

	// Initialise the Lua interface
	Rml::Lua::Initialise();

	// Create the main RmlUi context and set it on the shell's input layer.
	context = Rml::CreateContext("main", Rml::Vector2i(window_width, window_height));
	if (context == nullptr)
	{
		Rml::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	Rml::Debugger::Initialise(context);
	Input::SetContext(context);
	Shell::SetContext(context);

	// Load the font faces required for Invaders.
	Shell::LoadFonts("assets/");

	// Register Invader's custom decorator instancers.
	DecoratorInstancerStarfield decorator_starfield;
	DecoratorInstancerDefender decorator_defender;
	Rml::Factory::RegisterDecoratorInstancer("starfield", &decorator_starfield);
	Rml::Factory::RegisterDecoratorInstancer("defender", &decorator_defender);

	// Construct the game singletons.
	HighScores::Initialise(context);

	// Fire off the startup script.
    LuaInterface::Initialise(Rml::Lua::Interpreter::GetLuaState()); //the tables/functions defined in the samples
    Rml::Lua::Interpreter::LoadFile(Rml::String("luainvaders/lua/start.lua"));

	Shell::EventLoop(GameLoop);	

	// Shut down the game singletons.
	HighScores::Shutdown();

	// Shutdown RmlUi.
	Rml::Shutdown();

	Shell::CloseWindow();
	Shell::Shutdown();

	return 0;
}

#ifdef RMLUI_PLATFORM_WIN32

#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <process.h>

void DoAllocConsole()
{
	static const WORD MAX_CONSOLE_LINES = 500;
	int hConHandle;
	HANDLE lStdHandle;
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	FILE *fp;

	// allocate a console for this app
	AllocConsole();

	// set the screen buffer to be big enough to let us scroll text
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
	coninfo.dwSize.Y = MAX_CONSOLE_LINES;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

	// redirect unbuffered STDOUT to the console
	lStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "w" );

	*stdout = *fp;
	setvbuf( stdout, nullptr, _IONBF, 0 );

	// redirect unbuffered STDIN to the console
	lStdHandle = GetStdHandle(STD_INPUT_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "r" );

	*stdin = *fp;
	setvbuf( stdin, nullptr, _IONBF, 0 );

	// redirect unbuffered STDERR to the console
	lStdHandle = GetStdHandle(STD_ERROR_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "w" );
	*stderr = *fp;

	setvbuf( stderr, nullptr, _IONBF, 0 );
	ShowWindow(GetConsoleWindow(), SW_SHOW);
}
#endif
