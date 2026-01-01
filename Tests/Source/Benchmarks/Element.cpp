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

static const char* DefaultRow = R"(
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
			</div>)";

static const char* LongTextRow = R"(
			<div class="row">
				<div class="col col1"><button class="expand" index="%d">+</button>&nbsp;<a>Ut pulvinar urna nulla. Donec sed sollicitudin diam. Donec eu mauris massa. Suspendisse facilisis mollis dictum. Curabitur mollis nisi eu est semper, quis ultrices augue facilisis. Quisque venenatis malesuada leo, quis dictum turpis tristique at. Integer ut nunc nec odio imperdiet dignissim. %d</a></div>
				<div class="col col23"><input type="range" class="assign_range" min="0" max="%d" value="%d"/></div>
				<div class="col col4">Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nullam eros neque, blandit aliquam consectetur vitae, ornare ac magna. Nam purus nulla, vestibulum a mi vitae, vestibulum porta dolor. Interdum et malesuada fames ac ante ipsum primis in faucibus. Morbi euismod placerat libero, vel elementum purus blandit a. Aenean sed arcu dictum, pharetra diam tempor, tristique est. Vestibulum sagittis leo nec purus consectetur imperdiet. Aenean dictum, neque vitae consequat egestas, mi nibh rhoncus sapien, eu scelerisque eros arcu non lorem. Suspendisse eu pellentesque velit, non sagittis eros. Maecenas tellus odio, condimentum vitae volutpat at, varius eget leo. Maecenas dignissim sem a ligula fermentum</div>
				<select>
					<option>Red</option><option>Blue</option><option selected>Green</option><option style="background-color: yellow;">Yellow</option>
				</select>
				<div class="inrow unmark_collapse">
					<div class="col col123 assign_text">Quisque rhoncus ante arcu, at dapibus nulla mattis et. Fusce ac lacinia urna. Nulla facilisi. Morbi consequat ligula eget urna congue pellentesque. Nullam a risus mattis lectus rutrum rutrum. Etiam pharetra libero vitae nibh lobortis vestibulum. Fusce malesuada ligula sem, vitae bibendum mi sodales ac. Fusce mollis nunc non urna hendrerit viverra. Praesent ornare nunc dictum turpis suscipit, in lacinia risus malesuada. Sed sollicitudin purus eget sapien elementum venenatis.</div>
					<div class="col col4">
						<input type="submit" class="vehicle_depot_assign_confirm" quantity="0">Confirm</input>
					</div>
				</div>
			</div>)";

static String GenerateRml(const int num_rows, const char* row)
{
	static nanobench::Rng rng;

	Rml::String rml;
	rml.reserve(10000 * num_rows);

	for (int i = 0; i < num_rows; i++)
	{
		int index = rng() % 1000;
		int route = rng() % 50;
		int max = (rng() % 40) + 10;
		int value = rng() % max;
		Rml::String rml_row = Rml::CreateString(row, index, route, max, value);
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
	const String rml = GenerateRml(num_rows, DefaultRow);

	el->SetInnerRML(rml);
	context->Update();
	context->Render();
	TestsShell::RenderLoop();

	String msg = Rml::CreateString("\nElement construction and destruction of %d total elements.\n", GetNumDescendentElements(el));
	msg += TestsShell::GetRenderStats();
	MESSAGE(msg);

	nanobench::Bench bench;
	bench.title("Element");
	bench.timeUnit(std::chrono::microseconds(1), "us");
	bench.relative(true);

	bench.run("Update (unmodified)", [&] { context->Update(); });

	bool hover_toggle = true;
	auto child = el->GetChild(num_rows / 2);

	bench.run("Update (hover child)", [&] {
		static nanobench::Rng rng;
		child->SetPseudoClass(":hover", hover_toggle);
		hover_toggle = !hover_toggle;
		context->Update();
	});
	bench.run("Update (hover)", [&] {
		el->SetPseudoClass(":hover", hover_toggle);
		hover_toggle = !hover_toggle;
		context->Update();
	});

	bench.run("Render", [&] { context->Render(); });

	bench.run("SetInnerRML", [&] { el->SetInnerRML(rml); });

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

TEST_CASE("element.long_texts")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	REQUIRE(document);
	document->Show();

	Element* el = document->GetElementById("performance");
	REQUIRE(el);
	constexpr int num_rows = 50;
	const String rml = GenerateRml(num_rows, LongTextRow);

	el->SetInnerRML(rml);
	context->Update();
	context->Render();
	TestsShell::RenderLoop();

	String msg = Rml::CreateString("\nElement construction and destruction of %d total very long elements.\n", GetNumDescendentElements(el));
	msg += TestsShell::GetRenderStats();
	MESSAGE(msg);

	nanobench::Bench bench;
	bench.title("Element");
	bench.timeUnit(std::chrono::microseconds(1), "us");
	bench.relative(true);

	bench.run("Update (unmodified)", [&] { context->Update(); });

	bool hover_toggle = true;
	auto child = el->GetChild(num_rows / 2);

	bench.run("Update (hover child)", [&] {
		static nanobench::Rng rng;
		child->SetPseudoClass(":hover", hover_toggle);
		hover_toggle = !hover_toggle;
		context->Update();
	});
	bench.run("Update (hover)", [&] {
		el->SetPseudoClass(":hover", hover_toggle);
		hover_toggle = !hover_toggle;
		context->Update();
	});

	bench.run("Render", [&] { context->Render(); });

	bench.run("SetInnerRML", [&] { el->SetInnerRML(rml); });

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
		{"SetInnerRML", [&](const String& rml) { el->SetInnerRML(rml); }},
		{"Update (unmodified)", [&](const String& /*rml*/) { context->Update(); }},
		{"Render", [&](const String& /*rml*/) { context->Render(); }},
		{"SetInnerRML + Update",
			[&](const String& rml) {
				el->SetInnerRML(rml);
				context->Update();
			}},
		{"SetInnerRML + Update + Render",
			[&](const String& rml) {
				el->SetInnerRML(rml);
				context->Update();
				context->Render();
			}},
	};

	for (auto& bench_def : bench_list)
	{
		nanobench::Bench bench;
		bench.title(bench_def.title);
		bench.timeUnit(std::chrono::microseconds(1), "us");
		bench.relative(true);

		// Running the benchmark multiple times, with different number of rows.
		for (const int num_rows : {1, 2, 5, 10, 20, 50, 100, 200, 500})
		{
			const String rml = GenerateRml(num_rows, DefaultRow);

			el->SetInnerRML(rml);
			context->Update();
			context->Render();

			bench.complexityN(num_rows).run(bench_def.title, [&]() { bench_def.run(rml); });
		}

#if defined(RMLUI_BENCHMARKS_SHOW_COMPLEXITY) || 0
		MESSAGE(bench.complexityBigO());
#endif
	}

	document->Close();
}
