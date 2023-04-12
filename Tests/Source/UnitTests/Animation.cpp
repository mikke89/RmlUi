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

#include "../Common/TestsInterface.h"
#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
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
		
		@decorator from_rule : gradient { %s }
		@decorator to_rule: gradient{ %s }		

		@keyframes mix {
			from { decorator: %s; }
			to   { decorator: %s; }
		}
		div {
			background: #333;
			height: 64px;
			width: 64px;
			animation: mix 0.1s;
		}
	</style>
</head>

<body>
	<div/>
</body>
</rml>
)";

TEST_CASE("animation.decorator")
{
	struct Test {
		String from_rule;
		String to_rule;
		String from;
		String to;
		String expected_25p; // expected interpolated value at 25% progression
	};

	Vector<Test> tests{
		// Only standard declaration
		{
			"",
			"",

			"gradient(horizontal transparent transparent)",
			"gradient(horizontal white white)",

			"gradient(horizontal rgba(127,127,127,63) rgba(127,127,127,63))",
		},
		{
			"",
			"",

			"none",
			"gradient(horizontal transparent transparent)",

			"gradient(horizontal rgba(220,220,220,191) rgba(220,220,220,191))",
		},
		{
			"",
			"",

			"none",
			"gradient(horizontal transparent transparent), gradient(vertical transparent transparent)",

			"gradient(horizontal rgba(220,220,220,191) rgba(220,220,220,191)), gradient(horizontal rgba(220,220,220,191) rgba(220,220,220,191))",
		},
		{
			"",
			"",

			"gradient(horizontal transparent transparent), gradient(vertical transparent transparent)",
			"none",

			"gradient(horizontal rgba(127,127,127,63) rgba(127,127,127,63)), gradient(vertical rgba(127,127,127,63) rgba(127,127,127,63))",
		},

		/// Only rule declaration
		{
			"direction: horizontal; start-color: transparent; stop-color: transparent;",
			"direction: horizontal; start-color: white; stop-color: white;",

			"from_rule",
			"to_rule",

			"gradient(horizontal rgba(127,127,127,63) rgba(127,127,127,63))",
		},
		{
			"",
			"direction: horizontal; start-color: transparent; stop-color: transparent;",

			"from_rule",
			"to_rule",

			"gradient(horizontal rgba(220,220,220,191) rgba(220,220,220,191))",
		},
		{
			"direction: vertical; start-color: transparent; stop-color: transparent;",
			"",

			"from_rule",
			"to_rule",

			"gradient(vertical rgba(127,127,127,63) rgba(127,127,127,63))",
		},

		/// Mix rule and standard declaration
		{
			"direction: horizontal; start-color: transparent; stop-color: transparent;",
			"",

			"from_rule",
			"gradient(horizontal white white)",

			"gradient(horizontal rgba(127,127,127,63) rgba(127,127,127,63))",
		},
		{
			"",
			"direction: horizontal; start-color: transparent; stop-color: transparent;",

			"none",
			"to_rule",

			"gradient(horizontal rgba(220,220,220,191) rgba(220,220,220,191))",
		},
		{
			"direction: vertical; start-color: transparent; stop-color: transparent;",
			"",

			"from_rule",
			"none",

			"gradient(vertical rgba(127,127,127,63) rgba(127,127,127,63))",
		},
		{
			"",
			"",

			"from_rule, to_rule",
			"gradient(horizontal transparent transparent), gradient(vertical transparent transparent)",

			"gradient(horizontal rgba(220,220,220,191) rgba(220,220,220,191)), gradient(horizontal rgba(220,220,220,191) rgba(220,220,220,191))",
		},
		{
			"",
			"",

			"gradient(horizontal transparent transparent), gradient(vertical transparent transparent)",
			"from_rule, to_rule",

			"gradient(horizontal rgba(127,127,127,63) rgba(127,127,127,63)), gradient(vertical rgba(127,127,127,63) rgba(127,127,127,63))",
		},
	};

	TestsSystemInterface* system_interface = TestsShell::GetTestsSystemInterface();
	Context* context = TestsShell::GetContext();
	context->SetDensityIndependentPixelRatio(2.0f);

	for (const Test& test : tests)
	{
		const double t_final = 0.1;

		system_interface->SetTime(0.0);
		String document_rml = Rml::CreateString(document_decorator_rml.size() + 512, document_decorator_rml.c_str(), test.from_rule.c_str(),
			test.to_rule.c_str(), test.from.c_str(), test.to.c_str());

		ElementDocument* document = context->LoadDocumentFromMemory(document_rml, "assets/");
		Element* element = document->GetChild(0);

		document->Show();
		TestsShell::RenderLoop();

		system_interface->SetTime(0.25 * t_final);
		TestsShell::RenderLoop();
		CHECK_MESSAGE(element->GetProperty<String>("decorator") == test.expected_25p, "from: ", test.from, ", to: ", test.to);

		document->Close();
	}

	system_interface->SetTime(0.0);

	TestsShell::ShutdownShell();
}
