#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <doctest.h>

using namespace Rml;

static const String document_layout_rml = R"(
<rml>
<head>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body {
			width: 500px;
			height: 300px;
			top: 100px;
			left: 100px;
			border: 10px #fff;
			background-color: #ccc;
		}
		#relative {
			width: 100%;
			height: 25%;
			background-color: red;
			position: relative;
			top: 50%;
		}
	</style>
</head>

<body>
	<div id="relative"/>
</body>
</rml>
)";

static const String document_layout_rml_nested = R"(
<rml>
<head>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body {
			width: 500px;
			height: 300px;
			top: 100px;
			left: 100px;
			border: 10px #fff;
			background-color: #ccc;
		}
		#parent {
			background-color: green;
			position: relative;
			top: 50%;
		}
		#relative {
			width: 100%;
			height: 25%;
			background-color: red;
			position: relative;
			top: 50%;
		}
	</style>
</head>

<body>
	<div id="parent">
		<div id="relative"/>
	</div>
</body>
</rml>
)";

TEST_CASE("Layout.Position.Relative")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	// Test that percentage positioning in 'position: relative' elements is correctly resolved during the first layout run, and
	// does not change during the next layout run. See issue: https://github.com/mikke89/RmlUi/issues/262

	for (auto&& rml_source : {document_layout_rml, document_layout_rml_nested})
	{
		ElementDocument* document = context->LoadDocumentFromMemory(rml_source);
		REQUIRE(document);
		document->Show();

		Element* element = document->GetElementById("relative");
		REQUIRE(element);

		TestsShell::RenderLoop();

		const float absolute_top = element->GetAbsoluteTop();
		CHECK(absolute_top >= 150.f);

		// This forces a new layout run but shouldn't make any difference to the rendered output.
		document->SetProperty("width", "500px");
		TestsShell::RenderLoop();

		CHECK(absolute_top == element->GetAbsoluteTop());

		document->SetProperty("width", "400px");
		TestsShell::RenderLoop();

		CHECK(absolute_top == element->GetAbsoluteTop());

		document->Close();
	}

	TestsShell::ShutdownShell();
}

TEST_CASE("Layout.ScrollableOverflow.MarginTriggersScrollbar")
{
	const String document_rml = R"(
<rml>
<head>
	<link type="text/rcss" href="/../Tests/Data/style.rcss"/>
	<style>
		scrollbarvertical { width: 15px; }
		scrollbarhorizontal { height: 15px; }
		.scroller { width: 100px; height: 100px; background: #3a3; }
		.item { background: #33a; }
		.vertical { overflow-x: hidden; overflow-y: auto; }
		.horizontal { overflow-x: auto; overflow-y: hidden; }
		.flex { display: flex; }
		.flex.vertical { flex-direction: column; }
		.flex.horizontal { flex-direction: row; }
		.flex .item { flex-shrink: 0; }
		.vertical .item { width: 20px; height: 95px; margin-bottom: 20px; }
		.horizontal .item { width: 95px; height: 20px; margin-right: 20px; }
		.vertical .item.fits { height: 80px; }
	</style>
</head>
<body>
	<div class="scroller vertical"><div class="item"/></div>
	<div class="scroller flex vertical"><div class="item"/></div>
	<div class="scroller flex horizontal"><div class="item"/></div>

	<div class="scroller flex vertical"><div class="item fits"/></div>
	<div class="scroller vertical"><div class="item fits"/></div>
</body>
</rml>
)";

	// A direct child whose margin box extends past the scroll container's padding area should enable an auto-scrollbar, even
	// when the child's border box alone fits.
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	REQUIRE(document);
	document->Show();
	TestsShell::RenderLoop();

	CHECK(document->GetChild(0)->GetScrollHeight() == doctest::Approx(115.f));
	CHECK(document->GetChild(0)->GetClientWidth() == doctest::Approx(100.f - 15.f)); // Height - Scrollbar

	CHECK(document->GetChild(1)->GetScrollHeight() == doctest::Approx(115.f));
	CHECK(document->GetChild(1)->GetClientWidth() == doctest::Approx(100.f - 15.f)); // Height - Scrollbar

	CHECK(document->GetChild(2)->GetScrollWidth() == doctest::Approx(115.f));
	CHECK(document->GetChild(2)->GetClientHeight() == doctest::Approx(100.f - 15.f)); // Width - Scrollbar

	// The last two items fits just about, no scrollbars should appear.
	CHECK(document->GetChild(3)->GetScrollHeight() == doctest::Approx(100.f));
	CHECK(document->GetChild(3)->GetClientWidth() == doctest::Approx(100.f)); // No scrollbar.

	CHECK(document->GetChild(4)->GetScrollHeight() == doctest::Approx(100.f));
	CHECK(document->GetChild(4)->GetClientWidth() == doctest::Approx(100.f)); // No scrollbar.

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Layout.ScrollableOverflow.AutoHeightHorizontalScrollbar")
{
	const String document_rml = R"(
<rml>
<head>
	<link type="text/rcss" href="/../Tests/Data/style.rcss"/>
	<style>
		scrollbarvertical { width: 15px; }
		scrollbarhorizontal { height: 15px; }
		.scroller { width: 120px; border: 2px #000; overflow: auto; background: #3a3; }
		.scroller div { background: #33a; }
		.flex { display: flex; flex-direction: row; }
		.item { flex-shrink: 0; width: 240px; height: 60px; }
		.tall { height: 200px; }
		.fixed { height: 80px; }
	</style>
</head>
<body>
	<div class="scroller flex"><div class="item"/></div>
	<div class="scroller"><div class="item"/></div>

	<div class="scroller flex fixed"><div class="item tall"/></div>
	<div class="scroller fixed"><div class="item tall"/></div>
</body>
</rml>
)";

	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	REQUIRE(document);
	document->Show();
	TestsShell::RenderLoop();

	// Auto height, horizontal scrollbar only.
	for (int i = 0; i < 2; i++)
	{
		INFO("Scroller: " << i);
		Element* e = document->GetChild(i);
		CHECK(e->GetClientWidth() == doctest::Approx(120.f)); // No vertical scrollbar.
		CHECK(e->GetClientHeight() == doctest::Approx(60.f)); // Container height - scrollbar height.
		CHECK(e->GetScrollWidth() == doctest::Approx(240.f));
	}

	// Fixed height with overflow on both axes, both scrollbars needed.
	for (int i = 2; i < 4; i++)
	{
		INFO("Scroller: " << i);
		Element* e = document->GetChild(i);
		CHECK(e->GetClientWidth() == doctest::Approx(105.f)); // Container width - vertical scrollbar.
		CHECK(e->GetClientHeight() == doctest::Approx(65.f)); // Container height - horizontal scrollbar.
	}

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Layout.ScrollableOverflow.ChildMarginTriggersScrollbar")
{
	const String document_rml = R"(
<rml>
<head>
	<link type="text/rcss" href="/../Tests/Data/style.rcss"/>
	<style>
		scrollbarvertical { width: 15px; }
		scrollbarhorizontal { height: 15px; }
		.scroller { width: 100px; height: 80px; padding-bottom: 20px; overflow: auto; background: #3a3; }
		.flex { display: flex; }
		/* Height + margin-bottom overflows onto the container padding. Both child margin and container padding should be reachable by scrolling. */
		.child { flex-shrink: 0; width: 20px; height: 70px; margin-bottom: 25px; background: #33a; }
	</style>
</head>
<body>
	<div id="block" class="scroller"><div class="child"/></div>
	<div id="flex" class="scroller flex"><div class="child"/></div>
</body>
</rml>
)";

	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	REQUIRE(document);
	document->Show();
	TestsShell::RenderLoop();

	for (const char* id : {"block", "flex"})
	{
		INFO("Scroller: " << id);
		Element* e = document->GetElementById(id);
		REQUIRE(e);
		CHECK(e->GetScrollHeight() == doctest::Approx(95.f + 20.f)); // Child height (w/margin) + container padding
		CHECK(e->GetClientWidth() == doctest::Approx(100.f - 15.f)); // Container width - Scrollbar width
	}

	document->Close();
	TestsShell::ShutdownShell();
}
