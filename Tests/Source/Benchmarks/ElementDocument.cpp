/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
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

#include "../Common/TestsInterface.h"
#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Factory.h>
#include <RmlUi/Core/Types.h>
#include <doctest.h>
#include <nanobench.h>

using namespace ankerl;
using namespace Rml;

static const String document_rml = R"(
<rml>
<head>
	<link type="text/template" href="/assets/window.rml"/>
	<title>Benchmark Sample</title>
	<style>
		body.window
		{
			left: 50px;
			top: 50px;
			width: 800px;
			height: 200px;
		}
		#performance 
		{
			width: 500px;
			height: 300px;
		}
	</style>
</head>

<body template="window">
<div id="performance">
	<div class="row">
		<div class="col col1"><button class="expand" index="3">+</button>&nbsp;<a>Route 15</a></div>
		<div class="col col23"><input type="range" class="assign_range" min="0" max="20" value="3"/></div>
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
	</div>
</div>
</body>
</rml>
)";

static const String variables_rml = R"(
<rml>
<head>
	<link type="text/template" href="/assets/window.rml"/>
	<title>Benchmark Sample</title>
	<style>
		body {
			--perf-width: 500px;
			--perf-height: 300px;
			--window-left: 50px;
			--window-top: 50px;
			--window-width: 800px;
			--window-height: 200px;
		}
		body.window
		{
			left: var(--window-left);
			top: var(--window-top);
			width: var(--window-width);
			height: var(--window-height);
		}
		#performance 
		{
			width: var(--perf-width);
			height: var(--perf-height);
		}
	</style>
</head>

<body template="window">
<div id="performance">
	<div class="row">
		<div class="col col1"><button class="expand" index="3">+</button>&nbsp;<a>Route 15</a></div>
		<div class="col col23"><input type="range" class="assign_range" min="0" max="20" value="3"/></div>
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
	</div>
</div>
</body>
</rml>
)";


void benchmark(String const& doc)
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	{
		ElementDocument* document = context->LoadDocumentFromMemory(doc);
		document->Show();

		const String stats = TestsShell::GetRenderStats();
		MESSAGE(stats);

		TestsShell::RenderLoop();
		document->Close();
		context->Update();
	}

	{
		nanobench::Bench bench;
		bench.title("ElementDocument");
		bench.timeUnit(std::chrono::microseconds(1), "us");
		bench.relative(true);

		bench.run("LoadDocument", [&] {
			ElementDocument* document = context->LoadDocumentFromMemory(doc);
			document->Close();
			context->Update();
		});

		bench.run("LoadDocument + Show", [&] {
			ElementDocument* document = context->LoadDocumentFromMemory(doc);
			document->Show();
			document->Close();
			context->Update();
		});

		bench.run("LoadDocument + Show + Update", [&] {
			ElementDocument* document = context->LoadDocumentFromMemory(doc);
			document->Show();
			context->Update();
			document->Close();
			context->Update();
		});

		bench.run("LoadDocument + Show + Update + Render", [&] {
			ElementDocument* document = context->LoadDocumentFromMemory(doc);
			document->Show();
			context->Update();
			TestsShell::BeginFrame();
			context->Render();
			TestsShell::PresentFrame();
			document->Close();
			context->Update();
		});
	}

	{
		nanobench::Bench bench;
		bench.title("ElementDocument w/ClearStyleSheetCache");
		bench.timeUnit(std::chrono::microseconds(1), "us");
		bench.relative(true);

		bench.run("Clear + LoadDocument", [&] {
			Factory::ClearStyleSheetCache();
			ElementDocument* document = context->LoadDocumentFromMemory(doc);
			document->Close();
			context->Update();
		});

		bench.run("Clear + LoadDocument + Show", [&] {
			Factory::ClearStyleSheetCache();
			ElementDocument* document = context->LoadDocumentFromMemory(doc);
			document->Show();
			document->Close();
			context->Update();
		});

		bench.run("Clear + LoadDocument + Show + Update", [&] {
			Factory::ClearStyleSheetCache();
			ElementDocument* document = context->LoadDocumentFromMemory(doc);
			document->Show();
			context->Update();
			document->Close();
			context->Update();
		});

		bench.run("Clear + LoadDocument + Show + Update + Render", [&] {
			Factory::ClearStyleSheetCache();
			ElementDocument* document = context->LoadDocumentFromMemory(doc);
			document->Show();
			context->Update();
			TestsShell::BeginFrame();
			context->Render();
			TestsShell::PresentFrame();
			document->Close();
			context->Update();
		});
	}
}


TEST_CASE("elementdocument-baseline") {
	benchmark(document_rml);
}

TEST_CASE("elementdocument-variables") {
	benchmark(variables_rml);
}
