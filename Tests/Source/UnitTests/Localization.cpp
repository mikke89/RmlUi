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
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/StyleTypes.h>
#include <RmlUi/Core/StringUtilities.h>
#include <doctest.h>

using namespace Rml;

static const String document_localization_rml = R"(
<rml>
<head>
</head>

<body>
	<div id="parent" lang="en" dir="ltr">
		<span id="cell0"/>
		<span id="cell1" lang="nl"/>
		<span id="cell2" dir="auto"/>
		<span id="cell3" lang="ar" dir="rtl"/>
	</div>
</body>
</rml>
)";

TEST_CASE("Localization")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_localization_rml);
	REQUIRE(document);
	document->Show();

	Element* parent_element = document->GetElementById("parent");
	REQUIRE(parent_element);
	Element* cells[4]{};
	for (int i = 0; i < 4; ++i)
	{
		cells[i] = document->GetElementById(CreateString(8, "cell%d", i));
		REQUIRE(cells[i]);
	}

	TestsShell::RenderLoop();

	SUBCASE("Language")
	{
		REQUIRE(document->GetProperty(PropertyId::Language)->Get<String>() == "");
		REQUIRE(parent_element->GetProperty(PropertyId::Language)->Get<String>() == "en");

		REQUIRE(cells[0]->GetProperty(PropertyId::Language)->Get<String>() == "en");
		REQUIRE(cells[1]->GetProperty(PropertyId::Language)->Get<String>() == "nl");
		REQUIRE(cells[2]->GetProperty(PropertyId::Language)->Get<String>() == "en");
		REQUIRE(cells[3]->GetProperty(PropertyId::Language)->Get<String>() == "ar");
	}

	SUBCASE("Direction")
	{
		REQUIRE(document->GetProperty(PropertyId::Direction)->Get<Style::Direction>() == Style::Direction::Auto);
		REQUIRE(parent_element->GetProperty(PropertyId::Direction)->Get<Style::Direction>() == Style::Direction::Ltr);

		REQUIRE(cells[0]->GetProperty(PropertyId::Direction)->Get<Style::Direction>() == Style::Direction::Ltr);
		REQUIRE(cells[1]->GetProperty(PropertyId::Direction)->Get<Style::Direction>() == Style::Direction::Ltr);
		REQUIRE(cells[2]->GetProperty(PropertyId::Direction)->Get<Style::Direction>() == Style::Direction::Auto);
		REQUIRE(cells[3]->GetProperty(PropertyId::Direction)->Get<Style::Direction>() == Style::Direction::Rtl);
	}

	document->Close();
	TestsShell::ShutdownShell();
}
