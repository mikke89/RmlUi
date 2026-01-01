#include "../Common/Mocks.h"
#include "../Common/TestsInterface.h"
#include "../Common/TestsShell.h"
#include "../Common/TypesToString.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Factory.h>
#include <doctest.h>

using namespace Rml;

static const String document_clone_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<link type="text/rcss" href="/assets/invader.rcss"/>
	<style>
		body
		{
			left: 0;
			top: 0;
			right: 0;
			bottom: 0;
		}
		div {
			drag: clone;
			background-color: #fff;
			height: 100px;
		}
		div.blue {
			background-color: #00f;
		}
		span {
			color: red;
		}
	</style>
</head>

<body>
<div style="background-color: #f00">This is a <span>sample</span>.</div>
</body>
</rml>
)";

static const String document_scroll_rml = R"(
<rml>
<head>
	<title>Test</title>
	<style>
		body {
			left: 0;
			top: 0;
			width: 100px;
			height: 100px;
			padding: 0;
			margin: 0;
			overflow: scroll;
		}
		div {
			display: block;
			width: 200px;
			height: 50px;
			padding: 0;
			margin: 0;
		}
		#row0 { background: #3a3; }
		#row1 { background: #aa3; }
		#row2 { background: #3aa; }
		#row3 { background: #aaa; }
		span {
			display: inline-block;
			box-sizing: border-box;
			width: 50px;
			height: 50px;
			padding: 0;
			margin: 0;
			border: 1px #000;
		}
		span:nth-child(2) { border-color: #fff; }
		span:nth-child(3) { border-color: #f00; }
		span:nth-child(4) { border-color: #ff0; }
		scrollbarvertical,
		scrollbarhorizontal {
			width: 0;
			height: 0;
		}
	</style>
</head>

<body id="scrollable">
<div id="row0">
	<span id="cell00"></span>
	<span id="cell01"></span>
	<span id="cell02"></span>
	<span id="cell03"></span>
</div>
<div id="row1">
	<span id="cell10"></span>
	<span id="cell11"></span>
	<span id="cell12"></span>
	<span id="cell13"></span>
</div>
<div id="row2">
	<span id="cell20"></span>
	<span id="cell21"></span>
	<span id="cell22"></span>
	<span id="cell23"></span>
</div>
<div id="row3">
	<span id="cell30"></span>
	<span id="cell31"></span>
	<span id="cell32"></span>
	<span id="cell33"></span>
</div>
</body>
</rml>
)";

void Run(Context* context)
{
	context->Update();
	context->Render();

	TestsShell::RenderLoop();
}

TEST_CASE("Element")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_clone_rml);
	REQUIRE(document);
	document->Show();

	context->Update();
	context->Render();

	Element* div = document->GetFirstChild();
	Element* span = div->GetChild(1);
	REQUIRE(div);
	REQUIRE(div->GetTagName() == "div");
	REQUIRE(span);
	REQUIRE(span->GetTagName() == "span");

	SUBCASE("Attribute")
	{
		auto* button = document->AppendChild(document->CreateElement("button"));
		SUBCASE("Event listener")
		{
			namespace tl = trompeloeil;
			static constexpr auto ON_CLICK_ATTRIBUTE = "onclick";
			static constexpr auto ON_CLICK_VALUE = "moo";

			std::vector<UniquePtr<tl::expectation>> expectations;

			UniquePtr<MockEventListener> mockEventListener;
			const auto configureMockEventListener = [&]() {
				mockEventListener.reset(new MockEventListener());
				expectations.emplace_back(NAMED_ALLOW_CALL(*mockEventListener, OnAttach(button)));
				expectations.emplace_back(NAMED_ALLOW_CALL(*mockEventListener, OnDetach(button)).LR_SIDE_EFFECT(mockEventListener.reset()));
			};

			MockEventListenerInstancer mockEventListenerInstancer;
			const auto configureMockEventListenerInstancer = [&](const auto value) {
				expectations.emplace_back(NAMED_REQUIRE_CALL(mockEventListenerInstancer, InstanceEventListener(value, button))
											  .LR_SIDE_EFFECT(configureMockEventListener())
											  .LR_RETURN(mockEventListener.get()));
			};

			Factory::RegisterEventListenerInstancer(&mockEventListenerInstancer);

			configureMockEventListenerInstancer(ON_CLICK_VALUE);
			button->SetAttribute(ON_CLICK_ATTRIBUTE, ON_CLICK_VALUE);

			SUBCASE("Replacement")
			{
				static constexpr auto REPLACEMENT_ON_CLICK_VALUE = "boo";

				configureMockEventListenerInstancer(REPLACEMENT_ON_CLICK_VALUE);
				button->SetAttribute(ON_CLICK_ATTRIBUTE, REPLACEMENT_ON_CLICK_VALUE);
			}

			button->RemoveAttribute(ON_CLICK_ATTRIBUTE);
		}

		SUBCASE("Simple")
		{
			static constexpr auto DISABLED_ATTRIBUTE = "disabled";

			REQUIRE_FALSE(button->HasAttribute(DISABLED_ATTRIBUTE));
			button->SetAttribute(DISABLED_ATTRIBUTE, "");
			REQUIRE(button->HasAttribute(DISABLED_ATTRIBUTE));

			SUBCASE("Replacement")
			{
				static constexpr auto VALUE = "something";

				button->SetAttribute(DISABLED_ATTRIBUTE, VALUE);
				const auto* attributeValue = button->GetAttribute(DISABLED_ATTRIBUTE);
				REQUIRE(attributeValue);
				REQUIRE(attributeValue->GetType() == Variant::Type::STRING);
				CHECK(attributeValue->Get<String>() == VALUE);
			}

			button->RemoveAttribute(DISABLED_ATTRIBUTE);
			CHECK_FALSE(button->HasAttribute(DISABLED_ATTRIBUTE));
		}
	}

	SUBCASE("CloneDrag")
	{
		// Simulate input for mouse click and drag
		context->ProcessMouseMove(10, 10, 0);

		context->Update();
		context->Render();

		context->ProcessMouseButtonDown(0, 0);

		context->Update();
		context->Render();

		// This should initiate a drag clone.
		context->ProcessMouseMove(10, 11, 0);

		context->Update();
		context->Render();

		context->ProcessMouseButtonUp(0, 0);
	}

	SUBCASE("CloneManual")
	{
		Element* element = document->GetFirstChild();
		REQUIRE(element->GetProperty<String>("background-color") == "#ff0000");
		CHECK(element->Clone()->GetProperty<String>("background-color") == "#ff0000");

		element->SetProperty("background-color", "#0f0");
		CHECK(element->Clone()->GetProperty<String>("background-color") == "#00ff00");

		element->RemoveProperty("background-color");
		Element* clone = document->AppendChild(element->Clone());
		context->Update();
		CHECK(clone->GetProperty<String>("background-color") == "#ffffff");

		element->SetClass("blue", true);
		clone = document->AppendChild(element->Clone());
		context->Update();
		CHECK(clone->GetProperty<String>("background-color") == "#0000ff");
	}

	SUBCASE("SetInnerRML")
	{
		Element* element = document->GetFirstChild();
		CHECK(element->GetInnerRML() == "This is a <span>sample</span>.");
		element->SetInnerRML("text");
		CHECK(element->GetInnerRML() == "text");

		const char* inner_rml = R"(before<div class="blue">child</div>after)";
		element->SetInnerRML(inner_rml);
		CHECK(element->GetInnerRML() == inner_rml);

		ElementPtr element_ptr = document->CreateElement("div");
		CHECK(element_ptr->GetInnerRML() == "");
		element_ptr->SetInnerRML("text");
		CHECK(element_ptr->GetInnerRML() == "text");
	}

	SUBCASE("GetInnerRML")
	{
		String inner_rml;

		inner_rml = document->GetInnerRML();
		CHECK(inner_rml == R"(<div style="background-color: #ff0000;">This is a <span>sample</span>.</div>)");

		div->SetProperty("background-color", "white");
		inner_rml = document->GetInnerRML();
		CHECK(inner_rml == R"(<div style="background-color: #ffffff;">This is a <span>sample</span>.</div>)");

		div->RemoveProperty("background-color");
		inner_rml = document->GetInnerRML();
		CHECK(inner_rml == R"(<div>This is a <span>sample</span>.</div>)");

		div->SetProperty("cursor", "x<y");
		inner_rml = document->GetInnerRML();
		CHECK(inner_rml == R"(<div style="cursor: x&lt;y;">This is a <span>sample</span>.</div>)");

		span->SetProperty("font-weight", "bold");
		inner_rml = document->GetInnerRML();
		CHECK(inner_rml == R"(<div style="cursor: x&lt;y;">This is a <span style="font-weight: bold;">sample</span>.</div>)");
	}

	SUBCASE("InsertBefore")
	{
		String inner_rml;

		inner_rml = document->GetInnerRML();
		CHECK(inner_rml == R"(<div style="background-color: #ff0000;">This is a <span>sample</span>.</div>)");

		div->InsertBefore(document->CreateElement("img"), span);
		inner_rml = document->GetInnerRML();
		CHECK(inner_rml == R"(<div style="background-color: #ff0000;">This is a <img /><span>sample</span>.</div>)");

		div->InsertBefore(document->CreateElement("button"), nullptr)->SetInnerRML("Click me");
		inner_rml = document->GetInnerRML();
		CHECK(inner_rml == R"(<div style="background-color: #ff0000;">This is a <img /><span>sample</span>.<button>Click me</button></div>)");
	}

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("Element.ScrollIntoView")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_scroll_rml);
	REQUIRE(document);
	document->Show();

	Run(context);

	Element* scrollable = document->GetElementById("scrollable");
	REQUIRE(scrollable);
	Element* cells[4][4]{};
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			cells[i][j] = document->GetElementById(CreateString("cell%d%d", i, j));
			REQUIRE(cells[i][j]);
		}
	}

	REQUIRE(cells[0][0]->GetAbsoluteOffset(Rml::BoxArea::Border) == Vector2f(0, 0));
	REQUIRE(cells[2][2]->GetAbsoluteOffset(Rml::BoxArea::Border) == Vector2f(100, 100));
	REQUIRE(cells[3][3]->GetAbsoluteOffset(Rml::BoxArea::Border) == Vector2f(150, 150));
	REQUIRE(scrollable->GetScrollLeft() == 0);
	REQUIRE(scrollable->GetScrollTop() == 0);

	SUBCASE("LegacyScroll")
	{
		cells[2][2]->ScrollIntoView(true);

		Run(context);

		CHECK(cells[0][0]->GetAbsoluteOffset(Rml::BoxArea::Border) == Vector2f(-50, -100));
		CHECK(cells[2][2]->GetAbsoluteOffset(Rml::BoxArea::Border) == Vector2f(50, 0));
		CHECK(cells[3][3]->GetAbsoluteOffset(Rml::BoxArea::Border) == Vector2f(100, 50));
		CHECK(scrollable->GetScrollLeft() == 50);
		CHECK(scrollable->GetScrollTop() == 100);

		cells[2][2]->ScrollIntoView(false);

		Run(context);

		CHECK(cells[0][0]->GetAbsoluteOffset(Rml::BoxArea::Border) == Vector2f(-50, -50));
		CHECK(cells[2][2]->GetAbsoluteOffset(Rml::BoxArea::Border) == Vector2f(50, 50));
		CHECK(cells[3][3]->GetAbsoluteOffset(Rml::BoxArea::Border) == Vector2f(100, 100));
		CHECK(scrollable->GetScrollLeft() == 50);
		CHECK(scrollable->GetScrollTop() == 50);
	}

	SUBCASE("AdvancedScroll")
	{
		cells[2][2]->ScrollIntoView({ScrollAlignment::Center, ScrollAlignment::Center});

		Run(context);

		CHECK(cells[0][0]->GetAbsoluteOffset(Rml::BoxArea::Border) == Vector2f(-75, -75));
		CHECK(cells[2][2]->GetAbsoluteOffset(Rml::BoxArea::Border) == Vector2f(25, 25));
		CHECK(cells[3][3]->GetAbsoluteOffset(Rml::BoxArea::Border) == Vector2f(75, 75));

		SUBCASE("NearestAlready")
		{
			cells[2][2]->ScrollIntoView(ScrollAlignment::Nearest);

			Run(context);

			CHECK(cells[0][0]->GetAbsoluteOffset(Rml::BoxArea::Border) == Vector2f(-75, -75));
			CHECK(cells[2][2]->GetAbsoluteOffset(Rml::BoxArea::Border) == Vector2f(25, 25));
			CHECK(cells[3][3]->GetAbsoluteOffset(Rml::BoxArea::Border) == Vector2f(75, 75));
		}

		SUBCASE("NearestBefore")
		{
			cells[1][1]->ScrollIntoView(ScrollAlignment::Nearest);

			Run(context);

			CHECK(cells[0][0]->GetAbsoluteOffset(Rml::BoxArea::Border) == Vector2f(-50, -50));
			CHECK(cells[1][1]->GetAbsoluteOffset(Rml::BoxArea::Border) == Vector2f(0, 0));
			CHECK(cells[2][2]->GetAbsoluteOffset(Rml::BoxArea::Border) == Vector2f(50, 50));
		}

		SUBCASE("NearestAfter")
		{
			cells[3][3]->ScrollIntoView(ScrollAlignment::Nearest);

			Run(context);

			CHECK(cells[1][1]->GetAbsoluteOffset(Rml::BoxArea::Border) == Vector2f(-50, -50));
			CHECK(cells[2][2]->GetAbsoluteOffset(Rml::BoxArea::Border) == Vector2f(0, 0));
			CHECK(cells[3][3]->GetAbsoluteOffset(Rml::BoxArea::Border) == Vector2f(50, 50));
		}

		SUBCASE("Smoothscroll")
		{
			TestsSystemInterface* system_interface = TestsShell::GetTestsSystemInterface();
			system_interface->SetManualTime(0);
			cells[3][3]->ScrollIntoView({ScrollAlignment::Nearest, ScrollAlignment::Nearest, ScrollBehavior::Smooth});

			constexpr double dt = 1.0 / 15.0;
			system_interface->SetManualTime(dt);
			Run(context);

			// We don't define the exact offset at this time step, but it should be somewhere between the start and end offsets.
			Vector2f offset = cells[3][3]->GetAbsoluteOffset(Rml::BoxArea::Border);
			CHECK(offset.x > 50.f);
			CHECK(offset.y > 50.f);
			CHECK(offset.x < 75.f);
			CHECK(offset.y < 75.f);

			// After one second it should be at the destination offset.
			for (double t = 2.0 * dt; t < 1.0; t += dt)
			{
				system_interface->SetManualTime(t);
				Run(context);
			}
			CHECK(cells[3][3]->GetAbsoluteOffset(Rml::BoxArea::Border) == Vector2f(50, 50));
		}
	}

	document->Close();
	TestsShell::ShutdownShell();
}
