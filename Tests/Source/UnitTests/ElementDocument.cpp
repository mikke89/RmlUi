#include "../Common/Mocks.h"
#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Factory.h>
#include <algorithm>
#include <doctest.h>

using namespace Rml;

static void SimulateClick(Context* context, Vector2i position)
{
	context->Update();
	context->ProcessMouseMove(position.x, position.y, 0);
	context->Update();
	context->ProcessMouseButtonDown(0, 0);
	context->Update();
	context->ProcessMouseButtonUp(0, 0);
	context->Update();
}

static const String document_focus_rml = R"(
<rml>
<head>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<link type="text/rcss" href="/assets/invader.rcss"/>
	<style>
		button {
			display: inline-block;
			tab-index: auto;
		}
		:focus {
			image-color: #af0;
		}
		:disabled {
			image-color: #666;
		}
		.hide {
			visibility: hidden;
		}
		.nodisplay {
			display: none;
		}
	</style>
</head>

<body id="body" onload="something">
<input type="checkbox" id="p1"/> P1
<label><input type="checkbox" id="p2"/> P2</label>
<p>
	<input type="checkbox" id="p3"/><label for="p3"> P3</label>
</p>
<p class="nodisplay">
	<input type="checkbox" id="p4"/><label for="p4"> P4</label>
</p>
<input type="checkbox" id="p5"/> P5
<p>
	<input type="checkbox" id="p6" disabled/><label for="p6"> P6</label>
</p>
<div>
	<label><input type="checkbox" id="p7"/> P7</label>
	<label class="hide"><input type="checkbox" id="p8"/> P8</label>
	<label><button id="p9"> P9</button></label>
	<label><input type="checkbox" id="p10"/> P10</label>
</div>
<div id="container">
	<p> Deeply nested
		<span>
			<input type="checkbox" id="p11"/> P11
		</span>
		<input type="checkbox" id="p12"/> P12
	</p>
	<input type="checkbox" id="p13"/> P13
</div>
</body>
</rml>
)";

static const String focus_forward = "p1 p2 p3 p5 p7 p9 p10 p11 p12 p13";

TEST_SUITE_BEGIN("ElementDocument");

TEST_CASE("Focus")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_focus_rml);
	REQUIRE(document);
	document->Show();

	context->Update();
	context->Render();

	TestsShell::RenderLoop();
	SUBCASE("Tab order")
	{
		StringList ids;
		StringUtilities::ExpandString(ids, focus_forward, ' ');
		REQUIRE(!ids.empty());

		document->Focus();
		SUBCASE("Forward")
		{
			for (const String& id : ids)
			{
				context->ProcessKeyDown(Input::KI_TAB, 0);
				CHECK(context->GetFocusElement()->GetId() == id);
			}

			// Wrap around
			context->ProcessKeyDown(Input::KI_TAB, 0);
			CHECK(context->GetFocusElement()->GetId() == ids[0]);
		}

		SUBCASE("Reverse")
		{
			std::reverse(ids.begin(), ids.end());
			for (const String& id : ids)
			{
				context->ProcessKeyDown(Input::KI_TAB, Input::KM_SHIFT);
				CHECK(context->GetFocusElement()->GetId() == id);
			}

			// Wrap around (reverse)
			context->ProcessKeyDown(Input::KI_TAB, Input::KM_SHIFT);
			CHECK(context->GetFocusElement()->GetId() == ids[0]);
		}
	}

	SUBCASE("Tab to document")
	{
		Element* element = document->GetElementById("p13");
		REQUIRE(element);
		element->Focus();

		document->SetProperty("tab-index", "auto");
		document->UpdateDocument();

		context->ProcessKeyDown(Input::KI_TAB, 0);
		CHECK(context->GetFocusElement()->GetId() == "body");
	}

	SUBCASE("Tab from container element")
	{
		Element* container = document->GetElementById("container");
		REQUIRE(container);

		container->Focus();
		context->ProcessKeyDown(Input::KI_TAB, 0);
		CHECK(context->GetFocusElement()->GetId() == "p11");

		container->Focus();
		context->ProcessKeyDown(Input::KI_TAB, Input::KM_SHIFT);
		CHECK(context->GetFocusElement()->GetId() == "p10");
	}

	SUBCASE("Single element")
	{
		document->SetProperty("tab-index", "none");
		document->SetInnerRML(R"(<input type="checkbox" id="p1"/> P1)");
		document->UpdateDocument();

		document->Focus();
		context->ProcessKeyDown(Input::KI_TAB, 0);
		CHECK(context->GetFocusElement()->GetId() == "p1");
		context->ProcessKeyDown(Input::KI_TAB, 0);
		CHECK(context->GetFocusElement()->GetId() == "p1");
		context->ProcessKeyDown(Input::KI_TAB, Input::KM_SHIFT);
		CHECK(context->GetFocusElement()->GetId() == "p1");

		document->SetProperty("tab-index", "auto");
		document->UpdateDocument();

		document->Focus();
		context->ProcessKeyDown(Input::KI_TAB, 0);
		CHECK(context->GetFocusElement()->GetId() == "p1");
		context->ProcessKeyDown(Input::KI_TAB, 0);
		CHECK(context->GetFocusElement()->GetId() == "body");
		context->ProcessKeyDown(Input::KI_TAB, Input::KM_SHIFT);
		CHECK(context->GetFocusElement()->GetId() == "p1");
		context->ProcessKeyDown(Input::KI_TAB, Input::KM_SHIFT);
		CHECK(context->GetFocusElement()->GetId() == "body");
	}

	SUBCASE("Single, non-tabable element")
	{
		document->SetProperty("tab-index", "none");
		document->SetInnerRML(R"(<div id="child"/>)");
		document->UpdateDocument();
		Element* child = document->GetChild(0);

		document->Focus();
		context->ProcessKeyDown(Input::KI_TAB, 0);
		CHECK(context->GetFocusElement()->GetId() == "body");
		context->ProcessKeyDown(Input::KI_TAB, Input::KM_SHIFT);
		CHECK(context->GetFocusElement()->GetId() == "body");

		child->Focus();
		context->ProcessKeyDown(Input::KI_TAB, 0);
		CHECK(context->GetFocusElement()->GetId() == "child");
		context->ProcessKeyDown(Input::KI_TAB, Input::KM_SHIFT);
		CHECK(context->GetFocusElement()->GetId() == "child");

		document->SetProperty("tab-index", "auto");
		document->UpdateDocument();

		document->Focus();
		context->ProcessKeyDown(Input::KI_TAB, 0);
		CHECK(context->GetFocusElement()->GetId() == "body");
		context->ProcessKeyDown(Input::KI_TAB, Input::KM_SHIFT);
		CHECK(context->GetFocusElement()->GetId() == "body");

		child->Focus();
		context->ProcessKeyDown(Input::KI_TAB, 0);
		CHECK(context->GetFocusElement()->GetId() == "body");
		child->Focus();
		context->ProcessKeyDown(Input::KI_TAB, Input::KM_SHIFT);
		CHECK(context->GetFocusElement()->GetId() == "body");
	}

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Load")
{
	namespace tl = trompeloeil;
	constexpr auto BODY_TAG = "body";

	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	MockEventListener mockEventListener;
	MockEventListenerInstancer mockEventListenerInstancer;

	tl::sequence sequence;
	REQUIRE_CALL(mockEventListenerInstancer, InstanceEventListener("something", tl::_))
		.WITH(_2->GetTagName() == BODY_TAG)
		.IN_SEQUENCE(sequence)
		.LR_RETURN(&mockEventListener);

	ALLOW_CALL(mockEventListener, OnAttach(tl::_));
	ALLOW_CALL(mockEventListener, OnDetach(tl::_));

	REQUIRE_CALL(mockEventListener, ProcessEvent(tl::_))
		.WITH(_1.GetId() == EventId::Load && _1.GetTargetElement()->GetTagName() == BODY_TAG)
		.IN_SEQUENCE(sequence);

	Factory::RegisterEventListenerInstancer(&mockEventListenerInstancer);

	ElementDocument* document = context->LoadDocumentFromMemory(document_focus_rml);
	REQUIRE(document);

	document->Close();

	TestsShell::ShutdownShell();
}

TEST_CASE("ReloadStyleSheet")
{
	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocument("basic/demo/data/demo.rml");

	// There should be no warnings when reloading style sheets.
	document->ReloadStyleSheet();

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Modal.MultipleDocuments")
{
	Context* context = TestsShell::GetContext();

	constexpr float halfwidth = 150;

	ElementDocument* document1 = context->LoadDocument("assets/demo.rml");
	document1->Show(Rml::ModalFlag::Modal);
	document1->GetElementById("title")->SetInnerRML("Modal 1");
	constexpr float margin1 = 50;
	document1->SetProperty(PropertyId::MarginLeft, Property{margin1, Unit::PX});

	Rml::ElementDocument* document2 = context->LoadDocument("assets/demo.rml");
	document2->Show(Rml::ModalFlag::Modal);
	document2->GetElementById("title")->SetInnerRML("Modal 2");
	constexpr float margin2 = 350;
	document2->SetProperty(PropertyId::MarginLeft, Property{margin2, Unit::PX});

	Rml::ElementDocument* document3 = context->LoadDocument("assets/demo.rml");
	document3->Show(Rml::ModalFlag::None);
	document3->GetElementById("title")->SetInnerRML("Non-modal");
	constexpr float margin3 = 650;
	document3->SetProperty(PropertyId::MarginLeft, Property{margin3, Unit::PX});

	context->Update();

	TestsShell::RenderLoop();

	REQUIRE(context->GetFocusElement() == document2);

	SUBCASE("FocusFromModal")
	{
		document1->Focus();
		REQUIRE(context->GetFocusElement() == document1);

		document3->Focus();
		REQUIRE(context->GetFocusElement() == document1);
	}

	SUBCASE("ClickFromModal")
	{
		SimulateClick(context, Vector2i(int(margin1 + halfwidth), context->GetDimensions().y / 2));
		REQUIRE(context->GetFocusElement() == document2);

		SimulateClick(context, Vector2i(int(margin3 + halfwidth), context->GetDimensions().y / 2));
		REQUIRE(context->GetFocusElement() == document2);
	}

	SUBCASE("ModalFlag")
	{
		document1->Show(ModalFlag::None, FocusFlag::Document);
		REQUIRE(context->GetFocusElement() == document2);

		document3->Show(ModalFlag::None, FocusFlag::Document);
		REQUIRE(context->GetFocusElement() == document2);

		document3->Show(ModalFlag::Modal, FocusFlag::Document);
		REQUIRE(context->GetFocusElement() == document3);
	}

	document1->Close();
	document2->Close();
	document3->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Modal.FocusWithin")
{
	Context* context = TestsShell::GetContext();

	Rml::ElementDocument* document = context->LoadDocument("assets/demo.rml");
	document->GetElementById("content")->SetInnerRML("<input type='text' id='input'/>");
	document->Show(Rml::ModalFlag::Modal);

	REQUIRE(context->GetFocusElement() == document);
	Element* input = document->GetElementById("input");
	input->Focus();
	REQUIRE(context->GetFocusElement() == input);

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("ScrollFlag")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	static const String document_rml = R"(
<rml>
<head>
<title>Demo</title>
<link type="text/rcss" href="/assets/rml.rcss" />
<link type="text/rcss" href="/../Tests/Data/style.rcss" />
<style>
	body {
		width: 400px;
		height: 300px;
		overflow-y: scroll;
		background-color: #333;
	}
	div {
		height: 200px;
		background-color: #c33;
		margin: 5px 0;
	}
</style>
</head>
<body>
	<div/>
	<div/>
	<input id="input" type="text" autofocus/>
	<div/>
	<div/>
</body>
</rml>
)";

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	REQUIRE(document);
	Element* input = document->GetElementById("input");
	REQUIRE(input);

	constexpr float scroll_y_none = 0.f;
	constexpr float scroll_y_input = 138.f;

	SUBCASE("Default")
	{
		document->Show();
		CHECK(document->GetScrollTop() == scroll_y_input);
	}
	SUBCASE("Auto")
	{
		document->Show(ModalFlag::None, FocusFlag::Auto, ScrollFlag::Auto);
		CHECK(document->GetScrollTop() == scroll_y_input);
	}
	SUBCASE("FocusNone")
	{
		document->Show(ModalFlag::None, FocusFlag::None, ScrollFlag::Auto);
		CHECK(document->GetScrollTop() == scroll_y_none);
	}
	SUBCASE("FocusDocument")
	{
		document->Show(ModalFlag::None, FocusFlag::Document, ScrollFlag::Auto);
		CHECK(document->GetScrollTop() == scroll_y_none);
	}
	SUBCASE("ScrollNone")
	{
		document->Show(ModalFlag::None, FocusFlag::Auto, ScrollFlag::None);
		CHECK(document->GetScrollTop() == scroll_y_none);
	}

	TestsShell::RenderLoop();

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_SUITE_END();
