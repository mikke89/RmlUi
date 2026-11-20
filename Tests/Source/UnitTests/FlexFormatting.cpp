#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <doctest.h>

using namespace Rml;

static const String document_flex_rml = R"(
<rml>
<head>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body {
			width: 500px;
			height: 300px;
			background: #333;
		}
		#flex {
			background: #666;
			display: flex;
			width: 50%;
			height: 30%;
		}
		input.checkbox {
			background: #fff;
			border: 1px #000;
		}
	</style>
</head>

<body>
	<div id="flex">
		<input type="checkbox" id="checkbox"/>
	</div>
</body>
</rml>
)";

TEST_CASE("FlexFormatting")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_flex_rml);
	REQUIRE(document);
	document->Show();

	Element* checkbox = document->GetElementById("checkbox");
	Element* flex = document->GetElementById("flex");

	struct TestCase {
		String flex_direction;
		String align_items;
		Vector2f expected_size;
	};
	TestCase test_cases[] = {
		{"column", "stretch", Vector2f(248, 16)},
		{"column", "center", Vector2f(16, 16)},
		{"row", "stretch", Vector2f(16, 88)},
		{"row", "center", Vector2f(16, 16)},
	};

	for (auto& test_case : test_cases)
	{
		flex->SetProperty("flex-direction", test_case.flex_direction);
		flex->SetProperty("align-items", test_case.align_items);

		TestsShell::RenderLoop();

		CAPTURE(test_case.align_items);
		CAPTURE(test_case.flex_direction);
		CHECK(checkbox->GetBox().GetSize().x == doctest::Approx(test_case.expected_size.x));
		CHECK(checkbox->GetBox().GetSize().y == doctest::Approx(test_case.expected_size.y));
	}

	document->Close();

	TestsShell::ShutdownShell();
}

static const String document_flex_dp_ratio_rml = R"(
<rml>
<head>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body {
			width: 100%;
			height: 100%;
		}
		.window {
			width: 1920dp;
			height: 1080dp;
			margin: 0 auto;
			display: flex;
			flex-direction: column;
		}
		.inner {
			flex-grow: 1;
			display: flex;
			flex-direction: row;
		}
		.left {
			width: 30%;
			margin: 32dp;
			overflow: auto;
			background: #522;
		}
		.right {
			flex: 1;
			margin: 32dp;
			overflow: auto;
			background: #252;
		}
	</style>
</head>

<body>
	<div class="window">
		<div class="inner">
			<div class="left"/>
			<div class="right"/>
		</div>
	</div>
</body>
</rml>
)";

TEST_CASE("FlexFormatting.dp_ratio")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_flex_dp_ratio_rml);
	REQUIRE(document);
	document->Show();

	Element* window = document->GetChild(0);

	constexpr float native_width = 1920.f;
	constexpr float native_height = 1080.f;

	const float test_window_widths[] = {
		3440.f,
		2960.f,
		2880.f,
		2560.f,
		2400.f,
		2048.f,
		1921.f,
		1920.f,
		1919.f,
		1600.f,
		1366.f,
		1280.f,
		1024.f,
	};

	for (float window_width : test_window_widths)
	{
		const float dp_ratio = window_width / native_width;
		CAPTURE(window_width);
		CAPTURE(dp_ratio);

		context->SetDensityIndependentPixelRatio(dp_ratio);

		TestsShell::RenderLoop();

		CHECK(window->GetBox().GetSize().x == window_width);
		CHECK(window->GetBox().GetSize().y == doctest::Approx((window_width / native_width) * native_height));
	}

	document->Close();

	TestsShell::ShutdownShell();
}

static const String document_flex_absolute_rml = R"(
<rml>
<head>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body {
			width: 500px;
			height: 500px;
		}
		.band {
			display: flex;
			position: relative;
			min-height: 40px;
			padding: 5px 0;
		}
		.column {
			flex-direction: column;
			min-height: 0;
		}
		.floating {
			position: absolute;
			top: 0;
			right: 0;
		}
		.floated {
			float: right;
		}
		scrollbarvertical {
			width: 10px;
		}
		.scroll {
			flex-direction: column;
			min-height: 0;
			width: 100px;
			overflow: auto;
		}
		.scroll .tall {
			flex-shrink: 0;
		}
		.limited {
			max-height: 50px;
		}
		.fixed {
			height: 50px;
		}
		.short {
			width: 20px;
			height: 10px;
		}
		.tall {
			width: 20px;
			height: 60px;
		}
		.art {
			position: absolute;
			top: 0;
			bottom: 0;
			left: 0;
			width: 10px;
		}
		.half {
			position: absolute;
			top: 0;
			left: 0;
			width: 10px;
			height: 50%;
		}
		.bottom_half {
			position: absolute;
			bottom: 0;
			left: 0;
			width: 10px;
			height: 50%;
		}
		.centred {
			position: absolute;
			top: 0;
			bottom: 0;
			left: 0;
			width: 10px;
			height: 20px;
			margin: auto 0;
		}
	</style>
</head>

<body>
	<div class="band" id="min_height">
		<div class="short"/>
		<div class="art" id="min_height_art"/>
	</div>
	<div class="band" id="grown">
		<div class="tall"/>
		<div class="art" id="grown_art"/>
		<div class="half" id="grown_half"/>
		<div class="bottom_half" id="grown_bottom_half"/>
		<div class="centred" id="grown_centred"/>
	</div>
	<div class="band column" id="column">
		<div class="tall"/>
		<div class="tall"/>
		<div class="art" id="column_art"/>
	</div>
	<div class="band" id="nested">
		<div class="short"><div class="art" id="nested_art"/></div>
	</div>
	<div class="band floating" id="floating">
		<div class="tall"/>
		<div class="art" id="floating_art"/>
	</div>
	<div class="band floated" id="floated">
		<div class="tall"/>
		<div class="art" id="floated_art"/>
	</div>
	<div class="band scroll limited" id="limited">
		<div class="tall"/>
		<div class="tall"/>
		<div class="art" id="limited_art"/>
	</div>
	<div class="band scroll fixed" id="fixed">
		<div class="tall"/>
		<div class="tall"/>
		<div class="art" id="fixed_art"/>
	</div>
</body>
</rml>
)";

TEST_CASE("FlexFormatting.absolute_child_of_auto_height_container")
{
	// Absolutely positioned children of an auto-height flex container must be sized against the container's final
	// height, not the indefinite height it had before its flex items were laid out.
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_flex_absolute_rml);
	REQUIRE(document);
	document->Show();

	TestsShell::RenderLoop();

	struct TestCase {
		String container_id;
		String child_id;
		float container_height;
		float child_height;
		float child_offset; // Top of the child's border box, relative to the container's border box.
	};
	const TestCase test_cases[] = {
		// A 10px flex item leaves the band at its min-height: 40 + 2 * 5 padding.
		{"min_height", "min_height_art", 40.f, 50.f, 0.f},
		// A 60px flex item grows the band past its min-height.
		{"grown", "grown_art", 60.f, 70.f, 0.f},
		{"grown", "grown_half", 60.f, 35.f, 0.f},
		// Placed from the bottom, so the position depends on the size: 70 - 35.
		{"grown", "grown_bottom_half", 60.f, 35.f, 35.f},
		// Auto margins centre a 20px child between top: 0 and bottom: 0: (70 - 20) / 2.
		{"grown", "grown_centred", 60.f, 20.f, 25.f},
		// Column direction: two 60px items stacked.
		{"column", "column_art", 120.f, 130.f, 0.f},
		// Inside a static flex item: the band is still the containing block.
		{"nested", "nested_art", 40.f, 50.f, 0.f},
		// Absolutely positioned and floated flex containers, which get a shrink-to-fit width.
		{"floating", "floating_art", 60.f, 70.f, 0.f},
		{"floated", "floated_art", 60.f, 70.f, 0.f},
		// Scroll containers formatted again after enabling a scrollbar: auto height limited by max-height, and a fixed
		// height whose first Close() fails. The box stored for the child is the one from the accepted pass.
		{"limited", "limited_art", 50.f, 60.f, 0.f},
		{"fixed", "fixed_art", 50.f, 60.f, 0.f},
	};

	for (const TestCase& test_case : test_cases)
	{
		CAPTURE(test_case.child_id);

		Element* container = document->GetElementById(test_case.container_id);
		Element* child = document->GetElementById(test_case.child_id);
		// CHECK rather than REQUIRE, so that a missing element still lets the test close its document.
		CHECK(container);
		CHECK(child);
		if (!container || !child)
			continue;

		CHECK(container->GetBox().GetSize().y == doctest::Approx(test_case.container_height));
		CHECK(child->GetBox().GetSize().y == doctest::Approx(test_case.child_height));
		CHECK(child->GetRelativeOffset().y == doctest::Approx(test_case.child_offset));
	}

	// The scroll containers did enable their vertical scrollbar, so they went through the second formatting pass.
	for (const String id : {"limited", "fixed"})
	{
		CAPTURE(id);
		Element* scroll_container = document->GetElementById(id);
		CHECK(scroll_container);
		if (scroll_container)
			CHECK(scroll_container->GetClientWidth() == doctest::Approx(90.f));
	}

	document->Close();

	TestsShell::ShutdownShell();
}
