/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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

#include "../Common/Mocks.h"
#include "../Common/TestsShell.h"
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
			background: #ccc;
			height: 100px;
		}
		span {
			color: red;
		}
	</style>
</head>

<body>
<div>This is a <span>sample</span>.</div>
</body>
</rml>
)";

TEST_CASE("Element")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_clone_rml);
	REQUIRE(document);
	document->Show();

	context->Update();
	context->Render();

	TestsShell::RenderLoop();
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
			const auto configureMockEventListener = [&]()
			{
				mockEventListener.reset(new MockEventListener());
				expectations.emplace_back(NAMED_ALLOW_CALL(*mockEventListener, OnAttach(button)));
				expectations.emplace_back(NAMED_ALLOW_CALL(*mockEventListener, OnDetach(button))
					.LR_SIDE_EFFECT(mockEventListener.reset()));
			};

			MockEventListenerInstancer mockEventListenerInstancer;
			const auto configureMockEventListenerInstancer = [&](const auto value)
			{
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

	SUBCASE("Clone")
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

	document->Close();
	TestsShell::ShutdownShell();
}
