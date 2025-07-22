/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2025 The RmlUi Team, and contributors
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

#include "../../../Source/Core/Layout/FormattingContextDebug.h"
#include "../../../Source/Core/Layout/LayoutNode.h"
#include "../Common/TestsShell.h"
#include "../Common/TypesToString.h"
#include <RmlUi/Core/ComputedValues.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/ElementText.h>
#include <RmlUi/Core/ElementUtilities.h>
#include <RmlUi/Debugger/Debugger.h>
#include <doctest.h>

using namespace Rml;

struct ElementLayoutInfo {
	ElementLayoutInfo(int tree_depth, const String& address, const Box& box, Vector2f absolute_offset) :
		tree_depth(tree_depth), address(address), box(box), absolute_offset(absolute_offset)
	{}

	int tree_depth;
	String address;
	Box box;
	Vector2f absolute_offset;

	bool operator==(const ElementLayoutInfo& other) const
	{
		return tree_depth == other.tree_depth && address == other.address && box == other.box && absolute_offset == other.absolute_offset;
	}
	bool operator!=(const ElementLayoutInfo& other) const { return !(*this == other); }

	String ToString() const
	{
		return String(size_t(4 * tree_depth), ' ') +
			CreateString("%s :: box = %g x %g (outer %g x %g) :: absolute_offset = %g x %g", address.c_str(), box.GetSize().x, box.GetSize().y,
				box.GetSizeAcross(BoxDirection::Horizontal, BoxArea::Margin), box.GetSizeAcross(BoxDirection::Vertical, BoxArea::Margin),
				absolute_offset.x, absolute_offset.y);
	}
};

static Vector<ElementLayoutInfo> CaptureLayoutTree(Element* root_element)
{
	Vector<ElementLayoutInfo> layout_info_list;
	ElementUtilities::VisitElementsDepthOrder(root_element, [&](Element* element, int tree_depth) {
		layout_info_list.emplace_back(tree_depth, element->GetAddress(false, false), element->GetBox(), element->GetAbsoluteOffset());
	});
	return layout_info_list;
}

static void LogLayoutTree(const Vector<ElementLayoutInfo>& layout_info_list)
{
	String message = "Element layout tree:\n";
	for (const auto& layout_info : layout_info_list)
		message += layout_info.ToString() + "\n";

	Rml::Log::Message(Rml::Log::LT_DEBUG, "%s", message.c_str());
}

static void LogDirtyLayoutTree(Element* root_element)
{
	String tree_dirty_state;
	ElementUtilities::VisitElementsDepthOrder(root_element, [&](Element* element, int tree_depth) {
		tree_dirty_state += String(size_t(4 * tree_depth), ' ');
		tree_dirty_state += CreateString("%s.  Self: %d  Child: %d", element->GetAddress(false, tree_depth == 0).c_str(),
			element->GetLayoutNode()->IsSelfDirty(), element->GetLayoutNode()->IsChildDirty());
		tree_dirty_state += '\n';
	});

	Log::Message(Log::LT_INFO, "Dirty layout tree:\n%s\n", tree_dirty_state.c_str());
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
        #flex-container {
            display: flex;
            flex-direction: column;
        }
        #overflow-container {
            overflow: auto;
            height: 150px;
        }
        #absolute-container {
            position: absolute;
            top: 300px;
            left: 300px;
        }
    </style>
</head>
<body>
    <div class="container" id="flex-container">
        <div>Flex item 1</div>
        <div id="flex-item">Flex item 2</div>
        <div id="flex-item-next">Flex item 3</div>
    </div>

    <div class="container" id="overflow-container">
        <div>Overflow item 1</div>
        <div id="overflow-item">Overflow item 2</div>
        <div>Overflow item 3</div>
        <div>Overflow item 4</div>
        <div>Overflow item 5</div>
    </div>

    <div class="container" id="absolute-container">
        <div id="absolute-item">Absolute item 1</div>
        <div>Absolute item 2</div>
    </div>

    <div class="container" id="normal-container">
        <div id="normal-item">Normal block box</div>
    </div>
</body>
</rml>
)";

TEST_CASE("LayoutIsolation.InsideOutsideFormattingContexts")
{
	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocumentFromMemory(document_isolation_rml);
	document->Show();
	TestsShell::RenderLoop();

	SUBCASE("Flex")
	{
		Element* element = document->GetElementById("flex-item");
		Element* next_sibling = element->GetNextSibling();
		Element* next_outside_overflow = document->GetElementById("normal-container");
		REQUIRE(element);
		REQUIRE(next_sibling);
		REQUIRE(next_outside_overflow);

		const Vector2f absolute_offset_element = element->GetAbsoluteOffset();
		const Vector2f absolute_offset_next_sibling = next_sibling->GetAbsoluteOffset();
		const Vector2f absolute_offset_next_outside_overflow = next_outside_overflow->GetAbsoluteOffset();
		rmlui_dynamic_cast<ElementText*>(element->GetFirstChild())->SetText("Modified text that is long enough to cause line break");

		TestsShell::RenderLoop();
		CHECK(element->GetAbsoluteOffset() == absolute_offset_element);
		CHECK(next_sibling->GetAbsoluteOffset() != absolute_offset_next_sibling);
		CHECK(next_outside_overflow->GetAbsoluteOffset() != absolute_offset_next_outside_overflow);
	}

	SUBCASE("Overflow")
	{
		Element* element = document->GetElementById("overflow-item");
		Element* next_sibling = element->GetNextSibling();
		Element* next_outside_overflow = document->GetElementById("normal-container");
		REQUIRE(element);
		REQUIRE(next_sibling);
		REQUIRE(next_outside_overflow);

		const Vector2f absolute_offset_element = element->GetAbsoluteOffset();
		const Vector2f absolute_offset_next_sibling = next_sibling->GetAbsoluteOffset();
		const Vector2f absolute_offset_next_outside_overflow = next_outside_overflow->GetAbsoluteOffset();
		rmlui_dynamic_cast<ElementText*>(element->GetFirstChild())->SetText("Modified text that is long enough to cause line break");

		TestsShell::RenderLoop();
		CHECK(element->GetAbsoluteOffset() == absolute_offset_element);
		CHECK(next_sibling->GetAbsoluteOffset() != absolute_offset_next_sibling);
		CHECK(next_outside_overflow->GetAbsoluteOffset() == absolute_offset_next_outside_overflow);
	}

	SUBCASE("Absolute")
	{
		Element* element = document->GetElementById("absolute-item");
		Element* next_sibling = element->GetNextSibling();
		REQUIRE(element);
		REQUIRE(next_sibling);

		const Vector2f absolute_offset_element = element->GetAbsoluteOffset();
		const Vector2f absolute_offset_next_sibling = next_sibling->GetAbsoluteOffset();
		rmlui_dynamic_cast<ElementText*>(element->GetFirstChild())->SetText("Modified text that is long enough to cause line break");

		TestsShell::RenderLoop();
		CHECK(element->GetAbsoluteOffset() == absolute_offset_element);
		CHECK(next_sibling->GetAbsoluteOffset() != absolute_offset_next_sibling);
	}

	SUBCASE("Normal")
	{
		Element* element = document->GetElementById("normal-item");
		REQUIRE(element);

		const float initial_width = element->GetBox().GetSize().x;
		element->SetProperty("width", "250px");

		TestsShell::RenderLoop();

		float new_width = element->GetBox().GetSize().x;
		CHECK(new_width != initial_width);
		CHECK(new_width == doctest::Approx(250.0f));
	}

	document->Close();
	TestsShell::ShutdownShell();
}

#ifdef RMLUI_DEBUG
// Wrap all the following tests under this condition, since the format independent debug tracker is only available in debug mode.

TEST_CASE("LayoutIsolation.FullLayoutFormatIndependentCount")
{
	Context* context = TestsShell::GetContext();
	FormatIndependentDebugTracker format_independent_tracker;
	ElementDocument* document = context->LoadDocumentFromMemory(document_isolation_rml);

	document->Show();
	TestsShell::RenderLoop();

	format_independent_tracker.LogMessage();

	const auto count_level_1 = std::count_if(format_independent_tracker.GetEntries().begin(), format_independent_tracker.GetEntries().end(),
		[](const auto& entry) { return entry.level == 1; });
	CHECK_MESSAGE(count_level_1 == 3, "Expecting one entry for each of flex, overflow, and absolute");

	// There are quite a few flex item format occurrences being performed currently. We might reduce the following
	// number while working on the flex formatting engine. If this fails for any other reason, it is likely a bug.
	CHECK(format_independent_tracker.CountEntries() == 10);
	CHECK(format_independent_tracker.CountFormattedEntries() == 10);

	document->Close();
	TestsShell::ShutdownShell();
}

static const String document_isolation_absolute_rml = R"(
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
        #absolute-container {
            position: absolute;
            top: 300px;
            left: 300px;
			width: 30%;
        }
    </style>
</head>
<body>
    <div class="container" id="absolute-container">
        <div id="absolute-item">Absolutely positioned box</div>
    </div>

    <div class="container" id="normal-container">
		<div id="normal-item">Normal block box</div>
	</div>
</body>
</rml>
)";

TEST_CASE("LayoutIsolation.Absolute")
{
	Context* context = TestsShell::GetContext();

	ElementDocument* document = context->LoadDocumentFromMemory(document_isolation_absolute_rml);
	document->Show();

	TestsShell::RenderLoop();

	FormatIndependentDebugTracker format_independent_tracker;
	SUBCASE("Modify absolute content")
	{
		Element* element = document->GetElementById("absolute-item");
		const float initial_height = element->GetOffsetHeight();

		rmlui_dynamic_cast<ElementText*>(element->GetFirstChild())->SetText("Modified text that is long enough to cause line break");
		TestsShell::RenderLoop();

		CHECK(element->GetOffsetHeight() != initial_height);
		CHECK(format_independent_tracker.CountFormattedEntries() == 1);
	}

	SUBCASE("Modify absolute width")
	{
		Element* container = document->GetElementById("absolute-container");
		Element* element = document->GetElementById("absolute-item");
		const float container_initial_width = container->GetOffsetWidth();
		const float element_initial_width = element->GetOffsetWidth();

		container->SetProperty("width", "300px");

		LogDirtyLayoutTree(document);
		document->UpdatePropertiesForDebug();
		LogDirtyLayoutTree(document);

		TestsShell::RenderLoop();

		CHECK(container->GetOffsetWidth() != container_initial_width);
		CHECK(element->GetOffsetWidth() != element_initial_width);
		// The following could in principle be reduced to 1, since the size of an absolute element should not affect the
		// layout of the formatting context it takes part in.
		CHECK(format_independent_tracker.CountEntries() == 2);
		CHECK(format_independent_tracker.CountFormattedEntries() == 2);
	}

	SUBCASE("Modify document width")
	{
		Element* container = document->GetElementById("absolute-container");
		Element* element = document->GetElementById("absolute-item");

		const float document_width = document->GetOffsetWidth();
		const float container_width = container->GetOffsetWidth();
		const float element_width = element->GetOffsetWidth();

		document->SetProperty("width", "1000px");
		TestsShell::RenderLoop();

		CHECK(document->GetOffsetWidth() != document_width);
		CHECK(container->GetOffsetWidth() != container_width);
		CHECK(element->GetOffsetWidth() != element_width);
	}

	SUBCASE("Modify normal content")
	{
		Element* element = document->GetElementById("normal-item");
		const float initial_height = element->GetOffsetHeight();

		LogLayoutTree(CaptureLayoutTree(document));

		rmlui_dynamic_cast<ElementText*>(element->GetFirstChild())->SetText("Modified text that is long enough to cause line break");

		TestsShell::RenderLoop();
		CHECK(element->GetOffsetHeight() != initial_height);

		CHECK(format_independent_tracker.CountFormattedEntries() == 1);
	}

	SUBCASE("Modify normal content and absolute content")
	{
		Element* absolute_element = document->GetElementById("absolute-item");
		Element* normal_element = document->GetElementById("normal-item");
		const float absolute_initial_height = absolute_element->GetOffsetHeight();
		const float normal_initial_height = normal_element->GetOffsetHeight();

		rmlui_dynamic_cast<ElementText*>(absolute_element->GetFirstChild())->SetText("Modified text that is long enough to cause line break");
		rmlui_dynamic_cast<ElementText*>(normal_element->GetFirstChild())->SetText("Modified text that is long enough to cause line break");

		TestsShell::RenderLoop();
		CHECK(absolute_element->GetOffsetHeight() != absolute_initial_height);
		CHECK(normal_element->GetOffsetHeight() != normal_initial_height);

		CHECK(format_independent_tracker.CountFormattedEntries() == 2);
	}

	format_independent_tracker.LogMessage();

	document->Close();
	TestsShell::ShutdownShell();
}

static const String layout_isolation_hidden_absolute_rml = R"(
<rml>
<head>
	<title>Demo</title>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body {
			font-family: LatoLatin;
			font-size: 14px;
			border: 5px #a66;
			background: #cca;
			color: black;

			top: 200px;
			right: 300px;
			bottom: 200px;
			left: 300px;
		}
		div {
			width: 300px;
			height: 200px;
			left: 100px;
			top: 100px;
		}
		.hide {
			display: none;
		}
		.absolute {
			position: absolute;
		}
		.wide {
			width: 400px;
		}
	</style>
</head>
<body id="body">
	This is a sample.
	<div id="child">Child element</div>
</body>
</rml>
)";

TEST_CASE("LayoutIsolation.HiddenSkipsFormatting")
{
	Context* context = TestsShell::GetContext();

	ElementDocument* document = context->LoadDocumentFromMemory(layout_isolation_hidden_absolute_rml);
	Element* element = document->GetElementById("child");
	element->SetClass("hide", true);

	SUBCASE("Static") {}
	SUBCASE("Absolute")
	{
		element->SetClass("absolute", true);
	}

	FormatIndependentDebugTracker format_independent_tracker;
	document->Show();
	TestsShell::RenderLoop();
	CHECK(format_independent_tracker.CountFormattedEntries() == 1);

	rmlui_dynamic_cast<ElementText*>(element->GetFirstChild())->SetText("Modified text");
	CHECK(format_independent_tracker.CountFormattedEntries() == 1);

	// Modifying text in a hidden element should not trigger a new layout.
	TestsShell::RenderLoop();
	CHECK(format_independent_tracker.CountFormattedEntries() == 1);

	format_independent_tracker.LogMessage();

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("LayoutIsolation.HiddenToggleAndModify")
{
	Context* context = TestsShell::GetContext();

	ElementDocument* document = context->LoadDocumentFromMemory(layout_isolation_hidden_absolute_rml);
	Element* element = document->GetElementById("child");
	element->SetClass("hide", true);

	float element_offset_left = 0;
	SUBCASE("Static")
	{
		element_offset_left = 305;
	}
	SUBCASE("Absolute")
	{
		element->SetClass("absolute", true);
		element_offset_left = 405;
	}

	document->Show();
	TestsShell::RenderLoop();
	CHECK(element->IsVisible() == false);

	element->SetClass("hide", false);
	document->UpdatePropertiesForDebug();
	TestsShell::RenderLoop();
	CHECK(element->GetAbsoluteLeft() == element_offset_left);
	CHECK(element->IsVisible() == true);
	CHECK(element->GetComputedValues().width().value == 300);
	CHECK(element->GetOffsetWidth() == 300.f);

	element->SetClass("hide", true);
	document->UpdatePropertiesForDebug();
	TestsShell::RenderLoop();
	CHECK(element->IsVisible() == false);
	CHECK(element->GetComputedValues().width().value == 300);
	CHECK(element->GetOffsetWidth() == 300.f);

	element->SetClass("wide", true);
	document->UpdatePropertiesForDebug();
	TestsShell::RenderLoop();
	CHECK(element->IsVisible() == false);
	CHECK(element->GetComputedValues().width().value == 400);
	CHECK(element->GetOffsetWidth() == 300.f);

	element->SetClass("hide", false);
	document->UpdatePropertiesForDebug();
	TestsShell::RenderLoop();
	CHECK(element->IsVisible() == true);
	CHECK(element->GetComputedValues().width().value == 400);
	CHECK(element->GetOffsetWidth() == 400.f);

	document->Close();
	TestsShell::ShutdownShell();
}

static const String layout_isolation_document_rml = R"(
<rml>
<head>
	<title>Demo</title>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body {
			font-family: LatoLatin;
			font-size: 14px;
			border: 5px #a66;
			background: #cca;
			color: black;

			top: 200px;
			right: 300px;
			bottom: 200px;
			left: 300px;
		}
	</style>
</head>
<body>
	This is a sample.
</body>
</rml>
)";

TEST_CASE("LayoutIsolation.Document")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context->GetDimensions() == Rml::Vector2i{1500, 800});

	FormatIndependentDebugTracker format_independent_tracker;

	ElementDocument* document = context->LoadDocumentFromMemory(layout_isolation_document_rml);
	document->Show();

	TestsShell::RenderLoop();

	CHECK(document->GetOffsetWidth() == 900.f);
	CHECK(document->GetOffsetHeight() == 400.f);

	SUBCASE("Modify absolute content")
	{
		context->SetDimensions(Rml::Vector2i{1600, 900});

		document->UpdatePropertiesForDebug();
		TestsShell::RenderLoop();

		CHECK(document->GetOffsetWidth() == 1000.f);
		CHECK(document->GetOffsetHeight() == 500.f);

		CHECK(format_independent_tracker.CountFormattedEntries() == 2);
	}

	format_independent_tracker.LogMessage();

	document->Close();
	TestsShell::ShutdownShell();
}

#endif // RMLUI_DEBUG
