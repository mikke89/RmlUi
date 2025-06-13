/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2019-2024 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

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
