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
#include <RmlUi/Core/ComputedValues.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/ElementText.h>
#include <RmlUi/Core/Factory.h>
#include <doctest.h>

using namespace Rml;

static const String document_xml_tags_in_css = R"(
<rml>
    <head>
        <style>
            body {
                /* <body> */
                width: 200px;
                height: 200px;
                background-color: #00ff00;
            }
        </style>
    </head>
    <body>
    </body>
</rml>
)";

static const String document_escaping = R"(
<rml>
    <head>
	<style>
	p { 
		font-family: LatoLatin;
	}
	</style>
    </head>
    <body>
	<p id="p">&#x20AC;&#8364;</p>
    </body>
</rml>
)";

static const String document_escaping_tags = R"(
<rml>
    <head>
	<style>
	* { 
		font-family: LatoLatin;
	}
	</style>
    </head>
    <body>&lt;p&gt;&amp;lt;span/&amp;gt;&lt;/p&gt;</body>
</rml>
)";

TEST_CASE("XMLParser")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	// Style nodes should accept XML reserved characters, see https://github.com/mikke89/RmlUi/issues/341

	ElementDocument* document = context->LoadDocumentFromMemory(document_xml_tags_in_css);
	REQUIRE(document);
	document->Show();

	TestsShell::RenderLoop();

	const Colourb background = document->GetComputedValues().background_color();
	CHECK(background.red == 0);
	CHECK(background.green == 0xff);
	CHECK(background.blue == 0);
	CHECK(background.alpha == 0xff);

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("XMLParser.escaping")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_escaping);
	REQUIRE(document);
	document->Show();

	TestsShell::RenderLoop();

	auto element = document->GetElementById("p");
	REQUIRE(element);

	CHECK(element->GetInnerRML() == "\xe2\x82\xac\xe2\x82\xac");

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("XMLParser.escaping_tags")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_escaping_tags);
	REQUIRE(document);
	document->Show();

	TestsShell::RenderLoop();

	CHECK(document->GetNumChildren() == 1);
	CHECK(document->GetFirstChild()->GetTagName() == "#text");
	// Text-access should yield decoded value, while RML-access should yield encoded value
	CHECK(static_cast<ElementText*>(document->GetFirstChild())->GetText() == "<p>&lt;span/&gt;</p>");
	CHECK(document->GetInnerRML() == "&lt;p&gt;&amp;lt;span/&amp;gt;&lt;/p&gt;");

	document->Close();
	TestsShell::ShutdownShell();
}
