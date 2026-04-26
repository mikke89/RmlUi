#include "../Common/TestsInterface.h"
#include <RmlUi/Core/ComputedValues.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/FontEngineInterface.h>
#include <doctest.h>
#include <nanobench.h>

/*
    Microbenchmarks for RCSS variables. Compares non-var property compute (baseline), var()-deferred
    property compute, runtime SetVariable + Update (theme-toggle), and the no-event steady-state cost.
*/

using namespace ankerl;
using namespace Rml;

namespace {

constexpr int kNumChildren = 100;

String BuildBodyWithoutVars()
{
	String s;
	s.reserve(kNumChildren * 80);
	for (int i = 0; i < kNumChildren; ++i)
		s += "<div style=\"background-color: red; color: blue; padding: 10px;\"/>";
	return s;
}

String BuildBodyWithVars()
{
	String s;
	s.reserve(kNumChildren * 90);
	for (int i = 0; i < kNumChildren; ++i)
		s += "<div style=\"background-color: var(--bg); color: var(--fg); padding: var(--pad);\"/>";
	return s;
}

const String& BodyWithoutVars()
{
	static const String s = BuildBodyWithoutVars();
	return s;
}
const String& BodyWithVars()
{
	static const String s = BuildBodyWithVars();
	return s;
}

} // namespace

TEST_CASE("Variables benchmark")
{
	TestsSystemInterface system_interface;
	TestsRenderInterface render_interface;
	FontEngineInterface font_interface;
	SetRenderInterface(&render_interface);
	SetSystemInterface(&system_interface);
	SetFontEngineInterface(&font_interface);

	Rml::Initialise();

	Context* context = Rml::CreateContext("bench", Vector2i(1024, 768));
	REQUIRE(context != nullptr);
	ElementDocument* document = context->CreateDocument();
	REQUIRE(document != nullptr);

	context->SetVariable("--bg", "red");
	context->SetVariable("--fg", "blue");
	context->SetVariable("--pad", "10px");

	nanobench::Bench bench;
	bench.title("Context variables: compute and runtime cost");
	bench.timeUnit(std::chrono::microseconds(1), "us");
	bench.relative(true);
	bench.minEpochIterations(20);

	// Baseline: SetInnerRML with concrete property values; no var() in the property pipeline.
	bench.run("no-vars: SetInnerRML + Update (100 divs)", [&] {
		document->SetInnerRML(BodyWithoutVars());
		context->Update();
	});

	// Feature path: same shape but every property uses var(), forcing the deferred-resolution path.
	bench.run("with-vars: SetInnerRML + Update (100 divs)", [&] {
		document->SetInnerRML(BodyWithVars());
		context->Update();
	});

	// Pre-load var-using DOM, then measure the cost of changing one variable and re-Update.
	document->SetInnerRML(BodyWithVars());
	context->Update();

	int toggle = 0;
	bench.run("theme-toggle: SetVariable(--bg) + Update", [&] {
		context->SetVariable("--bg", (toggle++ & 1) ? "red" : "blue");
		context->Update();
	});

	// Per-frame steady-state cost: ComputeValues short-circuits when no properties are dirty.
	bench.run("steady-state: Update with var-using DOM, no changes", [&] {
		context->Update();
	});

	Rml::Shutdown();
}
