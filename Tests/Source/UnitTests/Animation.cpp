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
		
		@decorator from_rule : horizontal-gradient { %s }
		@decorator to_rule: horizontal-gradient{ %s }

		@keyframes mix {
			from { %s: %s; }
			to   { %s: %s; }
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

			"horizontal-gradient(transparent transparent)",
			"horizontal-gradient(white white)",

			"horizontal-gradient(horizontal #7f7f7f3f #7f7f7f3f)",
		},
		{
			"",
			"",

			"horizontal-gradient(transparent transparent) border-box",
			"horizontal-gradient(white white) border-box",

			"horizontal-gradient(horizontal #7f7f7f3f #7f7f7f3f) border-box",
		},
		{
			"",
			"",

			"none",
			"horizontal-gradient(transparent transparent)",

			"horizontal-gradient(horizontal #dcdcdcbf #dcdcdcbf)",
		},
		{
			"",
			"",

			"none",
			"horizontal-gradient(transparent transparent), horizontal-gradient(transparent transparent)",

			"horizontal-gradient(horizontal #dcdcdcbf #dcdcdcbf), horizontal-gradient(horizontal #dcdcdcbf #dcdcdcbf)",
		},
		{
			"",
			"",

			"horizontal-gradient(transparent transparent), horizontal-gradient(transparent transparent)",
			"none",

			"horizontal-gradient(horizontal #7f7f7f3f #7f7f7f3f), horizontal-gradient(horizontal #7f7f7f3f #7f7f7f3f)",
		},

		/// Only rule declaration
		{
			"start-color: transparent; stop-color: transparent;",
			"start-color: white; stop-color: white;",

			"from_rule",
			"to_rule",

			"horizontal-gradient(horizontal #7f7f7f3f #7f7f7f3f)",
		},
		{
			"",
			"start-color: transparent; stop-color: transparent;",

			"from_rule",
			"to_rule",

			"horizontal-gradient(horizontal #dcdcdcbf #dcdcdcbf)",
		},
		{
			"start-color: transparent; stop-color: transparent;",
			"",

			"from_rule",
			"to_rule",

			"horizontal-gradient(horizontal #7f7f7f3f #7f7f7f3f)",
		},

		/// Mix rule and standard declaration
		{
			"start-color: transparent; stop-color: transparent;",
			"",

			"from_rule",
			"horizontal-gradient(white white)",

			"horizontal-gradient(horizontal #7f7f7f3f #7f7f7f3f)",
		},
		{
			"",
			"start-color: transparent; stop-color: transparent;",

			"none",
			"to_rule",

			"horizontal-gradient(horizontal #dcdcdcbf #dcdcdcbf)",
		},
		{
			"start-color: transparent; stop-color: transparent;",
			"",

			"from_rule",
			"none",

			"horizontal-gradient(horizontal #7f7f7f3f #7f7f7f3f)",
		},
		{
			"",
			"",

			"from_rule, to_rule",
			"horizontal-gradient(transparent transparent), horizontal-gradient(transparent transparent)",

			"horizontal-gradient(horizontal #dcdcdcbf #dcdcdcbf), horizontal-gradient(horizontal #dcdcdcbf #dcdcdcbf)",
		},
		{
			"",
			"",

			"horizontal-gradient(transparent transparent), horizontal-gradient(transparent transparent)",
			"from_rule, to_rule",

			"horizontal-gradient(horizontal #7f7f7f3f #7f7f7f3f), horizontal-gradient(horizontal #7f7f7f3f #7f7f7f3f)",
		},
	};

	TestsSystemInterface* system_interface = TestsShell::GetTestsSystemInterface();
	Context* context = TestsShell::GetContext();

	for (const char* property_str : {"decorator", "mask-image"})
	{
		for (const Test& test : tests)
		{
			const double t_final = 0.1;

			system_interface->SetTime(0.0);
			String document_rml = Rml::CreateString(document_decorator_rml.size() + 512, document_decorator_rml.c_str(), test.from_rule.c_str(),
				test.to_rule.c_str(), property_str, test.from.c_str(), property_str, test.to.c_str());

			ElementDocument* document = context->LoadDocumentFromMemory(document_rml, "assets/");
			Element* element = document->GetChild(0);

			document->Show();
			TestsShell::RenderLoop();

			system_interface->SetTime(0.25 * t_final);
			TestsShell::RenderLoop();
			CHECK_MESSAGE(element->GetProperty<String>(property_str) == test.expected_25p, property_str, " from: ", test.from, ", to: ", test.to);

			document->Close();
		}
	}

	system_interface->SetTime(0.0);

	TestsShell::ShutdownShell();
}

static const String document_filter_rml = R"(
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
		@keyframes mix {
			from { %s: %s; }
			to   { %s: %s; }
		}
		div {
			background: #333;
			height: 64px;
			width: 64px;
			decorator: image(high_scores_alien_1.tga);
			animation: mix 0.1s;
		}
	</style>
</head>

<body>
	<div/>
</body>
</rml>
)";

TEST_CASE("animation.filter")
{
	struct Test {
		String from;
		String to;
		String expected_25p; // expected interpolated value at 25% progression
	};

	Vector<Test> tests{
		{
			"blur( 0px)",
			"blur(40px)",
			"blur(10px)",
		},
		{
			"blur(10px)",
			"blur(25dp)", // assumes dp-ratio == 2
			"blur(20px)",
		},
		{
			"blur(40px)",
			"none",
			"blur(30px)",
		},
		{
			"none",
			"blur(40px)",
			"blur(10px)",
		},
		{
			"drop-shadow(#000 30px 20px 0px)",
			"drop-shadow(#f00 30px 20px 4px)", // colors interpolated in linear space
			"drop-shadow(#7f0000 30px 20px 1px)",
		},
		{
			"opacity(0) brightness(2)",
			"none",
			"opacity(0.25) brightness(1.75)",
		},
		{
			"opacity(0) brightness(0)",
			"opacity(0.5)",
			"opacity(0.125) brightness(0.25)",
		},
		{
			"opacity(0.5)",
			"opacity(0) brightness(0)",
			"opacity(0.375) brightness(0.75)",
		},
		{
			"opacity(0) brightness(0)",
			"brightness(1) opacity(0.5)", // discrete interpolation due to non-matching types
			"opacity(0) brightness(0)",
		},
		{
			"none", // Test initial values of various filters.
			"brightness(2.00) contrast(2.00) grayscale(1.00) hue-rotate(4rad) invert(1.00) opacity(0.00) sepia(1.00) saturate(2.00)",
			"brightness(1.25) contrast(1.25) grayscale(0.25) hue-rotate(1rad) invert(0.25) opacity(0.75) sepia(0.25) saturate(1.25)",
		},
	};

	TestsSystemInterface* system_interface = TestsShell::GetTestsSystemInterface();
	Context* context = TestsShell::GetContext();
	context->SetDensityIndependentPixelRatio(2.0f);

	for (const char* property_str : {"filter", "backdrop-filter"})
	{
		for (const Test& test : tests)
		{
			const double t_final = 0.1;

			system_interface->SetTime(0.0);
			String document_rml = Rml::CreateString(document_filter_rml.size() + 512, document_filter_rml.c_str(), property_str, test.from.c_str(),
				property_str, test.to.c_str());

			ElementDocument* document = context->LoadDocumentFromMemory(document_rml, "assets/");
			Element* element = document->GetChild(0);

			document->Show();

			system_interface->SetTime(0.25 * t_final);
			TestsShell::RenderLoop();

			CHECK_MESSAGE(element->GetProperty<String>(property_str) == test.expected_25p, property_str, " from: ", test.from, ", to: ", test.to);

			document->Close();
		}
	}

	system_interface->SetTime(0.0);

	TestsShell::ShutdownShell();
}
