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
#include <RmlUi/Core/Factory.h>
#include <RmlUi/Core/StringUtilities.h>
#include <RmlUi/Core/Types.h>
#include <doctest.h>
#include <nanobench.h>

using namespace ankerl;
using namespace Rml;

static const String document_rml = R"(
<rml>
<head>
	<link type="text/template" href="/assets/window.rml"/>
	<title>Benchmark Sample</title>
	<style>
		body.window
		{
			left: 50px;
			top: 50px;
			width: 800px;
			height: 200px;
		}
		#performance
		{
			width: 500px;
			height: 300px;
		}
	</style>
</head>

<body template="window">
<div id="performance">
	<div class="row">
		<div class="col col1"><button class="expand" index="3">+</button>&nbsp;<a>Route 15</a></div>
		<div class="col col23"><input type="range" class="assign_range" min="0" max="20" value="3"/></div>
		<div class="col col4">Assigned</div>
		<select>
			<option>Red</option><option>Blue</option><option selected>Green</option><option style="background-color: yellow;">Yellow</option>
		</select>
		<div class="inrow unmark_collapse">
			<div class="col col123 assign_text">Assign to route</div>
			<div class="col col4">
				<input type="submit" class="vehicle_depot_assign_confirm" quantity="0">Confirm</input>
			</div>
		</div>
	</div>
</div>
</body>
</rml>
)";

TEST_CASE("elementdocument")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	std::string title = "ElementDocument";
	String modified_document_rml = document_rml;
	bool clear_style_sheet_cache = false;

	SUBCASE("ElementDocument")
	{
		clear_style_sheet_cache = false;
	}
	SUBCASE("ElementDocument w/o Layout Cache")
	{
		title += " w/o Layout Cache";
		clear_style_sheet_cache = false;
		modified_document_rml = StringUtilities::Replace(document_rml, "<body", "<body rmlui-disable-layout-cache");
	}
	SUBCASE("ElementDocument w/o Style Sheet Cache")
	{
		title += " w/o Style Sheet Cache";
		clear_style_sheet_cache = true;
	}

	auto ConditionallyClearStyleSheetCache = [&]() {
		if (clear_style_sheet_cache)
			Factory::ClearStyleSheetCache();
	};

	nanobench::Bench bench;
	bench.title(title);
	bench.timeUnit(std::chrono::microseconds(1), "us");
	bench.relative(true);
	bench.warmup(10);

	bench.run("LoadDocument", [&] {
		ConditionallyClearStyleSheetCache();
		ElementDocument* document = context->LoadDocumentFromMemory(modified_document_rml);
		document->Close();
		context->Update();
	});

	bench.run("LoadDocument + Show", [&] {
		ConditionallyClearStyleSheetCache();
		ElementDocument* document = context->LoadDocumentFromMemory(modified_document_rml);
		document->Show();
		document->Close();
		context->Update();
	});

	bench.run("LoadDocument + Show + Update", [&] {
		ConditionallyClearStyleSheetCache();
		ElementDocument* document = context->LoadDocumentFromMemory(modified_document_rml);
		document->Show();
		context->Update();
		document->Close();
		context->Update();
	});

	bench.run("LoadDocument + Show + Update + Render", [&] {
		ConditionallyClearStyleSheetCache();
		ElementDocument* document = context->LoadDocumentFromMemory(modified_document_rml);
		document->Show();
		context->Update();
		TestsShell::BeginFrame();
		context->Render();
		TestsShell::PresentFrame();
		document->Close();
		context->Update();
	});
}

TEST_CASE("elementdocument.render_stats")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	document->Show();

	const String stats = TestsShell::GetRenderStats();
	MESSAGE(stats);

	TestsShell::RenderLoop();
	document->Close();
}

static const String document_isolation_rml = R"(
<rml>
<head>
    <link type="text/rcss" href="/assets/rml.rcss"/>
    <style>
        body {
            width: 800px;
            height: 600px;
            background-color: #ddd;
			font-family: LatoLatin;
			font-size: 14px;
        }
		scrollbarvertical {
			width: 12px;
			cursor: arrow;
			margin-right: -1px;
			padding-right: 1px;
		}
		scrollbarvertical slidertrack {
			background-color: #f0f0f0;
		}
		scrollbarvertical sliderbar {
			background-color: #666;
		}
        .container {
            width: 200px;
            margin: 20px;
            padding: 10px;
            background-color: #bbb;
        }
        .container > div {
            background-color: #aaa;
            margin: 5px;
            padding: 10px;
        }
        #overflow-container {
            overflow: scroll;
            height: 150px;
        }
        #absolute-container {
            position: absolute;
            top: 300px;
            left: 300px;
			max-height: 300px;
			overflow: scroll;
        }
    </style>
</head>
<body %s>
    <div class="container" id="overflow-container">
        <div id="overflow-item">Overflow item</div>
        %s
    </div>

    <div class="container" id="absolute-container">
        <div id="absolute-item">Absolute item</div>
		%s
    </div>

    <div class="container" id="inflow-container">
        <div id="inflow-item">Inflow item</div>
		%s
    </div>
</body>
</rml>
)";

TEST_CASE("elementdocument.layout_cache")
{
	Context* context = TestsShell::GetContext();

	constexpr int num_items = 20;
	auto CreateIsolationDocument = [&](bool disable_layout_cache) {
		return CreateString(document_isolation_rml.c_str(), disable_layout_cache ? "rmlui-disable-layout-cache" : "",
			StringUtilities::RepeatString("<div>Item text</div>", num_items).c_str(),
			StringUtilities::RepeatString("<div>Item text</div>", num_items).c_str(),
			StringUtilities::RepeatString("<div>Item text</div>", num_items).c_str());
	};

	{
		ElementDocument* document = context->LoadDocumentFromMemory(CreateIsolationDocument(false));
		document->Show();
		TestsShell::RenderLoop();
		document->Close();
		context->Update();
	}

	for (const String& modify_item_id : {"overflow-item", "absolute-item", "inflow-item"})
	{
		nanobench::Bench bench;
		bench.title("Layout cache (" + modify_item_id + ")");
		bench.timeUnit(std::chrono::microseconds(1), "us");
		bench.relative(true);

		{
			ElementDocument* document = context->LoadDocumentFromMemory(CreateIsolationDocument(false));
			document->Show();
			context->Update();
			Element* element = document->GetElementById(modify_item_id);
			bench.run("Enabled layout cache", [&] {
				element->SetInnerRML("Changed item");
				context->Update();
			});
			document->Close();
			context->Update();
		}

		{
			ElementDocument* document = context->LoadDocumentFromMemory(CreateIsolationDocument(true));
			document->Show();
			context->Update();
			Element* element = document->GetElementById(modify_item_id);
			bench.run("Disabled layout cache", [&] {
				element->SetInnerRML("Changed item");
				context->Update();
			});
			document->Close();
			context->Update();
		}
	}

	TestsShell::ShutdownShell();
}
