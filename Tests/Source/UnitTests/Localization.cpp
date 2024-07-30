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
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/StringUtilities.h>
#include <RmlUi/Core/StyleTypes.h>
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
		cells[i] = document->GetElementById(CreateString("cell%d", i));
		REQUIRE(cells[i]);
	}

	TestsShell::RenderLoop();

	SUBCASE("Language")
	{
		REQUIRE(document->GetProperty(PropertyId::RmlUi_Language)->Get<String>() == "");
		REQUIRE(document->GetComputedValues().language() == "");
		REQUIRE(parent_element->GetProperty(PropertyId::RmlUi_Language)->Get<String>() == "en");
		REQUIRE(parent_element->GetComputedValues().language() == "en");

		CHECK(cells[0]->GetProperty(PropertyId::RmlUi_Language)->Get<String>() == "en");
		CHECK(cells[0]->GetComputedValues().language() == "en");
		CHECK(cells[1]->GetProperty(PropertyId::RmlUi_Language)->Get<String>() == "nl");
		CHECK(cells[1]->GetComputedValues().language() == "nl");
		CHECK(cells[2]->GetProperty(PropertyId::RmlUi_Language)->Get<String>() == "en");
		CHECK(cells[2]->GetComputedValues().language() == "en");
		CHECK(cells[3]->GetProperty(PropertyId::RmlUi_Language)->Get<String>() == "ar");
		CHECK(cells[3]->GetComputedValues().language() == "ar");

		SUBCASE("Change language")
		{
			parent_element->SetAttribute("lang", "es");
			TestsShell::RenderLoop();

			REQUIRE(parent_element->GetProperty(PropertyId::RmlUi_Language)->Get<String>() == "es");
			REQUIRE(parent_element->GetComputedValues().language() == "es");

			CHECK(cells[0]->GetProperty(PropertyId::RmlUi_Language)->Get<String>() == "es");
			CHECK(cells[0]->GetComputedValues().language() == "es");
			CHECK(cells[1]->GetProperty(PropertyId::RmlUi_Language)->Get<String>() == "nl");
			CHECK(cells[1]->GetComputedValues().language() == "nl");
		}
	}

	SUBCASE("Direction")
	{
		REQUIRE(document->GetProperty(PropertyId::RmlUi_Direction)->Get<Style::Direction>() == Style::Direction::Auto);
		REQUIRE(document->GetComputedValues().direction() == Style::Direction::Auto);
		REQUIRE(parent_element->GetProperty(PropertyId::RmlUi_Direction)->Get<Style::Direction>() == Style::Direction::Ltr);
		REQUIRE(parent_element->GetComputedValues().direction() == Style::Direction::Ltr);

		CHECK(cells[0]->GetProperty(PropertyId::RmlUi_Direction)->Get<Style::Direction>() == Style::Direction::Ltr);
		CHECK(cells[0]->GetComputedValues().direction() == Style::Direction::Ltr);
		CHECK(cells[1]->GetProperty(PropertyId::RmlUi_Direction)->Get<Style::Direction>() == Style::Direction::Ltr);
		CHECK(cells[1]->GetComputedValues().direction() == Style::Direction::Ltr);
		CHECK(cells[2]->GetProperty(PropertyId::RmlUi_Direction)->Get<Style::Direction>() == Style::Direction::Auto);
		CHECK(cells[2]->GetComputedValues().direction() == Style::Direction::Auto);
		CHECK(cells[3]->GetProperty(PropertyId::RmlUi_Direction)->Get<Style::Direction>() == Style::Direction::Rtl);
		CHECK(cells[3]->GetComputedValues().direction() == Style::Direction::Rtl);

		SUBCASE("Change direction")
		{
			parent_element->SetAttribute("dir", "rtl");
			TestsShell::RenderLoop();

			REQUIRE(parent_element->GetProperty(PropertyId::RmlUi_Direction)->Get<Style::Direction>() == Style::Direction::Rtl);
			REQUIRE(parent_element->GetComputedValues().direction() == Style::Direction::Rtl);

			CHECK(cells[0]->GetProperty(PropertyId::RmlUi_Direction)->Get<Style::Direction>() == Style::Direction::Rtl);
			CHECK(cells[0]->GetComputedValues().direction() == Style::Direction::Rtl);
			CHECK(cells[2]->GetProperty(PropertyId::RmlUi_Direction)->Get<Style::Direction>() == Style::Direction::Auto);
			CHECK(cells[2]->GetComputedValues().direction() == Style::Direction::Auto);
		}
	}

	document->Close();
	TestsShell::ShutdownShell();
}
