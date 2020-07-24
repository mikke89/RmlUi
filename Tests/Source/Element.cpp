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

#ifdef RMLUI_ENABLE_BENCHMARKS

#include "TestsInterface.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Types.h>
#include <RmlUi/Debugger.h>

#include <Shell.h>
#include <Input.h>
#include <ShellRenderInterfaceOpenGL.h>

#include <doctest.h>
#include <nanobench.h>

using namespace ankerl;
using namespace Rml;

static Rml::Context* context = nullptr;
static ShellRenderInterfaceExtensions* shell_renderer;


TEST_CASE("Element benchmark")
{
	const Vector2i window_size(1024, 768);

	ShellRenderInterfaceOpenGL opengl_renderer;
	shell_renderer = &opengl_renderer;

	// Generic OS initialisation, creates a window and attaches OpenGL.
	REQUIRE(Shell::Initialise());
	REQUIRE(Shell::OpenWindow("Element benchmark", shell_renderer, window_size.x, window_size.y, true));

	// RmlUi initialisation.
	Rml::SetRenderInterface(&opengl_renderer);
	shell_renderer->SetViewport(window_size.x, window_size.y);

	ShellSystemInterface system_interface;
	Rml::SetSystemInterface(&system_interface);

	//TestsSystemInterface system_interface;
	//TestsRenderInterface render_interface;

	//SetRenderInterface(&render_interface);
	//SetSystemInterface(&system_interface);

	REQUIRE(Initialise());

	context = Rml::CreateContext("main", window_size);
	REQUIRE(context);

	Rml::Debugger::Initialise(context);
	::Input::SetContext(context);
	shell_renderer->SetContext(context);

	Shell::LoadFonts("assets/");

	ElementDocument* document = context->LoadDocument("basic/benchmark/data/benchmark.rml");
	REQUIRE(document);
	document->Show();

	Rml::String rml;
	Element* el = document->GetElementById("performance");
	REQUIRE(el);

	nanobench::Bench bench;
	bench.title("Elements")
		.run("SetInnerRML", [&] {
		for (int i = 0; i < 50; i++)
		{
			int index = rand() % 1000;
			int route = rand() % 50;
			int max = (rand() % 40) + 10;
			int value = rand() % max;
			Rml::String rml_row = Rml::CreateString(1000, R"(
				<div class="row">
					<div class="col col1"><button class="expand" index="%d">+</button>&nbsp;<a>Route %d</a></div>
					<div class="col col23"><input type="range" class="assign_range" min="0" max="%d" value="%d"/></div>
					<div class="col col4">Assigned</div>
					<select>
						<option>Red</option><option>Blue</option><option selected>Green</option><option style="background-color: yellow;">Yellow</option>
					</select>
					<div class="inrow unmark_collapse">
						<div class="col col123 assign_text">Assign to route</div>
						<div class="col col4">
							<input type="submit" class="vehicle_depot_assign_confirm" quantity="0">Confirm</input>
						</div>
					</div>
				</div>)",
				index,
				route,
				max,
				value
			);
			rml += rml_row;
		}

		el->SetInnerRML(rml);
	});

	Rml::Shutdown();

	Shell::CloseWindow();
}

#endif
