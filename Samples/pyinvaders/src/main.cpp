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

#define _WIN32_WINNT 0x0500
#include "PythonInterface.h"

#include <Rocket/Core.h>
#include <Rocket/Controls.h>
#include <Rocket/Debugger.h>

#include <Input.h>
#include <Shell.h>
#include "DecoratorInstancerDefender.h"
#include "DecoratorInstancerStarfield.h"
#include "ElementGame.h"
#include "HighScores.h"

Rocket::Core::Context* context = NULL;

void DoAllocConsole();

ShellRenderInterfaceExtensions *shell_renderer;

void GameLoop()
{
	context->Update();

	shell_renderer->PrepareRenderBuffer();
	context->Render();
	shell_renderer->PresentRenderBuffer();
}

#if defined ROCKET_PLATFORM_WIN32
#include <windows.h>
int APIENTRY WinMain(HINSTANCE ROCKET_UNUSED_PARAMETER(instance_handle), HINSTANCE ROCKET_UNUSED_PARAMETER(previous_instance_handle), char* ROCKET_UNUSED_PARAMETER(command_line), int ROCKET_UNUSED_PARAMETER(command_show))
#else
int main(int ROCKET_UNUSED_PARAMETER(argc), char** ROCKET_UNUSED_PARAMETER(argv))
#endif
{
#ifdef ROCKET_PLATFORM_WIN32
	ROCKET_UNUSED(instance_handle);
	ROCKET_UNUSED(previous_instance_handle);
	ROCKET_UNUSED(command_line);
	ROCKET_UNUSED(command_show);
#else
	ROCKET_UNUSED(argc);
	ROCKET_UNUSED(argv);
#endif

// @TODO Make these lookup at runtime rather than using hard coded paths
#ifdef ROCKET_PLATFORM_LINUX
#define APP_PATH "../Samples/pyinvaders/"
#define ROCKET_PATH ""
#else
#define APP_PATH "../../Samples/pyinvaders/"
#define ROCKET_PATH "."
#endif

#ifdef ROCKET_PLATFORM_WIN32
	DoAllocConsole();
#endif

	int window_width = 1024;
	int window_height = 768;

	ShellRenderInterfaceOpenGL opengl_renderer;
	shell_renderer = &opengl_renderer;

	// @TODO switch to using variable for window sizes
	// Generic OS initialisation, creates a window and attaches OpenGL.
	if (!Shell::Initialise(APP_PATH) ||
		!Shell::OpenWindow("Rocket Invaders from Mars (Python Powered)", shell_renderer, window_width, window_height, false))
	{
		Shell::Shutdown();
		return -1;
	}

	// Rocket initialisation.
	Rocket::Core::SetRenderInterface(&opengl_renderer);
	opengl_renderer.SetViewport(window_width, window_height);

	ShellSystemInterface system_interface;
	Rocket::Core::SetSystemInterface(&system_interface);

	Rocket::Core::Initialise();
	// Initialise the Rocket Controls library.
	Rocket::Controls::Initialise();

	// Initialise the Python interface.
	PythonInterface::Initialise((Shell::GetExecutablePath() + (APP_PATH "python") + PATH_SEPARATOR + Shell::GetExecutablePath() + ROCKET_PATH).CString());

	// Create the main Rocket context and set it on the shell's input layer.
	context = Rocket::Core::CreateContext("main", Rocket::Core::Vector2i(window_width, window_height));
	if (context == NULL)
	{
		Rocket::Core::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	Rocket::Debugger::Initialise(context);
	Input::SetContext(context);
	shell_renderer->SetContext(context);

	// Load the font faces required for Invaders.
	Shell::LoadFonts("../assets/");

	// Register Invader's custom decorator instancers.
	Rocket::Core::DecoratorInstancer* decorator_instancer = new DecoratorInstancerStarfield();
	Rocket::Core::Factory::RegisterDecoratorInstancer("starfield", decorator_instancer);
	decorator_instancer->RemoveReference();

	decorator_instancer = new DecoratorInstancerDefender();
	Rocket::Core::Factory::RegisterDecoratorInstancer("defender", decorator_instancer);
	decorator_instancer->RemoveReference();	

	// Construct the game singletons.
	HighScores::Initialise();

	// Fire off the startup script.
	PythonInterface::Import("autoexec");

	Shell::EventLoop(GameLoop);	

	// Shutdown the Rocket contexts.	
	context->RemoveReference();
	
	// Shutdown Python before we shut down Rocket.
	PythonInterface::Shutdown();

	// Shut down the game singletons.
	HighScores::Shutdown();

	// Shutdown Rocket.
	Rocket::Core::Shutdown();

	Shell::CloseWindow();
	Shell::Shutdown();

	return 0;
}

#ifdef ROCKET_PLATFORM_WIN32

#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <process.h>

void DoAllocConsole()
{
	static const WORD MAX_CONSOLE_LINES = 500;
	int hConHandle;
	long lStdHandle;
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	FILE *fp;

	// allocate a console for this app
	AllocConsole();

	// set the screen buffer to be big enough to let us scroll text
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
	coninfo.dwSize.Y = MAX_CONSOLE_LINES;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

	// redirect unbuffered STDOUT to the console
	lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "w" );

	*stdout = *fp;
	setvbuf( stdout, NULL, _IONBF, 0 );

	// redirect unbuffered STDIN to the console
	lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "r" );

	*stdin = *fp;
	setvbuf( stdin, NULL, _IONBF, 0 );

	// redirect unbuffered STDERR to the console
	lStdHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "w" );
	*stderr = *fp;

	setvbuf( stderr, NULL, _IONBF, 0 );
	ShowWindow(GetConsoleWindow(), SW_SHOW);
}
#endif
