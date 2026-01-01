#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Types.h>
#include <doctest.h>
#include <nanobench.h>

using namespace ankerl;
using namespace Rml;

static constexpr const char* document_rml_template = R"(
<rml>
<head>

	<title>Benchmark Sample</title>
	<link type="text/template" href="/assets/window.rml"/>
	<style>
		body
		{
			font-family: LatoLatin;
			font-weight: normal;
			font-style: normal;
			font-size: 15dp;
			color: white;
		}
		body.window
		{
			max-width: 2000px;
			max-height: 2000px;
			left: 100px;
			top: 50px;
			width: 1300px;
			height: 600px;
		}
		#performance
		{
			width: 800px;
			height: 300px;
		}

/* Insert generated style rules below */
%s

	</style>
</head>

<body template="window">
<div id="performance"/>
</body>
</rml>
)";

static int GetNumDescendentElements(Element* element)
{
	const int num_children = element->GetNumChildren(true);
	int result = num_children;
	for (int i = 0; i < num_children; i++)
	{
		result += GetNumDescendentElements(element->GetChild(i));
	}
	return result;
}

static constexpr int num_rule_iterations = 10;

enum SelectorFlags {
	NO_SELECTOR,
	TAG = 1 << 0,
	ID = 1 << 1,
	CLASS = 1 << 2,
	PSEUDO_CLASS = 1 << 3,
	ATTRIBUTE = 1 << 4,
	NUM_COMBINATIONS = 1 << 5,
};

static String GenerateRCSS(SelectorFlags selectors, const String& complex_selector, String& out_rule_name)
{
	static_assert('a' < 'z' && 'a' + 25 == 'z', "Assumes ASCII characters");

	auto GenerateRule = [=](const String& name) {
		String rule;
		if (!complex_selector.empty())
		{
			rule = complex_selector;
		}
		else if (selectors == NO_SELECTOR || name.empty())
		{
			rule += '*';
		}
		else
		{
			if (selectors & TAG)
			{
				if (selectors == TAG)
					rule += name;
				else
					rule += "div";
			}

			if (selectors & ID)
				rule += '#' + name;
			if (selectors & CLASS)
				rule += '.' + name;
			if (selectors & PSEUDO_CLASS)
				rule += ':' + name;
			if (selectors & ATTRIBUTE)
				rule += '[' + name + ']';
		}

		return rule;
	};

	out_rule_name = GenerateRule("a");

	String result;

	for (int i = 0; i < num_rule_iterations; i++)
	{
		for (char c = 'a'; c <= 'z'; c++)
		{
			const String name(i, c);
			result += GenerateRule(name);

			// Set a property that does not require a layout change
			result += CreateString(" { scrollbar-margin: %dpx; }\n", int(c - 'a') + 1);

#if 1
			// This conditions ensures that only a single version of the complex selector is included. This can be disabled to test how well the rules
			// are de-duplicated, since then a lot more selectors will be tested per update call. Rules that contain sub-selectors are currently not
			// de-duplicated, such as :not().
			if (!complex_selector.empty())
				return result;
#endif
		}
	}

	return result;
}

static String GenerateRml(const int num_rows)
{
	static nanobench::Rng rng;

	Rml::String rml;
	rml.reserve(1000 * num_rows);

	for (int i = 0; i < num_rows; i++)
	{
		int index = rng() % 1000;
		int route = rng() % 50;
		int max = (rng() % 40) + 10;
		int value = rng() % max;
		String class_name_a = char('a' + char(rng() % 26)) + ToString(rng() % num_rule_iterations);
		String class_name_b = char('a' + char(rng() % 26)) + ToString(rng() % num_rule_iterations);
		Rml::String rml_row = Rml::CreateString(R"(
			<div class="row">
				<div class="col col1"><button class="expand" index="%d">+</button>&nbsp;<a>Route %d</a></div>
				<div class="col col23"><input type="range" class="assign_range" min="0" max="%d" value="%d"/></div>
				<div class="col col4 %s">Assigned</div>
				<select>
					<option>Red</option><option>Blue</option><option selected>Green</option><option style="background-color: yellow;">Yellow</option>
				</select>
				<div class="inrow unmark_collapse %s">
					<div class="col col123 assign_text">Assign to route</div>
					<div class="col col4">
						<input type="submit" class="vehicle_depot_assign_confirm" quantity="0">Confirm</input>
					</div>
				</div>
			</div>)",
			index, route, max, value, class_name_a.c_str(), class_name_b.c_str());
		rml += rml_row;
	}

	return rml;
}

TEST_CASE("Selectors")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	constexpr int num_rows = 50;
	const String rml = GenerateRml(num_rows);

	// Benchmark the lookup of applicable style rules for elements.
	//
	// We do this by toggling a pseudo class on an element with a lot of descendent elements. This dirties the style definition for this and all
	// descendent elements, requiring a lookup for applicable nodes on each of them. We repeat this benchmark with different combinations of unique
	// "dummy" style rules added to the style sheet.

	nanobench::Bench bench;
	bench.title("Selector (rule name)");
	bench.timeUnit(std::chrono::microseconds(1), "us");
	bench.relative(true);

	const Vector<String> complex_selectors = {
		"*",
		"div",
		"div div",
		"div > div",
		"div + div",
		"div ~ div",
		":empty div",
		":only-child div",
		":first-child div",
		":nth-child(2n+3) div",
		":nth-of-type(2n+3) div",
		":not(div) div",
		"[class] div",
		"[class=col] div",
		"[class~=col] div",
		"[class|=col] div",
		"[class^=col] div",
		"[class$=col] div",
		"[class*=col] div",
	};

	for (int i = 0; i < NUM_COMBINATIONS + (int)complex_selectors.size(); i++)
	{
		const bool reference = (i == 0);
		const SelectorFlags selector_flags = SelectorFlags(i < NUM_COMBINATIONS ? i : NO_SELECTOR);
		const String& complex_selector = (i < NUM_COMBINATIONS ? String() : complex_selectors[i - NUM_COMBINATIONS]);

		String name, styles;
		if (reference)
			name = "Reference (no style rules)";
		else
			styles = GenerateRCSS(selector_flags, complex_selector, name);

		const String compiled_document_rml = Rml::CreateString(document_rml_template, styles.c_str());

		ElementDocument* document = context->LoadDocumentFromMemory(compiled_document_rml);
		document->Show();

		Element* el = document->GetElementById("performance");
		el->SetInnerRML(rml);
		context->Update();
		context->Render();

		if (reference)
		{
			String msg = Rml::CreateString("\nElement update after pseudo class change with %d descendant elements and %d unique RCSS rules.",
				GetNumDescendentElements(el), num_rule_iterations * 26);
			MESSAGE(msg);

			bench.run("Reference (load document)", [&] {
				ElementDocument* new_document = context->LoadDocumentFromMemory(compiled_document_rml);
				new_document->Close();
				context->Update();
			});
			bench.run("Reference (update unmodified)", [&] { context->Update(); });
		}

		bool hover_active = false;

		bench.run(name.c_str(), [&] {
			hover_active = !hover_active;
			// Toggle some arbitrary pseudo class on the element to dirty the definition on this and all descendent elements.
			el->SetPseudoClass("hover", hover_active);
			context->Update();
		});

		document->Close();
		context->Update();
	}
}
