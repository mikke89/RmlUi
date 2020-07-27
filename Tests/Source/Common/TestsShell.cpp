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

#include "TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Debugger.h>
#include <Shell.h>
#include <Input.h>
#include <ShellRenderInterfaceOpenGL.h>

#include <doctest.h>

namespace {
	bool shell_initialized = false;

	ShellRenderInterfaceOpenGL shell_render_interface;
	ShellSystemInterface shell_system_interface;

	Rml::Context* shell_context = nullptr;

	const Rml::Vector2i window_size(1500, 800);
}

static void InitializeShell()
{
	if (!shell_initialized)
	{
		Rml::SetSystemInterface(&shell_system_interface);
		Rml::SetRenderInterface(&shell_render_interface);

		shell_initialized = true;

		// Generic OS initialisation, creates a window and attaches OpenGL.
		REQUIRE(Shell::Initialise());
		REQUIRE(Shell::OpenWindow("Element benchmark", &shell_render_interface, window_size.x, window_size.y, true));

		// RmlUi initialisation.
		shell_render_interface.SetViewport(window_size.x, window_size.y);

		REQUIRE(Rml::Initialise());

		REQUIRE(!shell_context);
		shell_context = Rml::CreateContext("main", window_size);
		REQUIRE(shell_context);

		REQUIRE(Rml::Debugger::Initialise(shell_context));
		::Input::SetContext(shell_context);
		shell_render_interface.SetContext(shell_context);

		Shell::LoadFonts("assets/");
	}
}


Rml::Context* TestsShell::GetMainContext()
{
	InitializeShell();
	return shell_context;
}


Rml::Context* TestsShell::CreateContext(const Rml::String& name, Rml::RenderInterface* render_interface)
{
	InitializeShell();

	if (!render_interface)
		render_interface = &shell_render_interface;

	Rml::Context* context = Rml::CreateContext(name, window_size, render_interface);
	REQUIRE(context);

	return context;
}


void TestsShell::RemoveContext(Rml::Context* context)
{
	REQUIRE(context);
	REQUIRE(context != shell_context);

	const Rml::String& name = context->GetName();
	REQUIRE(name != "main");

	REQUIRE(Rml::RemoveContext(name));
}

void TestsShell::EventLoop(ShellIdleFunction idle_func)
{
	Shell::EventLoop(idle_func);
}

void TestsShell::PrepareRenderBuffer()
{
	shell_render_interface.PrepareRenderBuffer();
}

void TestsShell::PresentRenderBuffer()
{
	shell_render_interface.PresentRenderBuffer();
}

void TestsShell::RequestExit()
{
	Shell::RequestExit();
}

void TestsShell::ShutdownShell()
{
	if (shell_initialized)
	{
		Rml::Shutdown();
		Shell::CloseWindow();
		Shell::Shutdown();

		shell_context = nullptr;
		shell_initialized = false;
	}
}
