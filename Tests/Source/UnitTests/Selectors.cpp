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
			font-family: LatoLatin;
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
		Some text
		<p  id="B" unit="m"/>
		<p  id="C"/>
		<p  id="D"> <span id="D0">  </span><span id="D1">Text</span> </p>
		<h3 id="E"/>
		<p  id="F"> <span id="F0" class="hello-world"/> </p>
		<p  id="G" class/>
		<p  id="H" class="world hello"/>
	</div>
	<input id="I" type="checkbox" checked/>
</body>
</rml>
)";

enum class SelectorOp { None, RemoveElementsByIds, InsertElementBefore, RemoveClasses, RemoveId, RemoveChecked, RemoveAttributeUnit, SetHover };

struct QuerySelector {
	QuerySelector(String selector, String expected_ids, int expect_num_warnings = 0, int expect_num_query_warnings = 0) :
		selector(std::move(selector)), expected_ids(std::move(expected_ids)), expect_num_warnings(expect_num_warnings),
		expect_num_query_warnings(expect_num_query_warnings)
	{}
	QuerySelector(String selector, String expected_ids, SelectorOp operation, String operation_argument, String expected_ids_after_operation) :
		selector(std::move(selector)), expected_ids(std::move(expected_ids)), operation(operation), operation_argument(std::move(operation_argument)),
		expected_ids_after_operation(std::move(expected_ids_after_operation))
	{}
	String selector;
	String expected_ids;

	// If this rule should fail to parse or otherwise produce warnings, set these to non-zero values.
	int expect_num_warnings = 0, expect_num_query_warnings = 0;

	// Optionally also test the selector after dynamically making a structural operation on the document.
	SelectorOp operation = SelectorOp::None;
	String operation_argument;
	String expected_ids_after_operation;
};

// clang-format off
static const Vector<QuerySelector> query_selectors =
{
	{ "span",                        "Y D0 D1 F0" },
	{ ".hello",                      "X Z H" },
	{ ".hello.world",                "Z H" },
	{ "div.hello",                   "X Z" },
	{ "body .hello",                 "X Z H" },
	{ "body>.hello",                 "X Z" },
	{ "body > .hello",               "X Z" },
	{ ".parent *",                   "A B C D D0 D1 E F F0 G H" },
	{ ".parent > *",                 "A B C D E F G H" },
	{ ":checked",                    "I",               SelectorOp::RemoveChecked,        "I", "" },

	{ "*",                           "X Y Z P A B C D D0 D1 E F F0 G H I" },
	{ "*span",                       "Y D0 D1 F0" },
	{ "*.hello",                     "X Z H" },
	{ "*:checked",                   "I" },
	
	{ "p[unit='m']",                 "B" },
	{ "p[unit=\"m\"]",               "B" },
	{ "[class]",                     "X Y Z P F0 G H" },
	{ "[class=hello]",               "X" },
	{ "[class=]",                    "G" },
	{ "[class='']",                  "G" },
	{ "[class=\"\"]",                "G" },
	{ "[class~=hello]",              "X Z H" },
	{ "[class~=ello]",               "" },
	{ "[class|=hello]",              "X F0" },
	{ "[class^=hello]",              "X Z F0" },
	{ "[class^=]",                   "X Y Z P F0 G H" },
	{ "[class$=hello]",              "X H" },
	{ "[class*=hello]",              "X Z F0 H" },
	{ "[class*=ello]",               "X Z F0 H" },
	
	{ "[class~=hello].world",         "Z H" },
	{ "*[class~=hello].world",        "Z H" },
	{ ".world[class~=hello]",         "Z H" },
	{ "[class~=hello][class~=world]", "Z H" },

	{ "[class=hello world]",          "Z" },
	{ "[class='hello world']",        "Z" },
	{ "[class=\"hello world\"]",      "Z" },

	{ "[invalid",                     "", 1, 4 },
	{ "[]",                           "", 1, 4 },
	{ "[x=Rule{What}]",               "", 2, 0 },
	{ "[x=Hello,world]",              "", 1, 2 }, 
	// The next ones are valid in CSS but we currently don't bother handling them, just make sure we don't crash.
	{ "[x='Rule{What}']",             "", 2, 0 },
	{ "[x='Hello,world']",            "", 1, 2 }, 

	{ "#X[class=hello]",              "X" },
	{ "[class=hello]#X",              "X" },
	{ "#Y[class=hello]",              "" },
	{ "div[class=hello]",             "X" },
	{ "[class=hello]div",             "X" },
	{ "span[class=hello]",            "" },
	
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
	{ ":nth-child(4) * span:first-child", "D0 F0",      SelectorOp::RemoveElementsByIds,  "X",    "" },
	{ "p:nth-last-of-type(3n+1)",    "D H" },
	{ ":first-of-type",              "X Y A B D0 E F0 I" },
	{ ":last-of-type",               "Y P A D1 E F0 H I" },
	{ ":only-child",                 "F0",              SelectorOp::RemoveElementsByIds,  "D0",    "D1 F0" },
	{ ":only-of-type",               "Y A E F0 I" },
	{ "span:empty",                  "Y D0 F0" },
	
	{ ".hello.world, #P span, #I",   "Z D0 D1 F0 H I",  SelectorOp::RemoveClasses,        "world", "D0 D1 F0 I" },
	{ "body * span",                 "D0 D1 F0" },
	{ "D1 *",                        "" },
	
	{ "#E + #F",                     "F",               SelectorOp::InsertElementBefore,  "F",     "" },
	{ "#E+#F",                       "F" },
	{ "#E +#F",                      "F" },
	{ "#E+ #F",                      "F" },
	{ "#F + #E",                     "" },
	{ "#A + #B",                     "B",               SelectorOp::RemoveId,             "A", "" },
	{ "* + #A",                      "" },
	{ "#H + *",                      "" },
	{ "#P + *",                      "I" },
	{ "div.parent > #B + p",         "C" },
	{ "div.parent > #B + div",       "" },
	
	{ "#B ~ #F",                     "F" },
	{ "#B~#F",                       "F" },
	{ "#B ~#F",                      "F" },
	{ "#B~ #F",                      "F" },
	{ "#F ~ #B",                     "" },
	{ "div.parent > #B ~ p:empty",   "C G H",           SelectorOp::InsertElementBefore,  "H",     "C G Inserted H" },
	{ "div.parent > #B ~ * span",    "D0 D1 F0" },

	{ ":not(*)",                     "" },
	{ ":not(span)",                  "X Z P A B C D E F G H I" },
	{ "#D :not(#D0)",                "D1" },
	{ "body > :not(:checked)",       "X Y Z P",         SelectorOp::RemoveChecked,        "I", "X Y Z P I" },
	{ "div.hello:not(.world)",       "X" },
	{ ":not(div,:nth-child(2),p *)", "A C D E F G H I" },

	{ ".hello + .world",             "Y",               SelectorOp::RemoveClasses,        "hello", ""  },
	{ "#B ~ h3",                     "E",               SelectorOp::RemoveId,             "B", ""  },
	{ "#Z + div > :nth-child(2)",    "B",               SelectorOp::RemoveId,             "Z", ""  },
	{ ":hover + #P #D1",             "",                SelectorOp::SetHover,             "Z", "D1"  },
	{ ":not(:hover) + #P #D1",       "D1",              SelectorOp::SetHover,             "Z", ""  },
	{ "#X + #Y",                     "Y",               SelectorOp::RemoveId,             "X", ""  },

	{ "p[unit=m]",                   "B",               SelectorOp::RemoveAttributeUnit,  "B", ""  },
	{ "p[unit=m] + *",               "C",               SelectorOp::RemoveAttributeUnit,  "B", ""  },

	{ "body > * #D0",                "D0" },
	{ "#E + * ~ *",                  "G H" },
	{ "#B + * ~ #G",                 "G" },
	{ "body > :nth-child(4) span:first-child",  "D0 F0", SelectorOp::RemoveElementsByIds,  "X",    "" },
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

struct MatchesSelector {
	String id;
	String selector;
	bool expected_result;
};
static const Vector<MatchesSelector> matches_selectors =
{
	{ "X", ".world",         false },
	{ "X", ".hello",         true },
	{ "X", ".hello, .world", true },
	{ "E", "h3",             true },
	{ "G", "p#G[class]",     true },
	{ "G", "p#G[missing]",   false },
	{ "B", "[unit='m']",     true }
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
	Context* context = TestsShell::GetContext();

	SUBCASE("RCSS document selectors")
	{
		for (const QuerySelector& selector : query_selectors)
		{
			TestsShell::SetNumExpectedWarnings(selector.expect_num_warnings);
			const String selector_css = selector.selector + " { drag: drag; } ";
			const String document_string = doc_begin + selector_css + doc_end;
			ElementDocument* document = context->LoadDocumentFromMemory(document_string);

			// Update the context to settle any dirty state.
			context->Update();

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
				case SelectorOp::RemoveId:
					document->GetElementById(selector.operation_argument)->SetId("");
					operation_str = "RemoveId";
					break;
				case SelectorOp::RemoveAttributeUnit:
					document->GetElementById(selector.operation_argument)->RemoveAttribute("unit");
					operation_str = "RemoveAttributeUnit";
					break;
				case SelectorOp::SetHover:
					document->GetElementById(selector.operation_argument)->SetPseudoClass("hover", true);
					operation_str = "SetHover";
					break;
				case SelectorOp::None: break;
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
			TestsShell::SetNumExpectedWarnings(selector.expect_num_query_warnings);

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

	SUBCASE("Matches")
	{
		const String document_string = doc_begin + doc_end;
		ElementDocument* document = context->LoadDocumentFromMemory(document_string);
		REQUIRE(document);

		for (const MatchesSelector& selector : matches_selectors)
		{
			Element* start = document->GetElementById(selector.id);
			REQUIRE(start);

			bool matches = start->Matches(selector.selector);
			CHECK_MESSAGE(matches == selector.expected_result, "Matches() selector '" << selector.selector << "' from " << selector.id);
		}
		context->UnloadDocument(document);
	}

	TestsShell::ShutdownShell();
}
