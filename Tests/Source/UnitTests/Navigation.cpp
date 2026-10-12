#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Input.h>
#include <doctest.h>

using namespace Rml;

static const String navigation_grid_rml = R"(
<rml>
<head>
<style>
	body {
		position: relative;
		width: 400px;
		height: 400px;
		background-color: #ddd;
	}
	.box {
		position: absolute;
		width: 50px;
		height: 50px;
		tab-index: auto;
	}
	#a1,#a2,#a3 { background-color: #c33; }
	#b1,#b2,#b3 { background-color: #3c3; }
	#c1,#c2,#c3 { background-color: #33c; }
	#a1 { top: 0px;   left: 0px;   }
	#a2 { top: 0px;   left: 100px; }
	#a3 { top: 0px;   left: 200px; }
	#b1 { top: 100px; left: 0px;   }
	#b2 { top: 100px; left: 100px; }
	#b3 { top: 100px; left: 200px; }
	#c1 { top: 200px; left: 0px;   }
	#c2 { top: 200px; left: 100px; }
	#c3 { top: 200px; left: 200px; }
</style>
</head>
<body id="main">
	<div id="a1" class="box"/>
	<div id="a2" class="box"/>
	<div id="a3" class="box"/>
	<div id="b1" class="box"/>
	<div id="b2" class="box"/>
	<div id="b3" class="box"/>
	<div id="c1" class="box"/>
	<div id="c2" class="box"/>
	<div id="c3" class="box"/>
</body>
</rml>
)";

static const String scroll_container_rml = R"(
<rml>
<head>
<style>
	body {
		position: relative;
		width: 400px;
		height: 400px;
		background-color: #ddd;
	}
	.box {
		position: absolute;
		width: 50px;
		height: 50px;
		tab-index: auto;
		background-color: #c33;
	}
	#scroller {
		position: absolute;
		top: 100px;
		left: 0px;
		width: 250px;
		height: 150px;
		overflow: auto;
	}
	#a1 { top: 0px;   left: 0px;   }
	#a2 { top: 0px;   left: 100px; }
	#a3 { top: 0px;   left: 200px; }
	#b1 { top: 0px;   left: 0px;   }
	#b2 { top: 0px;   left: 100px; }
	#c1 { top: 60px;  left: 0px;   }
	#c2 { top: 60px;  left: 100px; }
	#d1 { top: 300px; left: 0px;   }
	#d2 { top: 300px; left: 100px; }
	#d3 { top: 300px; left: 200px; }
</style>
</head>
<body id="main">
	<div id="a1" class="box"/>
	<div id="a2" class="box"/>
	<div id="a3" class="box"/>
	<div id="scroller">
		<div id="b1" class="box"/>
		<div id="b2" class="box"/>
		<div id="c1" class="box"/>
		<div id="c2" class="box"/>
	</div>
	<div id="d1" class="box"/>
	<div id="d2" class="box"/>
	<div id="d3" class="box"/>
</body>
</rml>
)";

static void SetNavOnAllBoxes(ElementDocument* document, const String& nav_value)
{
	ElementList elements;
	document->GetElementsByClassName(elements, "box");
	for (Element* element : elements)
		element->SetProperty("nav", nav_value);
	document->UpdateDocument();
}

static String FocusAndPressWithContext(Context* context, ElementDocument* document, const String& start_id, Input::KeyIdentifier key)
{
	document->GetElementById(start_id)->Focus();
	context->ProcessKeyDown(key, 0);
	return context->GetFocusElement()->GetId();
};

TEST_CASE("Navigation.Spatial")
{
	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocumentFromMemory(navigation_grid_rml);
	document->Show();
	TestsShell::RenderLoop();

	auto FocusAndPress = [&](const String& start_id, Input::KeyIdentifier key) -> String {
		return FocusAndPressWithContext(context, document, start_id, key);
	};

	SUBCASE("Default (no nav property): arrows do nothing")
	{
		CHECK(FocusAndPress("b2", Input::KI_UP) == "b2");
		CHECK(FocusAndPress("b2", Input::KI_DOWN) == "b2");
		CHECK(FocusAndPress("b2", Input::KI_LEFT) == "b2");
		CHECK(FocusAndPress("b2", Input::KI_RIGHT) == "b2");
	}

	SUBCASE("nav: none: arrows do nothing")
	{
		SetNavOnAllBoxes(document, "none");
		CHECK(FocusAndPress("b2", Input::KI_UP) == "b2");
		CHECK(FocusAndPress("b2", Input::KI_RIGHT) == "b2");
	}

	SUBCASE("nav: auto: arrows navigate in all four directions")
	{
		SetNavOnAllBoxes(document, "auto");
		CHECK(FocusAndPress("b2", Input::KI_UP) == "a2");
		CHECK(FocusAndPress("b2", Input::KI_DOWN) == "c2");
		CHECK(FocusAndPress("b2", Input::KI_LEFT) == "b1");
		CHECK(FocusAndPress("b2", Input::KI_RIGHT) == "b3");

		// From a corner there is nothing further in the boundary directions.
		CHECK(FocusAndPress("a1", Input::KI_UP) == "a1");
		CHECK(FocusAndPress("a1", Input::KI_LEFT) == "a1");
		CHECK(FocusAndPress("a1", Input::KI_DOWN) == "b1");
		CHECK(FocusAndPress("a1", Input::KI_RIGHT) == "a2");
	}

	SUBCASE("nav: horizontal: only left/right navigate")
	{
		SetNavOnAllBoxes(document, "horizontal");
		CHECK(FocusAndPress("b2", Input::KI_LEFT) == "b1");
		CHECK(FocusAndPress("b2", Input::KI_RIGHT) == "b3");
		CHECK(FocusAndPress("b2", Input::KI_UP) == "b2");
		CHECK(FocusAndPress("b2", Input::KI_DOWN) == "b2");
	}

	SUBCASE("nav: vertical: only up/down navigate")
	{
		SetNavOnAllBoxes(document, "vertical");
		CHECK(FocusAndPress("b2", Input::KI_UP) == "a2");
		CHECK(FocusAndPress("b2", Input::KI_DOWN) == "c2");
		CHECK(FocusAndPress("b2", Input::KI_LEFT) == "b2");
		CHECK(FocusAndPress("b2", Input::KI_RIGHT) == "b2");
	}

	SUBCASE("Per-direction keyword: only the configured direction navigates")
	{
		document->GetElementById("b2")->SetProperty("nav-right", "auto");
		document->UpdateDocument();
		CHECK(FocusAndPress("b2", Input::KI_RIGHT) == "b3");
		CHECK(FocusAndPress("b2", Input::KI_LEFT) == "b2");
		CHECK(FocusAndPress("b2", Input::KI_UP) == "b2");
		CHECK(FocusAndPress("b2", Input::KI_DOWN) == "b2");
	}

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Navigation.Explicit")
{
	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocumentFromMemory(navigation_grid_rml);
	document->Show();
	TestsShell::RenderLoop();

	SUBCASE("nav-X: #id navigates to the referenced element")
	{
		Element* b2 = document->GetElementById("b2");
		b2->SetProperty("nav-up", "#c3");
		b2->SetProperty("nav-right", "#a1");
		document->UpdateDocument();

		b2->Focus();
		context->ProcessKeyDown(Input::KI_UP, 0);
		CHECK(context->GetFocusElement()->GetId() == "c3");

		b2->Focus();
		context->ProcessKeyDown(Input::KI_RIGHT, 0);
		CHECK(context->GetFocusElement()->GetId() == "a1");
	}

	SUBCASE("Unknown id: focus stays and a warning is logged")
	{
		TestsShell::SetNumExpectedWarnings(1);

		Element* b2 = document->GetElementById("b2");
		b2->SetProperty("nav-up", "#does_not_exist");
		document->UpdateDocument();

		b2->Focus();
		context->ProcessKeyDown(Input::KI_UP, 0);
		CHECK(context->GetFocusElement()->GetId() == "b2");
	}

	SUBCASE("Missing '#' prefix: focus stays and a warning is logged")
	{
		TestsShell::SetNumExpectedWarnings(1);

		Element* b2 = document->GetElementById("b2");
		b2->SetProperty("nav-up", "a1");
		document->UpdateDocument();

		b2->Focus();
		context->ProcessKeyDown(Input::KI_UP, 0);
		CHECK(context->GetFocusElement()->GetId() == "b2");
	}

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Navigation.TreeOrder")
{
	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocumentFromMemory(navigation_grid_rml);
	document->Show();
	TestsShell::RenderLoop();

	SetNavOnAllBoxes(document, "tree-order");

	auto FocusAndPress = [&](const String& start_id, Input::KeyIdentifier key) -> String {
		return FocusAndPressWithContext(context, document, start_id, key);
	};

	// Forward direction follows tab order
	CHECK(FocusAndPress("b2", Input::KI_DOWN) == "b3");
	CHECK(FocusAndPress("b3", Input::KI_RIGHT) == "c1");

	// Backward direction follows reverse tab order
	CHECK(FocusAndPress("b2", Input::KI_UP) == "b1");
	CHECK(FocusAndPress("b1", Input::KI_LEFT) == "a3");

	// Does not wrap at the document boundary
	CHECK(FocusAndPress("c3", Input::KI_DOWN) == "c3");
	CHECK(FocusAndPress("c3", Input::KI_RIGHT) == "c3");
	CHECK(FocusAndPress("a1", Input::KI_UP) == "a1");
	CHECK(FocusAndPress("a1", Input::KI_LEFT) == "a1");

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Navigation.Body")
{
	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocumentFromMemory(navigation_grid_rml);
	document->Show();
	TestsShell::RenderLoop();

	SUBCASE("default: Does not navigate")
	{
		document->Focus();
		context->ProcessKeyDown(Input::KI_RIGHT, 0);
		CHECK(context->GetFocusElement()->GetId() == "main");

		document->Focus();
		context->ProcessKeyDown(Input::KI_UP, 0);
		CHECK(context->GetFocusElement()->GetId() == "main");
	}

	SUBCASE("auto: Navigates in tree-order")
	{
		document->SetProperty("nav", "auto");
		document->UpdateDocument();

		document->Focus();
		context->ProcessKeyDown(Input::KI_RIGHT, 0);
		CHECK(context->GetFocusElement()->GetId() == "a1");

		document->Focus();
		context->ProcessKeyDown(Input::KI_UP, 0);
		CHECK(context->GetFocusElement()->GetId() == "c3");
	}

	SUBCASE("tree-order: Navigates in tree-order")
	{
		document->SetProperty("nav", "tree-order");
		document->UpdateDocument();

		document->Focus();
		context->ProcessKeyDown(Input::KI_RIGHT, 0);
		CHECK(context->GetFocusElement()->GetId() == "a1");

		document->Focus();
		context->ProcessKeyDown(Input::KI_UP, 0);
		CHECK(context->GetFocusElement()->GetId() == "c3");
	}

	SUBCASE("id: Navigates to that id")
	{
		document->SetProperty("nav", "#a2 #b3");
		document->UpdateDocument();

		document->Focus();
		context->ProcessKeyDown(Input::KI_RIGHT, 0);
		CHECK(context->GetFocusElement()->GetId() == "b3");

		document->Focus();
		context->ProcessKeyDown(Input::KI_UP, 0);
		CHECK(context->GetFocusElement()->GetId() == "a2");
	}
	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Navigation.ScrollContainer")
{
	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocumentFromMemory(scroll_container_rml);
	document->Show();
	TestsShell::RenderLoop();

	auto FocusAndPress = [&](const String& start_id, Input::KeyIdentifier key) -> String {
		return FocusAndPressWithContext(context, document, start_id, key);
	};

	SUBCASE("nav: auto navigates past scroll containers from outside")
	{
		SetNavOnAllBoxes(document, "auto");
		CHECK(FocusAndPress("a2", Input::KI_DOWN) == "d2");
		CHECK(FocusAndPress("d2", Input::KI_UP) == "a2");
	}

	SUBCASE("nav: auto contains navigation within a scroll container")
	{
		SetNavOnAllBoxes(document, "auto");
		CHECK(FocusAndPress("b1", Input::KI_RIGHT) == "b2");
		CHECK(FocusAndPress("b1", Input::KI_DOWN) == "c1");
		CHECK(FocusAndPress("c2", Input::KI_UP) == "b2");

		CHECK(FocusAndPress("b1", Input::KI_UP) == "b1");
		CHECK(FocusAndPress("c1", Input::KI_DOWN) == "c1");
	}

	SUBCASE("nav: tree-order enters and exits scroll containers")
	{
		SetNavOnAllBoxes(document, "tree-order");
		CHECK(FocusAndPress("a3", Input::KI_DOWN) == "b1");
		CHECK(FocusAndPress("c2", Input::KI_DOWN) == "d1");
		CHECK(FocusAndPress("b1", Input::KI_UP) == "a3");
		CHECK(FocusAndPress("d1", Input::KI_UP) == "c2");
	}

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Navigation.FindNextTabElement")
{
	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocumentFromMemory(scroll_container_rml);
	document->Show();
	TestsShell::RenderLoop();

	auto NextId = [&](const String& from_id, bool forward, bool wrap = true) -> String {
		Element* current = document->GetElementById(from_id);
		Element* next = document->FindNextTabElement(current, forward, wrap);
		return next ? next->GetId() : "(null)";
	};

	SUBCASE("Forward walks the tab order")
	{
		CHECK(NextId("a1", true) == "a2");
		CHECK(NextId("a3", true) == "b1");
		CHECK(NextId("b1", true) == "b2");
		CHECK(NextId("d2", true) == "d3");
		CHECK(NextId("main", true) == "a1");
		CHECK(NextId("scroller", true) == "b1");
	}

	SUBCASE("Backward walks the reverse tab order")
	{
		CHECK(NextId("a2", false) == "a1");
		CHECK(NextId("b1", false) == "a3");
		CHECK(NextId("b2", false) == "b1");
		CHECK(NextId("d3", false) == "d2");
		CHECK(NextId("main", false) == "d3");
		CHECK(NextId("scroller", false) == "a3");
	}

	SUBCASE("Wraps around the document by default")
	{
		CHECK(NextId("d3", true) == "a1");
		CHECK(NextId("a1", false) == "d3");
	}

	SUBCASE("Wraps to the document with tab-index: auto")
	{
		document->SetProperty("tab-index", "auto");
		document->UpdateDocument();
		CHECK(NextId("d3", true) == "main");
		CHECK(NextId("a1", false) == "main");
	}

	SUBCASE("Navigating from document")
	{
		CHECK(NextId("main", true) == "a1");
		CHECK(NextId("main", false) == "d3");
	}

	SUBCASE("Navigating from document with tab-index: auto")
	{
		document->SetProperty("tab-index", "auto");
		document->UpdateDocument();
		CHECK(NextId("main", true) == "a1");
		CHECK(NextId("main", false) == "d3");
	}

	SUBCASE("Skips elements that are not focusable")
	{
		document->GetElementById("b2")->SetProperty("tab-index", "none");
		document->UpdateDocument();
		CHECK(NextId("b1", true) == "c1");
		CHECK(NextId("c1", false) == "b1");
	}

	SUBCASE("Skips elements that are not visible, including their subtree")
	{
		document->GetElementById("scroller")->SetProperty("visibility", "hidden");
		document->UpdateDocument();
		CHECK(NextId("a3", true) == "d1");
		CHECK(NextId("d1", false) == "a3");
	}

	SUBCASE("wrap_around=false works normally when not wrapping")
	{
		CHECK(NextId("d2", true, false) == "d3");
		CHECK(NextId("a2", false, false) == "a1");

		CHECK(NextId("c2", true, false) == "d1");
		CHECK(NextId("b1", false, false) == "a3");

		CHECK(NextId("main", true, false) == "a1");
		CHECK(NextId("main", false, false) == "d3");
	}

	SUBCASE("wrap_around=false works normally when not wrapping (with tabbable body)")
	{
		document->SetProperty("tab-index", "auto");
		document->UpdateDocument();

		CHECK(NextId("d2", true, false) == "d3");
		CHECK(NextId("a2", false, false) == "a1");

		CHECK(NextId("c2", true, false) == "d1");
		CHECK(NextId("b1", false, false) == "a3");

		CHECK(NextId("main", true, false) == "a1");
		CHECK(NextId("main", false, false) == "d3");
	}

	SUBCASE("wrap_around=false returns nullptr at the boundaries")
	{
		CHECK(NextId("d3", true, false) == "(null)");
		CHECK(NextId("a1", false, false) == "(null)");
	}

	SUBCASE("wrap_around=false returns nullptr at the boundaries (with tabbable body)")
	{
		document->SetProperty("tab-index", "auto");
		document->UpdateDocument();
		CHECK(NextId("d3", true, false) == "(null)");
		CHECK(NextId("a1", false, false) == "(null)");
	}

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Navigation.NoTabbableElements")
{
	const String document_rml = StringUtilities::Replace(navigation_grid_rml, "tab-index: auto", "tab-index: none");

	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	document->Show();
	TestsShell::RenderLoop();

	SUBCASE("No tabbable elements")
	{
		CHECK(document->FindNextTabElement(document->GetElementById("a2"), true, true) == nullptr);
		CHECK(document->FindNextTabElement(document->GetElementById("a2"), false, true) == nullptr);
	}

	SUBCASE("Tabbable document only")
	{
		document->SetProperty("tab-index", "auto");
		document->UpdateDocument();
		CHECK(document->FindNextTabElement(document->GetElementById("a2"), true, true) == document);
		CHECK(document->FindNextTabElement(document->GetElementById("a2"), false, true) == document);
	}

	CHECK(document->FindNextTabElement(document, true, true) == nullptr);
	CHECK(document->FindNextTabElement(document, true, false) == nullptr);
	CHECK(document->FindNextTabElement(document, false, true) == nullptr);
	CHECK(document->FindNextTabElement(document, false, false) == nullptr);
	CHECK(document->FindNextTabElement(document->GetElementById("a2"), true, false) == nullptr);
	CHECK(document->FindNextTabElement(document->GetElementById("a2"), false, false) == nullptr);

	auto FocusAndPress = [&](const String& start_id, Input::KeyIdentifier key) -> String {
		return FocusAndPressWithContext(context, document, start_id, key);
	};

	SetNavOnAllBoxes(document, "auto");
	CHECK(FocusAndPress("main", Input::KI_DOWN) == "main");
	CHECK(FocusAndPress("main", Input::KI_UP) == "main");
	CHECK(FocusAndPress("a1", Input::KI_UP) == "a1");
	CHECK(FocusAndPress("b2", Input::KI_RIGHT) == "b2");
	CHECK(FocusAndPress("c1", Input::KI_LEFT) == "c1");
	CHECK(FocusAndPress("c3", Input::KI_DOWN) == "c3");

	SetNavOnAllBoxes(document, "tab-order");
	CHECK(FocusAndPress("main", Input::KI_DOWN) == "main");
	CHECK(FocusAndPress("main", Input::KI_UP) == "main");
	CHECK(FocusAndPress("a1", Input::KI_UP) == "a1");
	CHECK(FocusAndPress("b2", Input::KI_RIGHT) == "b2");
	CHECK(FocusAndPress("c1", Input::KI_LEFT) == "c1");
	CHECK(FocusAndPress("c3", Input::KI_DOWN) == "c3");

	document->Close();
	TestsShell::ShutdownShell();
}
