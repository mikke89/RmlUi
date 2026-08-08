#include "../Common/TestsShell.h"
#include <RmlUi/Core/ComputedValues.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <doctest.h>
#include <nanobench.h>

using namespace ankerl;
using namespace Rml;

namespace {
void ConfigureBench(nanobench::Bench& bench, const char* title)
{
	bench.title(title);
	bench.timeUnit(std::chrono::nanoseconds(1), "ns");
	bench.minEpochIterations(1000);
}
} // namespace

TEST_CASE("calculation")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	ElementDocument* document = context->CreateDocument();
	REQUIRE(document);

	nanobench::Bench parse_bench;
	ConfigureBench(parse_bench, "Calculation parsing");
	parse_bench.run("calc/ordinary/number", [&] { nanobench::doNotOptimizeAway(document->SetProperty("opacity", "0.625")); });
	parse_bench.run("calc/ordinary/length", [&] { nanobench::doNotOptimizeAway(document->SetProperty("border-left-width", "13px")); });
	parse_bench.run("calc/ordinary/length-percentage", [&] { nanobench::doNotOptimizeAway(document->SetProperty("width", "37%")); });
	parse_bench.run("calc/ordinary/declaration", [&] { nanobench::doNotOptimizeAway(document->SetProperty("margin", "3px 7% 11px 13%")); });

#ifdef RMLUI_MATH_EXPRESSIONS
	parse_bench.run("calc/math/definite", [&] { nanobench::doNotOptimizeAway(document->SetProperty("width", "calc(10px + 3px)")); });
	parse_bench.run("calc/math/affine", [&] { nanobench::doNotOptimizeAway(document->SetProperty("width", "calc(37% - 13px)")); });
	parse_bench.run("calc/math/tree",
		[&] { nanobench::doNotOptimizeAway(document->SetProperty("width", "max(calc(20% + 3px), min(240px, calc(70% - 11px)))")); });
#endif

	REQUIRE(document->SetProperty("width", "131px"));
	document->UpdateDocument();
	const auto ordinary_fixed = document->GetComputedValues().width();
	REQUIRE(document->SetProperty("width", "37%"));
	document->UpdateDocument();
	const auto ordinary_percent = document->GetComputedValues().width();
#ifdef RMLUI_MATH_EXPRESSIONS
	REQUIRE(ordinary_fixed.type != Style::LengthPercentageAuto::Calculation);
	REQUIRE(ordinary_percent.type != Style::LengthPercentageAuto::Calculation);
#endif

	nanobench::Bench layout_bench;
	ConfigureBench(layout_bench, "Calculation used values");
	layout_bench.run("calc/layout/ordinary-fixed", [&] { nanobench::doNotOptimizeAway(ResolveValue(ordinary_fixed, 640.f)); });
	layout_bench.run("calc/layout/ordinary-percentage", [&] { nanobench::doNotOptimizeAway(ResolveValue(ordinary_percent, 640.f)); });

#ifdef RMLUI_MATH_EXPRESSIONS
	REQUIRE(document->SetProperty("width", "calc(37% - 13px)"));
	document->UpdateDocument();
	const auto affine = document->GetComputedValues().width();
	REQUIRE(document->SetProperty("width", "max(calc(20% + 3px), min(240px, calc(70% - 11px)))"));
	document->UpdateDocument();
	const auto tree = document->GetComputedValues().width();
	REQUIRE(affine.type == Style::LengthPercentageAuto::Calculation);
	REQUIRE(tree.type == Style::LengthPercentageAuto::Calculation);
	REQUIRE(bool(affine.calculation));
	REQUIRE(bool(tree.calculation));
	const Calculation* affine_identity = affine.calculation.get();
	const Calculation* tree_identity = tree.calculation.get();

	layout_bench.run("calc/layout/affine", [&] { nanobench::doNotOptimizeAway(ResolveValue(affine, 640.f)); });
	layout_bench.run("calc/layout/tree", [&] { nanobench::doNotOptimizeAway(ResolveValue(tree, 640.f)); });

	// Repeated used-value resolution operates on the already-parsed immutable residual. Keeping the
	// same owner identity here catches accidental reparse/replacement in this benchmarked hot path.
	for (int i = 0; i < 100; ++i)
	{
		nanobench::doNotOptimizeAway(ResolveValue(affine, 320.f + float(i)));
		nanobench::doNotOptimizeAway(ResolveValue(tree, 320.f + float(i)));
	}
	CHECK(affine.calculation.get() == affine_identity);
	CHECK(tree.calculation.get() == tree_identity);
#endif

	document->Close();
	TestsShell::ShutdownShell();
}
