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
#include "../Common/TestsInterface.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Types.h>

#include <doctest.h>
#include <nanobench.h>

using namespace ankerl;
using namespace Rml;

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


TEST_CASE("Elements (shell)")
{
	Context* context = TestsShell::GetMainContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocument("basic/benchmark/data/benchmark.rml");
	REQUIRE(document);
	document->Show();

	Element* el = document->GetElementById("performance");
	REQUIRE(el);

	nanobench::Bench bench;
	bench.title("Elements (shell)");
	bench.relative(true);

	constexpr int num_rows = 50;
	const String rml = GenerateRml(num_rows);

	el->SetInnerRML(rml);
	context->Update();
	context->Render();

	bench.run("Update (unmodified)", [&] {
		context->Update();
	});

	bench.run("Render", [&] {
		TestsShell::PrepareRenderBuffer();
		context->Render();
		TestsShell::PresentRenderBuffer();
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
		TestsShell::PrepareRenderBuffer();
		context->Render();
		TestsShell::PresentRenderBuffer();
	});

	document->Close();
}


TEST_CASE("Elements (dummy interface)")
{
	TestsRenderInterface render_interface;
	Context* context = TestsShell::CreateContext("element_dummy", &render_interface);
	REQUIRE(context);

	ElementDocument* document = context->LoadDocument("basic/benchmark/data/benchmark.rml");
	REQUIRE(document);
	document->Show();

	Element* el = document->GetElementById("performance");
	REQUIRE(el);

	nanobench::Bench bench;
	bench.title("Elements (dummy interface)");
	bench.relative(true);

	constexpr int num_rows = 50;
	const String rml = GenerateRml(num_rows);

	el->SetInnerRML(rml);
	context->Update();
	context->Render();

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

	render_interface.ResetCounters();
	context->Render();
	auto& counters = render_interface.GetCounters();

	const String msg = CreateString(256,
		"Stats for single Context::Render() with n=%d rows: \n"
		"Render calls: %zu\n"
		"Scissor enable: %zu\n"
		"Scissor set: %zu\n"
		"Texture load: %zu\n"
		"Texture generate: %zu\n"
		"Texture release: %zu\n"
		"Transform set: %zu\n",
		num_rows,
		counters.render_calls,
		counters.enable_scissor,
		counters.set_scissor,
		counters.load_texture,
		counters.generate_texture,
		counters.release_texture,
		counters.set_transform
	);
	MESSAGE(msg);

	document->Close();
	TestsShell::RemoveContext(context);
}


TEST_CASE("Elements asymptotic complexity (dummy interface)")
{
	TestsRenderInterface render_interface;
	Context* context = TestsShell::CreateContext("element_complexity", &render_interface);
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

#ifdef RMLUI_BENCHMARKS_SHOW_COMPLEXITY
		MESSAGE(bench.complexityBigO());
#endif
	}

	TestsShell::RemoveContext(context);
}