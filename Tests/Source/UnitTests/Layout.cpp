/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
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
