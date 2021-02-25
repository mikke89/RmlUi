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
	<link type="text/rcss" href="/../Tests/Data/UnitTests/Specificity_MediaQuery.rcss"/>
	<style>
		body {
			width: 50px;
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
			width: 50px;
		}
	</style>
	<link type="text/rcss" href="/../Tests/Data/UnitTests/Specificity_MediaQuery.rcss"/>
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
			width: 50px;
		}
	</style>
	<link type="text/rcss" href="/../Tests/Data/UnitTests/Specificity_MediaQuery.rcss"/>
</head>
<body class="narrow"/>
</rml>
)";
static const String simple_doc4_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/../Tests/Data/UnitTests/Specificity_MediaQuery.rcss"/>
	<style>
		body { width: 200px; }
		@media (min-width: 100px) {
			body { width: 50px; }
		}
	</style>
</head>
<body/>
</rml>
)";
static const String simple_doc5_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/../Tests/Data/UnitTests/Specificity_MediaQuery.rcss"/>
	<style>
		@media (min-width: 100px) {
			body { width: 50px; }
		}
		body { width: 200px; }
	</style>
</head>
<body/>
</rml>
)";
static const String simple_doc6_rml = R"(
<rml>
<head>
	<title>Test</title>
	<style>
		body { width: 200px; }
		@media (min-width: 100px) {
			body { width: 50px; }
		}
	</style>
	<link type="text/rcss" href="/../Tests/Data/UnitTests/Specificity_MediaQuery.rcss"/>
</head>
<body/>
</rml>
)";
static const String simple_doc7_rml = R"(
<rml>
<head>
	<title>Test</title>
	<style>
		@media (min-width: 100px) {
			body { width: 50px; }
		}
		body { width: 200px; }
	</style>
	<link type="text/rcss" href="/../Tests/Data/UnitTests/Specificity_MediaQuery.rcss"/>
</head>
<body/>
</rml>
)";
static const String simple_doc8_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/../Tests/Data/UnitTests/Specificity_MediaQuery.rcss"/>
	<style>
		@media (min-width: 100px) {
			body { width: 50px; }
		}
		body { width: 200px; }
	</style>
</head>
<body/>
</rml>
)";
static const String simple_doc9_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/../Tests/Data/UnitTests/Specificity_MediaQuery.rcss"/>
	<style>
		@media (min-width: 100px) {
			body { width: 50px; }
		}
		body { width: 200px; }
	</style>
	<style>
		body { width: 300px; }
		@media (min-width: 100px) {
			body { width: 400px; }
		}
	</style>
</head>
<body/>
</rml>
)";
static const String simple_doc10_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/../Tests/Data/UnitTests/Specificity_MediaQuery.rcss"/>
	<style>
		@media (min-width: 100px) {
			body { width: 50px; }
		}
		body { width: 200px; }
	</style>
	<style>
		@media (min-width: 100px) {
			body { width: 400px; }
		}
		body { width: 300px; }
	</style>
</head>
<body/>
</rml>
)";
static const String simple_doc11_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/../Tests/Data/UnitTests/Specificity_MediaQuery.rcss"/>
	<style>
		@media (min-width: 100px) {
			body { width: 50px; }
		}
		body { width: 200px; }
	</style>
	<style>
		body { width: 300px; }
		@media (min-width: 200px) {
			body { width: 400px; }
		}
	</style>
</head>
<body/>
</rml>
)";
static const String simple_doc12_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/../Tests/Data/UnitTests/Specificity_MediaQuery.rcss"/>
	<style>
		@media (min-width: 100px) {
			body { width: 50px; }
		}
		body { width: 200px; }
	</style>
	<style>
		@media (min-width: 200px) {
			body { width: 400px; }
		}
		body { width: 300px; }
	</style>
</head>
<body/>
</rml>
)";
static const String simple_doc13_rml = R"(
<rml>
<head>
	<title>Test</title>
	<style>
		@media (min-width: 100px) {
			body { width: 50px; }
		}
		body { width: 200px; }
	</style>
	<link type="text/rcss" href="/../Tests/Data/UnitTests/Specificity_MediaQuery.rcss"/>
	<style>
		@media (min-width: 200px) {
			body { width: 400px; }
		}
		body { width: 300px; }
	</style>
</head>
<body/>
</rml>
)";
static const String simple_doc14_rml = R"(
<rml>
<head>
	<title>Test</title>
	<style>
		@media (min-width: 100px) {
			body { width: 50px; }
		}
		body { width: 200px; }
	</style>
	<style>
		@media (min-width: 200px) {
			body { width: 400px; }
		}
		body { width: 300px; }
	</style>
	<link type="text/rcss" href="/../Tests/Data/UnitTests/Specificity_MediaQuery.rcss"/>
</head>
<body/>
</rml>
)";


TEST_CASE("specificity.mediaquery")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	struct Test {
		const String* document_rml;
		float expected_width;
	};

	Test tests[] = {
		{&simple_doc1_rml, 50.f},
		{&simple_doc2_rml, 100.f},
		{&simple_doc3_rml, 50.f},
		{&simple_doc4_rml, 50.f},
		{&simple_doc5_rml, 200.f},
		{&simple_doc6_rml, 100.f},
		{&simple_doc7_rml, 100.f},
		{&simple_doc8_rml, 200.f},
		{&simple_doc9_rml, 400.f},
		{&simple_doc10_rml, 300.f},
		{&simple_doc11_rml, 400.f},
		{&simple_doc12_rml, 300.f},
		{&simple_doc13_rml, 300.f},
		{&simple_doc14_rml, 100.f},
	};

	int i = 1;
	for (const Test& test : tests)
	{
		ElementDocument* document = context->LoadDocumentFromMemory(*test.document_rml);
		REQUIRE(document);
		document->Show();

		context->Update();
		context->Render();

		TestsShell::RenderLoop();

		CHECK_MESSAGE(document->GetBox().GetSize().x == test.expected_width, "Document " << i);

		document->Close();
		i++;
	}

	TestsShell::ShutdownShell();
}
