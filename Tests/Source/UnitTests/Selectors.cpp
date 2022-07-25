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

// clang-format off
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

enum class SelectorOp { None, RemoveElementsByIds, InsertElementBefore, RemoveClasses, RemoveChecked };

struct QuerySelector {
	QuerySelector(String selector, String expected_ids) : selector(std::move(selector)), expected_ids(std::move(expected_ids)) {}
	QuerySelector(String selector, String expected_ids, SelectorOp operation, String operation_argument, String expected_ids_after_operation) :
		selector(std::move(selector)), expected_ids(std::move(expected_ids)), operation(operation), operation_argument(std::move(operation_argument)),
		expected_ids_after_operation(std::move(expected_ids_after_operation))
	{}
	String selector;
	String expected_ids;

	// Optionally also test the selector after dynamically making a structural operation on the document.
	SelectorOp operation = SelectorOp::None;
	String operation_argument;
	String expected_ids_after_operation;
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
	{ ":checked",                    "I",               SelectorOp::RemoveChecked,        "I", "" },
	{ ".parent :nth-child(odd)",     "A C D0 E F0 G" },
	{ ".parent > :nth-child(even)",  "B D F H",         SelectorOp::RemoveClasses,        "parent", "" },
	{ ":first-child",                "X A D0 F0",       SelectorOp::RemoveElementsByIds,  "A F0", "X B D0" },
	{ ":last-child",                 "D1 F0 H I",       SelectorOp::RemoveElementsByIds,  "D0 H", "D1 F0 G I" },
	{ "h1:nth-child(2)",             "",                SelectorOp::InsertElementBefore,  "A",    "A" },
	{ "p:nth-child(2)",              "B",               SelectorOp::InsertElementBefore,  "A",    "" },
	{ "p:nth-child(2)",              "B",               SelectorOp::RemoveElementsByIds,  "A",    "C" },
	{ "p:nth-child(3n+1)",           "D G",             SelectorOp::RemoveElementsByIds,  "B",    "H" },
	{ "p:nth-child(3n + 1)",         "D G" },
	{ "#P > :nth-last-child(2n+1)",  "B D F H" },
	{ "#P p:nth-of-type(odd)",       "B D G" },
	{ "span:first-child",            "D0 F0" },
	{ "body span:first-child",       "D0 F0" },
	{ "body > p span:first-child",   "" },
	{ "body > div span:first-child", "D0 F0" },
	{ ":nth-child(4) * span:first-child", "D0 F0", SelectorOp::RemoveElementsByIds,  "X",    "" },
	{ "p:nth-last-of-type(3n+1)",    "D H" },
	{ ":first-of-type",              "X Y A B D0 E F0 I" },
	{ ":last-of-type",               "Y P A D1 E F0 H I" },
	{ ":only-child",                 "F0",              SelectorOp::RemoveElementsByIds,  "D0",    "D1 F0" },
	{ ":only-of-type",               "Y A E F0 I" },
	{ "span:empty",                  "Y D0 D1 F0" },
	{ ".hello.world, #P span, #I",   "Z D0 D1 F0 I",    SelectorOp::RemoveClasses,        "world", "D0 D1 F0 I" },
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
// clang-format on

// Recursively iterate through 'element' and all of its descendants to find all
// elements matching a particular property used to tag matching selectors.
static void GetMatchingIds(String& matching_ids, Element* element)
{
	String id = element->GetId();
	if (!id.empty() && element->GetProperty<int>("drag") == (int)Style::Drag::Drag)
	{
		if (!matching_ids.empty())
			matching_ids += ' ';
		matching_ids += id;
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

static void RemoveElementsWithIds(ElementDocument* document, const String& remove_ids)
{
	StringList remove_id_list;
	StringUtilities::ExpandString(remove_id_list, remove_ids, ' ');
	for (const String& id : remove_id_list)
	{
		if (Element* element = document->GetElementById(id))
			element->GetParentNode()->RemoveChild(element);
	}
}
static void RemoveClassesFromAllElements(ElementDocument* document, const String& remove_classes)
{
	StringList class_list;
	StringUtilities::ExpandString(class_list, remove_classes, ' ');
	for (const String& name : class_list)
	{
		ElementList element_list;
		document->GetElementsByClassName(element_list, name);
		for (Element* element : element_list)
			element->SetClass(name, false);
	}
}
static void InsertElementBefore(ElementDocument* document, const String& before_id)
{
	Element* element = document->GetElementById(before_id);
	ElementPtr new_element = document->CreateElement("p");
	new_element->SetId("Inserted");
	element->GetParentNode()->InsertBefore(std::move(new_element), element);
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
			CHECK_MESSAGE(matching_ids == selector.expected_ids, "Selector: " << selector.selector);

			// Also check validity of selectors after structural document changes.
			if (selector.operation != SelectorOp::None)
			{
				String operation_str;
				switch (selector.operation)
				{
				case SelectorOp::RemoveElementsByIds:
					RemoveElementsWithIds(document, selector.operation_argument);
					operation_str = "RemoveElementsByIds";
					break;
				case SelectorOp::InsertElementBefore:
					InsertElementBefore(document, selector.operation_argument);
					operation_str = "InsertElementBefore";
					break;
				case SelectorOp::RemoveClasses:
					RemoveClassesFromAllElements(document, selector.operation_argument);
					operation_str = "RemoveClasses";
					break;
				case SelectorOp::RemoveChecked:
					document->GetElementById(selector.operation_argument)->RemoveAttribute("checked");
					operation_str = "RemoveChecked";
					break;
				case SelectorOp::None:
					break;
				}
				context->Update();

				String matching_ids_after_operation;
				GetMatchingIds(matching_ids_after_operation, document);
				CHECK_MESSAGE(matching_ids_after_operation == selector.expected_ids_after_operation, "Selector: ", selector.selector,
					"  Operation: ", operation_str, "  Argument: ", selector.operation_argument);
			}

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
