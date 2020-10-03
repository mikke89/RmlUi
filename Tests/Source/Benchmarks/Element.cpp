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

#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
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
			max-width: 2000px;
			max-height: 2000px;
			left: 100px;
			top: 50px;
			width: 1300px;
			height: 600px;
		}
		#performance 
		{
			width: 800px;
			height: 300px;
		}
	</style>
</head>

<body template="window">
<div id="performance"/>
</body>
</rml>
)";


static int GetNumDescendentElements(Element* element)
{
	const int num_children = element->GetNumChildren(true);
	int result = num_children;
	for (int i = 0; i < num_children; i++)
	{
		result += GetNumDescendentElements(element->GetChild(i));
	}
	return result;
}


static String GenerateRml(const int num_rows)
{
	static nanobench::Rng rng;

	Rml::String rml;
	rml.reserve(1000 * num_rows);

	for (int i = 0; i < num_rows; i++)
	{
		int index = rng() % 1000;
		int route = rng() % 50;
		int max = (rng() % 40) + 10;
		int value = rng() % max;
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

	return rml;
}


TEST_CASE("element.creation_and_destruction")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	REQUIRE(document);
	document->Show();

	Element* el = document->GetElementById("performance");
	REQUIRE(el);
	constexpr int num_rows = 50;
	const String rml = GenerateRml(num_rows);

	el->SetInnerRML(rml);
	context->Update();
	context->Render();
	TestsShell::RenderLoop();

	String msg = Rml::CreateString(128, "\nElement construction and destruction of %d total elements.\n", GetNumDescendentElements(el));
	msg += TestsShell::GetRenderStats();
	MESSAGE(msg);

	nanobench::Bench bench;
	bench.title("Element");
	bench.timeUnit(std::chrono::microseconds(1), "us");
	bench.relative(true);

	bench.run("Update (unmodified)", [&] {
		context->Update();
	});

	bench.run("Render", [&] {
		context->Render();
	});

	bench.run("SetInnerRML", [&] {
		el->SetInnerRML(rml);
	});

	bench.run("SetInnerRML + Update", [&] {
		el->SetInnerRML(rml);
		context->Update();
	});

	bench.run("SetInnerRML + Update + Render", [&] {
		el->SetInnerRML(rml);
		context->Update();
		context->Render();
	});

	document->Close();
}


TEST_CASE("element.asymptotic_complexity")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocument("basic/benchmark/data/benchmark.rml");
	REQUIRE(document);
	document->Show();

	Element* el = document->GetElementById("performance");
	REQUIRE(el);


	struct BenchDef {
		const char* title;
		Function<void(const String& rml)> run;
	};

	Vector<BenchDef> bench_list = {
		{
			"SetInnerRML",
			[&](const String& rml) {
				el->SetInnerRML(rml);
			}
		},
		{
			"Update (unmodified)",
			[&](const String& /*rml*/) {
				context->Update();
			}
		},
		{
			"Render",
			[&](const String& /*rml*/) {
				context->Render();
			}
		},
		{
			"SetInnerRML + Update",
			[&](const String& rml) {
				el->SetInnerRML(rml);
				context->Update();
			}
		},
		{
			"SetInnerRML + Update + Render",
			[&](const String& rml) {
				el->SetInnerRML(rml);
				context->Update();
				context->Render();
			}
		},
	};

	for (auto& bench_def : bench_list)
	{
		nanobench::Bench bench;
		bench.title(bench_def.title);
		bench.timeUnit(std::chrono::microseconds(1), "us");
		bench.relative(true);

		// Running the benchmark multiple times, with different number of rows.
		for (const int num_rows : { 1, 2, 5, 10, 20, 50, 100, 200, 500 })
		{
			const String rml = GenerateRml(num_rows);

			el->SetInnerRML(rml);
			context->Update();
			context->Render();

			bench.complexityN(num_rows).run(bench_def.title, [&]() {
				bench_def.run(rml);
			});
		}

#if defined(RMLUI_BENCHMARKS_SHOW_COMPLEXITY) || 0
		MESSAGE(bench.complexityBigO());
#endif
	}

	document->Close();
}