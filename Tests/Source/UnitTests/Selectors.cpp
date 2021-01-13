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

#include "../Common/TestsInterface.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Types.h>
#include <doctest.h>

using namespace Rml;

static const String doc_begin = R"(
<rml>
<head>
	<title>Demo</title>
	<style>
		body
		{
			width: 400px;
			height: 300px;
			margin: auto;
		}
)";
static const String doc_end = R"(
	</style>
</head>
<body>
	<div  id="X" class="hello"/>
	<span id="Y" class="world"/>
	<div  id="Z" class="hello world"/>
	<div  id="P" class="parent">
		<h1 id="A"/>
		<p  id="B"/>
		<p  id="C"/>
		<p  id="D"> <span id="D0"/><span id="D1"/> </p>
		<h3 id="E"/>
		<p  id="F"> <span id="F0"/> </p>
		<p  id="G"/>
		<p  id="H" class="hello"/>
	</div>
	<input id="I" type="checkbox" checked/>
</body>
</rml>
)";

struct QuerySelector {
	String selector;
	String expected_ids;
};
static const Vector<QuerySelector> query_selectors =
{
	{ "span",                        "Y D0 D1 F0" },
	{ ".hello",                      "X Z H" },
	{ ".hello.world",                "Z" },
	{ "div.hello",                   "X Z" },
	{ "body .hello",                 "X Z H" },
	{ "body>.hello",                 "X Z" },
	{ "body > .hello",               "X Z" },
	{ ".parent *",                   "A B C D D0 D1 E F F0 G H" },
	{ ".parent > *",                 "A B C D E F G H" },
	{ ":checked",                    "I" },
	{ ".parent :nth-child(odd)",     "A C D0 E F0 G" },
	{ ".parent > :nth-child(even)",  "B D F H" },
	{ ":first-child",                "X A D0 F0" },
	{ ":last-child",                 "D1 F0 H I" },
	{ "p:nth-child(2)",              "B" },
	{ "h1:nth-child(2)",             "" },
	{ "p:nth-child(3n+1)",           "D G" },
	{ "p:nth-child(3n + 1)",         "D G" },
	{ "#P > :nth-last-child(2n+1)",  "B D F H" },
	{ "#P p:nth-of-type(odd)",       "B D G" },
	{ "p:nth-last-of-type(3n+1)",    "D H" },
	{ ":first-of-type",              "X Y A B D0 E F0 I" },
	{ ":last-of-type",               "Y P A D1 E F0 H I" },
	{ ":only-child",                 "F0" },
	{ ":only-of-type",               "Y A E F0 I" },
	{ "span:empty",                  "Y D0 D1 F0" },
	{ ".hello.world, #P span, #I",   "Z D0 D1 F0 I" },
	{ "body * span",                 "D0 D1 F0" },
};
struct ClosestSelector {
	String start_id;
	String selector;
	String expected_id;
};
static const Vector<ClosestSelector> closest_selectors =
{
	{ "D1",  "#P",               "P" },
	{ "D1",  "#P, body",         "P" },
	{ "D1",  "#P, #D",           "D" },
	{ "D1",  "#Z",               "" },
	{ "D1",  "#D1",              "" },
	{ "D1",  "#D0",              "" },
	{ "D1",  "div",              "P" },
	{ "D1",  "p",                "D" },
	{ "D1",  ":nth-child(4)",    "D" },
	{ "D1",  "div:nth-child(4)", "P" },
};


// Recursively iterate through 'element' and all of its descendants to find all
// elements matching a particular property used to tag matching selectors.
static void GetMatchingIds(String& matching_ids, Element* element)
{
	String id = element->GetId();
	if (!id.empty() && element->GetProperty<int>("drag") == (int)Style::Drag::Drag)
	{
		matching_ids += id + ' ';
	}

	for (int i = 0; i < element->GetNumChildren(); i++)
	{
		GetMatchingIds(matching_ids, element->GetChild(i));
	}
}

// Return the list of IDs that should match the above 'selectors.expected_ids'.
static String ElementListToIds(const ElementList& elements)
{
	String result;

	for (Element* element : elements)
	{
		result += element->GetId() + ' ';
	}

	if (!result.empty())
		result.pop_back();

	return result;
}

TEST_CASE("Selectors")
{
	const Vector2i window_size(1024, 768);

	TestsSystemInterface system_interface;
	TestsRenderInterface render_interface;

	SetRenderInterface(&render_interface);
	SetSystemInterface(&system_interface);

	Initialise();

	Context* context = Rml::CreateContext("main", window_size);
	REQUIRE(context);

	SUBCASE("RCSS document selectors")
	{
		for (const QuerySelector& selector : query_selectors)
		{
			const String selector_css = selector.selector + " { drag: drag; } ";
			const String document_string = doc_begin + selector_css + doc_end;
			ElementDocument* document = context->LoadDocumentFromMemory(document_string);
			REQUIRE(document);

			String matching_ids;
			GetMatchingIds(matching_ids, document);

			if (!matching_ids.empty())
				matching_ids.pop_back();

			CHECK_MESSAGE(matching_ids == selector.expected_ids, "Selector: " << selector.selector);
			context->UnloadDocument(document);
		}
	}

	SUBCASE("QuerySelector(All)")
	{
		const String document_string = doc_begin + doc_end;
		ElementDocument* document = context->LoadDocumentFromMemory(document_string);
		REQUIRE(document);

		for (const QuerySelector& selector : query_selectors)
		{
			ElementList elements;
			document->QuerySelectorAll(elements, selector.selector);
			String matching_ids = ElementListToIds(elements);

			Element* first_element = document->QuerySelector(selector.selector);
			if (first_element)
			{
				CHECK_MESSAGE(first_element == elements[0], "QuerySelector does not return the first match of QuerySelectorAll.");
			}
			else
			{
				CHECK_MESSAGE(elements.empty(), "QuerySelector found nothing, while QuerySelectorAll found " << elements.size() << " element(s).");
			}

			CHECK_MESSAGE(matching_ids == selector.expected_ids, "QuerySelector: " << selector.selector);
		}
		context->UnloadDocument(document);
	}

	SUBCASE("Closest")
	{
		const String document_string = doc_begin + doc_end;
		ElementDocument* document = context->LoadDocumentFromMemory(document_string);
		REQUIRE(document);

		for (const ClosestSelector& selector : closest_selectors)
		{
			Element* start = document->GetElementById(selector.start_id);
			REQUIRE(start);

			Element* match = start->Closest(selector.selector);
			const String match_id = match ? match->GetId() : "";
			CHECK_MESSAGE(match_id == selector.expected_id, "Closest() selector '" << selector.selector << "' from " << selector.start_id);
		}
		context->UnloadDocument(document);
	}

	Rml::Shutdown();
}
