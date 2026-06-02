#include "../Common/TestsInterface.h"
#include "../Common/TestsShell.h"
#include <RmlUi/Core/ComputedValues.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <doctest.h>
#include <nanobench.h>

using namespace ankerl;
using namespace Rml;

namespace {
constexpr int kNumChildren = 100;

String BuildBodyWithoutVars()
{
	String s;
	for (int i = 0; i < kNumChildren; ++i)
		s += R"(<div style="background-color: red; color: blue; padding: 10px;"/>)";
	return s;
}

String BuildBodyWithVars()
{
	String s;
	for (int i = 0; i < kNumChildren; ++i)
		s += R"(<div style="background-color: var(--bg); color: var(--fg); padding: var(--pad);"/>)";
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

TEST_CASE("custom_properties")
{
	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->CreateDocument();

	document->SetProperty("--bg", "red");
	document->SetProperty("--fg", "blue");
	document->SetProperty("--pad", "10px");

	nanobench::Bench bench;
	bench.title("Custom properties");
	bench.timeUnit(std::chrono::microseconds(1), "us");
	bench.relative(true);
	bench.minEpochIterations(10);

	bench.run("baseline: SetInnerRML + Update", [&] {
		document->SetInnerRML(BodyWithoutVars());
		context->Update();
	});

	bench.run("with-vars: SetInnerRML + Update", [&] {
		document->SetInnerRML(BodyWithVars());
		context->Update();
	});

	document->SetInnerRML(BodyWithoutVars());
	context->Update();

	int toggle = 0;
	bench.run("theme-toggle baseline: SetProperty + Update", [&] {
		document->SetProperty("color", (toggle++ & 1) ? "red" : "blue");
		context->Update();
	});

	document->SetInnerRML(BodyWithVars());
	context->Update();

	bench.run("theme-toggle with-vars: SetProperty + Update", [&] {
		document->SetProperty("--bg", (toggle++ & 1) ? "red" : "blue");
		context->Update();
	});

	document->Close();
	TestsShell::ShutdownShell();
}
