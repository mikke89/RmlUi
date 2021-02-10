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
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <doctest.h>

using namespace Rml;

static const String simple_doc1_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/../Tests/Data/UnitTests/test.rcss"/>
	<style>
		body {
			width: 48px;
		}
	</style>
</head>
<body/>
</rml>
)";
static const String simple_doc2_rml = R"(
<rml>
<head>
	<title>Test</title>
	<style>
		body {
			width: 48px;
		}
	</style>
	<link type="text/rcss" href="/../Tests/Data/UnitTests/test.rcss"/>
</head>
<body/>
</rml>
)";
static const String simple_doc3_rml = R"(
<rml>
<head>
	<title>Test</title>
	<style>
		body.narrow {
			width: 48px;
		}
	</style>
	<link type="text/rcss" href="/../Tests/Data/UnitTests/test.rcss"/>
</head>
<body class="narrow"/>
</rml>
)";


TEST_CASE("stylesheet.override_basic")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	struct Test {
		const String* document_rml;
		float expected_width;
	};

	Test tests[] = {
		{&simple_doc1_rml, 48.f},
		{&simple_doc2_rml, 100.f},
		{&simple_doc3_rml, 48.f},
	};

	for (const Test& test : tests)
	{
		ElementDocument* document = context->LoadDocumentFromMemory(*test.document_rml);
		REQUIRE(document);
		document->Show();

		context->Update();
		context->Render();

		TestsShell::RenderLoop();

		CHECK(document->GetBox().GetSize().x == test.expected_width);

		document->Close();
	}

	TestsShell::ShutdownShell();
}
