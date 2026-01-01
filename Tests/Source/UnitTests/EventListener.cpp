#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/EventListenerInstancer.h>
#include <RmlUi/Core/Factory.h>
#include <doctest.h>

using namespace Rml;

static const String document_decorator_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body {
			left: 0;
			top: 0;
			right: 0;
			bottom: 0;
		}
		div, button {
			display: block;
			background: #333;
			height: 64px;
			width: 64px;
		}
	</style>
</head>

<body>
<div>
	<button id="exit" onclick="exit" />
</div>
</body>
</rml>
)";

class DemoEventListenerInstancer : public Rml::EventListenerInstancer {
public:
	Rml::EventListener* InstanceEventListener(const Rml::String& value, Rml::Element* element) override;
	bool has_exited = false;
};

class DemoEventListener : public Rml::EventListener {
public:
	DemoEventListener(const Rml::String& value, Rml::Element* element, DemoEventListenerInstancer* instancer) :
		value(value), element(element), instancer(instancer)
	{}

	void ProcessEvent(Rml::Event& /*event*/) override
	{
		if (value == "exit")
		{
			// Test replacing the current element. Need to be careful with regard to lifetime issues. The event's
			// current element will be destroyed, so we cannot use it after SetInnerRml(). The library should handle
			// this case safely internally when propagating the event further.
			Element* parent = element->GetParentNode();
			parent->SetInnerRML("<button onclick='confirm_exit' onmouseout='cancel_exit' />");
			if (Element* child = parent->GetChild(0))
				child->Focus();
		}
		else if (value == "confirm_exit")
		{
			instancer->has_exited = true;
		}
		else if (value == "cancel_exit")
		{
			if (Element* parent = element->GetParentNode())
				parent->SetInnerRML("<button id='exit' onclick='exit' />");
		}
	}

	void OnDetach(Rml::Element* /*element*/) override { delete this; }

private:
	Rml::String value;
	Rml::Element* element;
	DemoEventListenerInstancer* instancer;
};

Rml::EventListener* DemoEventListenerInstancer::InstanceEventListener(const Rml::String& value, Rml::Element* element)
{
	return new DemoEventListener(value, element, this);
}

TEST_CASE("event_listener.replace_current_element")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	DemoEventListenerInstancer event_listener_instancer;
	Rml::Factory::RegisterEventListenerInstancer(&event_listener_instancer);

	ElementDocument* document = context->LoadDocumentFromMemory(document_decorator_rml, "assets/");
	REQUIRE(document);
	document->Show();
	TestsShell::RenderLoop();

	context->ProcessMouseMove(32, 32, 0);
	TestsShell::RenderLoop();

	context->ProcessMouseButtonDown(0, 0);
	TestsShell::RenderLoop();

	context->ProcessMouseButtonUp(0, 0);
	TestsShell::RenderLoop();

	bool exit_cancelled = false;

	SUBCASE("move_mouse_outside_button")
	{
		context->ProcessMouseMove(100, 100, 0);
		TestsShell::RenderLoop();
		exit_cancelled = true;
	}
	SUBCASE("move_mouse_within_button")
	{
		context->ProcessMouseMove(33, 33, 0);
		TestsShell::RenderLoop();
	}

	context->ProcessMouseButtonDown(0, 0);
	TestsShell::RenderLoop();

	context->ProcessMouseButtonUp(0, 0);
	TestsShell::RenderLoop();

	CHECK(event_listener_instancer.has_exited == !exit_cancelled);

	document->Close();
	Rml::Factory::RegisterEventListenerInstancer(nullptr);
	TestsShell::ShutdownShell();
}
