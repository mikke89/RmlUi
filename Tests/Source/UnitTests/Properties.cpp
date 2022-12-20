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

#include "../Common/TestsShell.h"
#include "../Common/TestsInterface.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <doctest.h>

using namespace Rml;

TEST_CASE("Properties")
{
	const Vector2i window_size(1024, 768);

	TestsSystemInterface system_interface;
	TestsRenderInterface render_interface;

	SetRenderInterface(&render_interface);
	SetSystemInterface(&system_interface);

	Rml::Initialise();

	Context* context = Rml::CreateContext("main", window_size);
	ElementDocument* document = context->CreateDocument();

	struct FlexTestCase {
		String flex_value;

		struct ExpectedValues {
			float flex_grow;
			float flex_shrink;
			String flex_basis;
		} expected;
	};

	FlexTestCase tests[] = {
		{"", {0.f, 1.f, "auto"}},
		{"none", {0.f, 0.f, "auto"}},
		{"auto", {1.f, 1.f, "auto"}},
		{"1", {1.f, 1.f, "0px"}},
		{"2", {2.f, 1.f, "0px"}},
		{"2 0", {2.f, 0.f, "0px"}},
		{"2 3", {2.f, 3.f, "0px"}},
		{"2 auto", {2.f, 1.f, "auto"}},
		{"2 0 auto", {2.f, 0.f, "auto"}},
		{"0 0 auto", {0.f, 0.f, "auto"}},
		{"0 0 50px", {0.f, 0.f, "50px"}},
		{"0 0 50px", {0.f, 0.f, "50px"}},
		{"0 0 0", {0.f, 0.f, "0px"}},
	};

	for (const FlexTestCase& test : tests)
	{
		if (!test.flex_value.empty())
		{
			CHECK(document->SetProperty("flex", test.flex_value));
		}

		CHECK(document->GetProperty<float>("flex-grow") == test.expected.flex_grow);
		CHECK(document->GetProperty<float>("flex-shrink") == test.expected.flex_shrink);
		CHECK(document->GetProperty("flex-basis")->ToString() == test.expected.flex_basis);
	}

	Rml::Shutdown();
}

static const String document_rml = R"(
<rml>
<head>
	<style>
	body {
		--color-var: #ffffff;
	}
	div {
		
			background-color: var(--color-var);
	}
		/*body {
			--color-var: #ffffff;
			--font-var: 20px bold serif;
			--padding-var: 4px 5px;
			--recursive-border-var: 5px var(--color-var);
		}

		div {
			width: var(--missing-var, 500px);
			background-color: var(--color-var);
			padding: var(--padding-var);
			font: var(--font-var);
		}

		p {
			border: var(--recursive-border-var);
		}*/
	</style>
</head>

<body>
<div id="div"></div>
</body>
</rml>
)";

TEST_CASE("Variables")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	REQUIRE(document);
	document->Show();

	TestsShell::RenderLoop();

	Element* element = document->GetElementById("div");
	CHECK(element->GetProperty(PropertyId::BackgroundColor)->ToString() == "rgba(255,255,255,255)");

	document->SetVariable("color-var", "#000000");

	TestsShell::RenderLoop();

	CHECK(element->GetProperty(PropertyId::BackgroundColor)->ToString() == "rgba(0,0,0,255)");

	document->Close();

	TestsShell::ShutdownShell();
}
