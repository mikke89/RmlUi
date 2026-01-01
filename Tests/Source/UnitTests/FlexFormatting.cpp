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
