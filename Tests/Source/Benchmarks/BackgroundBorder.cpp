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
#include <RmlUi/Core/Types.h>

#include <doctest.h>
#include <nanobench.h>

using namespace ankerl;
using namespace Rml;

static String document_rml = R"(
<rml>
<head>
    <link type="text/rcss" href="/../Tests/Data/style.rcss"/>
	<style>
		div > div {
			margin: 50px auto;
			width: 300px;
			height: 200px;
			background: #c3c3c3;
			border-color: #55f #f57 #55f #afa;
		}
		#no-radius > div {
			border-width: 8px 10px;
		}
		#small-radius > div {
			border-width: 40px 20px;
			border-radius: 5px;
		}
		#large-radius > div {
			border-width: 10px 5px 25px 20px;
			border-radius: 80px 30px;
		}
	</style>
</head>

<body>
<div id="no-radius">
	<div/><div/><div/><div/><div/><div/><div/><div/><div/><div/>
</div>
<div id="small-radius">
	<div/><div/><div/><div/><div/><div/><div/><div/><div/><div/>
</div>
<div id="large-radius">
	<div/><div/><div/><div/><div/><div/><div/><div/><div/><div/>
</div>
</body>
</rml>
)";


TEST_CASE("backgrounds_and_borders")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	REQUIRE(document);
	document->Show();

	const String msg = TestsShell::GetRenderStats();
	MESSAGE(msg);

	nanobench::Bench bench;
	bench.title("Backgrounds and borders");
	bench.relative(true);
	bench.minEpochIterations(100);
	bench.warmup(50);

	TestsShell::RenderLoop();

	bench.run("Reference (update + render)", [&] {
		context->Update();
		context->Render();
	});

	{
		ElementList elements;
		document->QuerySelectorAll(elements, "div > div");
		REQUIRE(!elements.empty());

		bench.run("Background all", [&] {
			// Force regeneration of backgrounds without changing layout
			for (auto& element : elements)
				element->SetProperty(Rml::PropertyId::BackgroundColor, Rml::Property(Colourb(), Property::COLOUR));
			context->Update();
			context->Render();
		});

		bench.run("Border all", [&] {
			// Force regeneration of borders without changing layout
			for (auto& element : elements)
				element->SetProperty(Rml::PropertyId::BorderLeftColor, Rml::Property(Colourb(), Property::COLOUR));
			context->Update();
			context->Render();
		});
	}

	for (const String id : {"no-radius", "small-radius", "large-radius"})
	{
		ElementList elements;
		document->QuerySelectorAll(elements, "#" + id + " > div");
		REQUIRE(!elements.empty());

		bench.run("Border " + id, [&] {
			for (auto& element : elements)
				element->SetProperty(Rml::PropertyId::BorderLeftColor, Rml::Property(Colourb(), Property::COLOUR));
			context->Update();
			context->Render();
		});
	}

	document->Close();
}
