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
#include <algorithm>

using namespace Rml;

static const String document_body_template_rml = R"(
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

static const String p_address_body_template = "p#p < div#content < div#window < body#body.window < #root#main";

static const String document_inline_template_rml = R"(
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

static const String p_address_inline_template = "p#p < div#content < div#window < div#template_parent < body#body.inline < #root#main";


TEST_CASE("template")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	SUBCASE("body")
	{
		INFO("Expected warning: Body 'class' attribute overridden by template.");
		TestsShell::SetNumExpectedWarnings(1);

		ElementDocument* document = context->LoadDocumentFromMemory(document_body_template_rml);
		REQUIRE(document);
		TestsShell::SetNumExpectedWarnings(0);

		document->Show();

		context->Update();
		context->Render();

		TestsShell::RenderLoop();

		Element* el_p = document->GetElementById("p");
		REQUIRE(el_p);

		CHECK(el_p->GetAddress() == p_address_body_template);

		document->Close();
	}

	SUBCASE("inline")
	{
		ElementDocument* document = context->LoadDocumentFromMemory(document_inline_template_rml);
		REQUIRE(document);
		document->Show();

		context->Update();
		context->Render();

		TestsShell::RenderLoop();

		Element* el_p = document->GetElementById("p");
		REQUIRE(el_p);

		CHECK(el_p->GetAddress() == p_address_inline_template);

		document->Close();
	}

	TestsShell::ShutdownShell();
}
