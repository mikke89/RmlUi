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
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <algorithm>
#include <doctest.h>

using namespace Rml;

TEST_CASE("template.body")
{
	static const String document_rml = R"(
<rml>
<head>
	<link type="text/template" href="/assets/window.rml"/>
	<style>
		body.window
		{
			top: 100px;
			left: 200px;
			width: 600px;
			height: 450px;
		}
	</style>
</head>

<body id="body" class="overridden" template="window">
	<p id="p">A paragraph</p>
</body>
</rml>
)";

	static const String p_address = "p#p < div#content < div#window < body#body.window < #root#main";

	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	INFO("Expected warning: Body 'class' attribute overridden by template.");
	TestsShell::SetNumExpectedWarnings(1);
	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	TestsShell::SetNumExpectedWarnings(0);

	document->Show();
	TestsShell::RenderLoop();

	Element* el_p = document->GetElementById("p");
	REQUIRE(el_p);
	CHECK(el_p->GetAddress() == p_address);

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("template.body+inline")
{
	static const String document_rml = R"(
<rml>
<head>
	<link type="text/template" href="/assets/window.rml"/>
	<link type="text/template" href="/../Tests/Data/UnitTests/template_basic.rml"/>
	<style>
		body.window
		{
			top: 100px;
			left: 200px;
			width: 600px;
			height: 450px;
		}
	</style>
</head>

<body id="body" template="window">
	<p id="p">A paragraph</p>
	<div id="basic_wrapper">
		<template src="basic">
			Hello!<span id="span">World</span>
		</template>
	</div>
</body>
</rml>
)";

	static const String p_address = "p#p < div#content < div#window < body#body.window < #root#main";
	static const String span_address = "span#span < p#text < div#basic_wrapper < div#content < div#window < body#body.window < #root#main";

	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	document->Show();
	TestsShell::RenderLoop();

	Element* el_p = document->GetElementById("p");
	Element* el_span = document->GetElementById("span");
	REQUIRE(el_p);
	REQUIRE(el_span);
	CHECK(el_p->GetAddress() == p_address);
	CHECK(el_span->GetAddress() == span_address);

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("template.inline")
{
	static const String document_rml = R"(
<rml>
<head>
	<link type="text/template" href="/assets/window.rml"/>
	<style>
		body
		{
			top: 100px;
			left: 200px;
			width: 600px;
			height: 450px;
		}
	</style>
</head>

<body id="body" class="inline">
<p>Paragraph outside the window.</p>
<div id="template_parent">
	<template src="window">
		<p id="p">A paragraph</p>
	</template>
</div>
</body>
</rml>
)";

	static const String p_address = "p#p < div#content < div#window < div#template_parent < body#body.inline < #root#main";

	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	document->Show();
	TestsShell::RenderLoop();

	Element* el_p = document->GetElementById("p");
	REQUIRE(el_p);
	CHECK(el_p->GetAddress() == p_address);

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("template.inline+inline.unique")
{
	static const String document_rml = R"(
<rml>
<head>
	<link type="text/template" href="/assets/window.rml"/>
	<link type="text/template" href="/../Tests/Data/UnitTests/template_basic.rml"/>
	<style>
		body
		{
			top: 100px;
			left: 200px;
			width: 600px;
			height: 450px;
		}
	</style>
</head>

<body id="body" class="inline">
<p>Paragraph outside the window.</p>
<div id="template_parent">
	<template src="window">
		<p id="p">A paragraph</p>
	</template>
	<div id="basic_wrapper">
		<template src="basic">
			Hello!<span id="span">World</span>
		</template>
	</div>
</div>
</body>
</rml>
)";

	static const String p_address = "p#p < div#content < div#window < div#template_parent < body#body.inline < #root#main";
	static const String span_address = "span#span < p#text < div#basic_wrapper < div#template_parent < body#body.inline < #root#main";

	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	document->Show();
	TestsShell::RenderLoop();

	Element* el_p = document->GetElementById("p");
	Element* el_span = document->GetElementById("span");
	REQUIRE(el_p);
	REQUIRE(el_span);
	CHECK(el_p->GetAddress() == p_address);
	CHECK(el_span->GetAddress() == span_address);

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("template.inline+inline.identical.wrapped")
{
	static const String document_rml = R"(
<rml>
<head>
	<link type="text/template" href="/assets/window.rml"/>
	<link type="text/template" href="/../Tests/Data/UnitTests/template_basic.rml"/>
	<style>
		body
		{
			top: 100px;
			left: 200px;
			width: 600px;
			height: 450px;
		}
		p {
			border: 1px aqua;
			padding: 5px;
			margin: 10px;
		}
	</style>
</head>

<body id="body" class="inline">
<div id="wrap_t1">
	<template src="basic">
		Enable<span id="s1">X</span>
	</template>
</div>
<div id="wrap_t2">
	<template src="basic">
		Disable<span id="s2">Y</span>
	</template>
</div>
</body>
</rml>
)";

	static const String s1_address = "span#s1 < p#text < div#wrap_t1 < body#body.inline < #root#main";
	static const String s2_address = "span#s2 < p#text < div#wrap_t2 < body#body.inline < #root#main";

	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	document->Show();
	TestsShell::RenderLoop();

	CHECK(document->GetElementById("s1")->GetAddress() == s1_address);
	CHECK(document->GetElementById("s2")->GetAddress() == s2_address);
	CHECK(StringUtilities::StripWhitespace(document->QuerySelector("#wrap_t1 p#text")->GetInnerRML()) == R"(Enable<span id="s1">X</span>)");

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("template.inline+inline.identical.siblings")
{
	static const String document_rml = R"(
<rml>
<head>
	<link type="text/template" href="/assets/window.rml"/>
	<link type="text/template" href="/../Tests/Data/UnitTests/template_basic.rml"/>
	<style>
		body
		{
			top: 100px;
			left: 200px;
			width: 600px;
			height: 450px;
		}
		p {
			border: 1px aqua;
			padding: 5px;
			margin: 10px;
		}
	</style>
</head>

<body id="body" class="inline">
<template src="basic">
	Enable<span id="s1">X</span>
</template>
<template src="basic">
	Disable<span id="s2">Y</span>
</template>
</body>
</rml>
)";

	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	document->Show();
	TestsShell::RenderLoop();

	ElementList p_elements;
	document->GetElementsByTagName(p_elements, "p");
	REQUIRE(p_elements.size() == 2);
	CHECK(StringUtilities::StripWhitespace(p_elements[0]->GetInnerRML()) == R"(Enable<span id="s1">X</span>)");
	CHECK(StringUtilities::StripWhitespace(p_elements[1]->GetInnerRML()) == R"(Disable<span id="s2">Y</span>)");

	document->Close();
	TestsShell::ShutdownShell();
}
