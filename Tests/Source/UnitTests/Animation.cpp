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
#include "../Common/TypesToString.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <doctest.h>
#include <float.h>

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
			String document_rml = Rml::CreateString(document_decorator_rml.c_str(), test.from_rule.c_str(), test.to_rule.c_str(), property_str,
				test.from.c_str(), property_str, test.to.c_str());

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
			String document_rml = Rml::CreateString(document_filter_rml.c_str(), property_str, test.from.c_str(), property_str, test.to.c_str());

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

static const String document_multiple_values_rml = R"(
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
		@keyframes fade {
			from { opacity: 0; }
			to { opacity: 1; }
		}
		@keyframes colorize {
			from { background-color: #f00; }
			25% { background-color: #0f0; }
			to { background-color: #00f; }
		}
		div {
			width: 300dp;
			height: 300dp;
			background-color: #000;
			animation: 2s fade, 4s 1s colorize;
		}
	</style>
</head>

<body>
	<div/>
</body>
</rml>
)";

TEST_CASE("animation.multiple_values")
{
	TestsSystemInterface* system_interface = TestsShell::GetTestsSystemInterface();
	Context* context = TestsShell::GetContext();

	const double dt = 0.1;
	const double t_fade_start = 0;
	const double t_colorize_start = 1;
	const double t_fade_final = 2;
	const double t_colorize_final = 5;

	struct TestCase {
		double time;
		float opacity_min;
		Colourb background_color;
	};
	const std::vector<TestCase> tests = {
		{t_fade_start, 0.f, {0, 0, 0}},
		{t_fade_start + dt, 0.01f, {0, 0, 0}},

		{t_colorize_start - dt, 0.4f, {0, 0, 0}},
		{t_colorize_start, 0.49f, {0, 0, 0}},
		{t_colorize_start + dt, 0.5f, {0xf1, 0x50, 0}},

		{t_fade_final - dt, 0.9f, {0x50, 0xf1, 0}},
		{t_fade_final, 0.99f, {0, 0xfe, 0x00}},
		{t_fade_final + dt, 1.0f, {0, 0xfa, 0x2e}},

		{t_colorize_final - dt, 1.0f, {0, 0x2e, 0xfa}},
		{t_colorize_final, 1.0f, {0, 0, 0xfe}},
		{t_colorize_final + dt, 1.0f, {0, 0, 0}},
	};

	{
		system_interface->SetTime(0.0);
		ElementDocument* document = context->LoadDocumentFromMemory(document_multiple_values_rml, "assets/");
		Element* element = document->GetChild(0);

		document->Show();

		double t = 0.f;
		for (const auto& test : tests)
		{
			// Simulate progression in small time steps since animations are constrained to 0.1s maximum step size.
			while (t < test.time)
			{
				t = Math::Min(t + dt, test.time);
				system_interface->SetTime(t);
				context->Update();
			}

			INFO("Time: ", test.time);
			CHECK(element->GetProperty<float>("opacity") >= test.opacity_min);
			CHECK(element->GetProperty<Colourb>("background-color") == test.background_color);
		}

		document->Close();
	}

	system_interface->SetTime(0.0);
	TestsShell::ShutdownShell();
}

static const String document_animation_multiple_values_rml = R"(
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
		@keyframes fadein {
			from {
				opacity: 0;
				visibility: hidden;
			}
			to {
				opacity: 1;
				visibility: visible;
			}
		}
		@keyframes fadeout {
			from {
				opacity: 1;
				visibility: visible;
			}
			to {
				opacity: 0;
				visibility: hidden;
			}
		}

		div {
			background-color: #c66;
			width: 300dp;
			height: 300dp;
			margin: auto;
			animation: 1s fadein, 1s 5s fadeout;
		}
	</style>
</head>

<body>
	<div id="div"/>
</body>
</rml>
)";

TEST_CASE("animation.multiple_overlapping")
{
	TestsSystemInterface* system_interface = TestsShell::GetTestsSystemInterface();
	Context* context = TestsShell::GetContext();

	const float greater_than_zero = FLT_MIN;

	const double dt = 1.0 / 60.0;
	const double t0 = 0;
	const double t_delta = 0.1;
	const double t_fadein_final = 1;
	const double t_fadeout_start = 5;
	const double t_fadeout_final = 6;

	struct TestCase {
		double time;
		float opacity_min;
		bool is_visible;
	};
	const std::vector<TestCase> tests = {
		{t0, 0.f, true},
		{t0 + t_delta, greater_than_zero, true},
		{t_fadein_final - t_delta, 0.8f, true},
		{t_fadein_final, 1.f, true},
		{t_fadein_final + t_delta, 1.f, true},
		{t_fadeout_start - t_delta, 1.f, true},
		{t_fadeout_start, 1.f, true},
		{t_fadeout_start + t_delta, 0.8f, true},
		{t_fadeout_final - t_delta, greater_than_zero, true},
		{t_fadeout_final, 0.f, false},
		{t_fadeout_final + t_delta, 0.f, false},
	};

	// Currently we don't support animating the same property from multiple animations on the same element, see #608.
	// This test only checks that we submit warnings to the user. Once we support this feature, we can enable more checks in this test.
	{
		system_interface->SetNumExpectedWarnings(2);

		system_interface->SetTime(0.0);

		ElementDocument* document = context->LoadDocumentFromMemory(document_animation_multiple_values_rml, "assets/");
		Element* element = document->GetChild(0);

		document->Show();

		TestsShell::RenderLoop();

		double t = 0.f;
		for (const auto& test : tests)
		{
			while (t < test.time)
			{
				t = Math::Min(t + dt, test.time);
				system_interface->SetTime(t);
				context->Update();
			}

			INFO("Time: ", test.time);
			(void)(element->GetProperty<float>("opacity") == test.opacity_min);
			(void)(element->IsVisible() == test.is_visible);
		}

		document->Close();
	}

	system_interface->SetTime(0.0);
	TestsShell::ShutdownShell();
}
