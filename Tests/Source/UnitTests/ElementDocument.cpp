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
#include <algorithm>

using namespace Rml;

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
			for(const String& id : ids)
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

TEST_SUITE_END();
