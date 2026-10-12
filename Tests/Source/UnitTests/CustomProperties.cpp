#include "../Common/TestsInterface.h"
#include "../Common/TestsShell.h"
#include "RmlUi/Core/ComputedValues.h"
#include "RmlUi/Core/DecorationTypes.h"
#include "RmlUi/Core/PropertyDictionary.h"
#include "RmlUi/Core/StyleSheetTypes.h"
#include "RmlUi/Core/Transform.h"
#include "RmlUi/Core/TransformPrimitive.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <Shell.h>
#include <algorithm>
#include <doctest.h>

using namespace Rml;

TEST_CASE("custom_properties.nested_inherited")
{
	static const String basic_rml = R"(
<rml>
<head>
	<style>
	* {
		color: #00ff00;
	}
	body {
		--color-var: #ffffff;
		width: 500px; height: 500px;
	}
	div {
		background-color: var(--color-var);
		--color2-var: var(--color-var);
	}
	p {
		background-color: var(--color2-var);
	}
	</style>
</head>

<body>
	<div id="div">
		<p id="p"></p>
	</div>
</body>
</rml>
)";

	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocumentFromMemory(basic_rml);
	document->Show();

	// basic variable
	Element* div = document->GetElementById("div");
	CHECK(div->GetProperty(PropertyId::BackgroundColor)->ToString() == "#ffffff");
	CHECK(div->GetComputedValues().background_color() == Colourb(255, 255, 255, 255));

	// recursive variable
	Element* p = document->GetElementById("p");
	CHECK(p->GetProperty(PropertyId::BackgroundColor)->ToString() == "#ffffff");
	CHECK(p->GetComputedValues().background_color() == Colourb(255, 255, 255, 255));

	// variable modification
	div->SetProperty("--color-var", "#000000");
	TestsShell::RenderLoop();

	CHECK(div->GetProperty(PropertyId::BackgroundColor)->ToString() == "#000000");
	CHECK(div->GetComputedValues().background_color() == Colourb(0, 0, 0, 255));

	// inheritance validation
	CHECK(div->GetProperty("--color-var")->ToString() == "#000000");
	CHECK(document->GetProperty("--color-var")->ToString() == "#ffffff");

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("custom_properties.invalid_substitution")
{
	static const String basic_rml = R"(
<rml>
<head>
	<style>
	body {
		background-color: var(--invalid-var);
	}
	</style>
</head>

<body>
</body>
</rml>
)";

	Context* context = TestsShell::GetContext();

	TestsShell::SetNumExpectedWarnings(3);

	ElementDocument* document = context->LoadDocumentFromMemory(basic_rml);
	document->Show();

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("custom_properties.inheritance_change_variable")
{
	static const String rml = R"(
<rml>
<head>
	<style>
	body {
		color: #fff;
		--color-var: #f00;
	}
	div {
		color: var(--color-var);
	}
	#p2 {
		--color-var: #0f0;
	}
	</style>
</head>

<body>
<div id="div">
	<p id="p1"></p>
	<p id="p2"></p>
	<p id="p3" style="--color-var: #00f"></p>
</div>
</body>
</rml>
)";

	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocumentFromMemory(rml);
	document->Show();

	TestsShell::RenderLoop();

	Element* div = document->GetElementById("div");
	Element* p1 = document->GetElementById("p1");
	Element* p2 = document->GetElementById("p2");
	Element* p3 = document->GetElementById("p3");

	CHECK(div->GetComputedValues().color() == Colourb(255, 0, 0, 255));
	CHECK(p1->GetComputedValues().color() == Colourb(255, 0, 0, 255));
	CHECK(p2->GetComputedValues().color() == Colourb(255, 0, 0, 255));
	CHECK(p3->GetComputedValues().color() == Colourb(255, 0, 0, 255));

	div->SetProperty("--color-var", "#000");
	TestsShell::RenderLoop();

	CHECK(div->GetComputedValues().color() == Colourb(0, 0, 0, 255));
	CHECK(p1->GetComputedValues().color() == Colourb(0, 0, 0, 255));
	CHECK(p2->GetComputedValues().color() == Colourb(0, 0, 0, 255));
	CHECK(p3->GetComputedValues().color() == Colourb(0, 0, 0, 255));

	div->SetProperty("color", "#fff");
	TestsShell::RenderLoop();

	CHECK(div->GetComputedValues().color() == Colourb(255, 255, 255, 255));
	CHECK(p1->GetComputedValues().color() == Colourb(255, 255, 255, 255));
	CHECK(p2->GetComputedValues().color() == Colourb(255, 255, 255, 255));
	CHECK(p3->GetComputedValues().color() == Colourb(255, 255, 255, 255));

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("custom_properties.shorthands")
{
	static const String shorthand_rml = R"(
<rml>
<head>
	<style>
	body {
		--padding-var: 20px 5px;
		--v-padding-var: 3px;
		--h-padding-var: 7px;
	}
	div {
		padding: var(--padding-var);
	}
	p {
		padding: var(--v-padding-var) var(--h-padding-var);
	}
	</style>
</head>

<body id="body">
<div id="div"></div>
<p id="p"></p>
</body>
</rml>
)";

	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocumentFromMemory(shorthand_rml);
	document->Show();

	Element* div = document->GetElementById("div");
	CHECK(div->GetComputedValues().padding_top().type == Style::LengthPercentage::Type::Length);
	CHECK(div->GetComputedValues().padding_top().value == 20);
	CHECK(div->GetComputedValues().padding_right().type == Style::LengthPercentage::Type::Length);
	CHECK(div->GetComputedValues().padding_right().value == 5);
	CHECK(div->GetProperty(PropertyId::PaddingTop)->ToString() == "20px");
	CHECK(div->GetProperty(PropertyId::PaddingRight)->ToString() == "5px");

	// variable modification and shorthand override
	div->SetProperty(PropertyId::PaddingTop, Property(6, Unit::PX));
	TestsShell::RenderLoop();

	CHECK(div->GetComputedValues().padding_top().type == Style::LengthPercentage::Type::Length);
	CHECK(div->GetComputedValues().padding_top().value == 6);
	CHECK(div->GetProperty(PropertyId::PaddingTop)->ToString() == "6px");

	// Change shorthand
	div->SetProperty("--padding-var", "15px 0px");
	div->RemoveProperty(PropertyId::PaddingTop);
	TestsShell::RenderLoop();

	CHECK(div->GetComputedValues().padding_top().type == Style::LengthPercentage::Type::Length);
	CHECK(div->GetComputedValues().padding_top().value == 15);
	CHECK(div->GetProperty(PropertyId::PaddingTop)->ToString() == "15px");

	// Multi-var shorthand
	Element* p = document->GetElementById("p");
	CHECK(p->GetComputedValues().padding_bottom().type == Style::LengthPercentage::Type::Length);
	CHECK(p->GetComputedValues().padding_bottom().value == 3);
	CHECK(p->GetComputedValues().padding_left().type == Style::LengthPercentage::Type::Length);
	CHECK(p->GetComputedValues().padding_left().value == 7);
	CHECK(p->GetProperty(PropertyId::PaddingBottom)->ToString() == "3px");
	CHECK(p->GetProperty(PropertyId::PaddingLeft)->ToString() == "7px");

	document->SetProperty("--v-padding-var", "1px");
	TestsShell::RenderLoop();

	CHECK(p->GetComputedValues().padding_bottom().type == Style::LengthPercentage::Type::Length);
	CHECK(p->GetComputedValues().padding_bottom().value == 1);
	CHECK(p->GetComputedValues().padding_left().type == Style::LengthPercentage::Type::Length);
	CHECK(p->GetComputedValues().padding_left().value == 7);
	CHECK(p->GetProperty(PropertyId::PaddingBottom)->ToString() == "1px");
	CHECK(p->GetProperty(PropertyId::PaddingLeft)->ToString() == "7px");

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("custom_properties.shorthand_font")
{
	static const String shorthand_rml = R"(
<rml>
<head>
	<style>
	body {
		--value: normal 20px;
		font-size: 10px;
		margin-top: 50px;
	}
	div {
		font: italic var(--value) "LatoLatin";
	}
    .large {
		font-size: 40px;
	}
	</style>
</head>

<body id="body">
	<div id="div">
		<p id="p">Text</p>
	</div>
</body>
</rml>
)";

	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocumentFromMemory(shorthand_rml);
	document->Show();

	Element* div = document->GetElementById("div");
	Element* p = document->GetElementById("p");

	SUBCASE("preliminary checks")
	{
		TestsShell::RenderLoop();
		CHECK(div->GetComputedValues().font_size() == 20);
		CHECK(div->GetComputedValues().font_weight() == Style::FontWeight::Normal);
		CHECK(div->GetComputedValues().font_style() == Style::FontStyle::Italic);
		CHECK(div->GetComputedValues().font_family() == "latolatin");
		CHECK(div->GetComputedValues().line_height().value == 24);

		CHECK(p->GetComputedValues().font_size() == 20);
		CHECK(p->GetComputedValues().font_weight() == Style::FontWeight::Normal);
		CHECK(p->GetComputedValues().font_style() == Style::FontStyle::Italic);
		CHECK(p->GetComputedValues().font_family() == "latolatin");
		CHECK(p->GetComputedValues().line_height().value == 24);
	}

	SUBCASE("modify variable on document")
	{
		document->SetProperty("--value", "normal 30px");
		TestsShell::RenderLoop();
		CHECK(div->GetComputedValues().font_size() == 30);
		CHECK(div->GetComputedValues().line_height().value == 36);
		CHECK(p->GetComputedValues().line_height().value == 36);
	}

	SUBCASE("modify variable on div")
	{
		div->SetProperty("--value", "normal 30px");
		TestsShell::RenderLoop();
		CHECK(div->GetComputedValues().font_size() == 30);
		CHECK(div->GetComputedValues().line_height().value == 36);
		CHECK(p->GetComputedValues().line_height().value == 36);
	}

	SUBCASE("modify variable to affect a different property")
	{
		document->SetProperty("--value", "bold 12px");
		TestsShell::RenderLoop();
		CHECK(div->GetComputedValues().font_size() == 12);
		CHECK(div->GetComputedValues().font_weight() == Style::FontWeight::Bold);
		CHECK(div->GetComputedValues().line_height().value == 12.0f * 1.2f);
		CHECK(p->GetComputedValues().font_weight() == Style::FontWeight::Bold);
		CHECK(p->GetComputedValues().line_height().value == 12.0f * 1.2f);
	}

	SUBCASE("overriding a single property that's also in a shorthand")
	{
		div->SetClass("large", true);
		TestsShell::RenderLoop();
		CHECK(div->GetComputedValues().font_size() == 40);
		CHECK(div->GetComputedValues().font_weight() == Style::FontWeight::Normal);
		CHECK(div->GetComputedValues().line_height().value == 48);
		CHECK(div->GetComputedValues().font_size() == 40);
		CHECK(p->GetComputedValues().font_size() == 40);
		CHECK(p->GetComputedValues().font_weight() == Style::FontWeight::Normal);
		CHECK(p->GetComputedValues().line_height().value == 48);

		document->SetProperty("--value", "bold 12px");
		TestsShell::RenderLoop();
		CHECK(div->GetComputedValues().font_size() == 40);
		CHECK(div->GetComputedValues().font_weight() == Style::FontWeight::Bold);
		CHECK(div->GetComputedValues().line_height().value == 48);
		CHECK(p->GetComputedValues().font_size() == 40);
		CHECK(p->GetComputedValues().font_weight() == Style::FontWeight::Bold);
		CHECK(p->GetComputedValues().line_height().value == 48);
	}

	SUBCASE("overriding a single property in a parent should have no effect")
	{
		document->SetClass("large", true);
		TestsShell::RenderLoop();
		CHECK(div->GetComputedValues().font_size() == 20);
		CHECK(div->GetComputedValues().font_weight() == Style::FontWeight::Normal);
		CHECK(div->GetComputedValues().font_style() == Style::FontStyle::Italic);
		CHECK(div->GetComputedValues().font_family() == "latolatin");
		CHECK(div->GetComputedValues().line_height().value == 24);
	}

	SUBCASE("overriding a single property on a child should apply that value")
	{
		p->SetClass("large", true);
		TestsShell::RenderLoop();
		CHECK(p->GetComputedValues().font_size() == 40);
		CHECK(p->GetComputedValues().font_weight() == Style::FontWeight::Normal);
		CHECK(p->GetComputedValues().line_height().value == 48);
	}

	SUBCASE("child should still inherit font properties it does not assign itself")
	{
		p->SetClass("large", true);
		TestsShell::RenderLoop();
		div->SetProperty("--value", "bold 12px");
		TestsShell::RenderLoop();
		CHECK(p->GetComputedValues().font_size() == 40);
		CHECK(p->GetComputedValues().font_weight() == Style::FontWeight::Bold);
		CHECK(p->GetComputedValues().line_height().value == 48);
	}

	SUBCASE("overriding the variable on a child has no effect")
	{
		p->SetProperty("--value", "bold 12px");
		TestsShell::RenderLoop();
		CHECK(p->GetComputedValues().font_size() == 20);
		CHECK(p->GetComputedValues().font_weight() == Style::FontWeight::Normal);
	}

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("custom_properties.partially_override_shorthand")
{
	static const String shorthand_rml = R"(
<rml>
<head>
	<style>
	body {
		--value: normal 20px;
	}
    .bold-before {
		font-weight: bold;
	}
	.div {
		font: italic var(--value) "LatoLatin";
	}
    .bold {
		font-weight: bold;
	}
	</style>
</head>

<body id="body">
	<div id="div" class="div">
		<p id="p">Text</p>
	</div>
</body>
</rml>
)";

	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocumentFromMemory(shorthand_rml);
	document->Show();

	Element* div = document->GetElementById("div");
	Element* p = document->GetElementById("p");

	SUBCASE("preliminary checks")
	{
		CHECK(div->GetComputedValues().font_weight() == Style::FontWeight::Normal);
		CHECK(p->GetComputedValues().font_weight() == Style::FontWeight::Normal);
	}
	SUBCASE("bold on div")
	{
		div->SetClass("bold", true);
		TestsShell::RenderLoop();
		CHECK(div->GetComputedValues().font_weight() == Style::FontWeight::Bold);
		CHECK(p->GetComputedValues().font_weight() == Style::FontWeight::Bold);
	}
	SUBCASE("bold on p")
	{
		p->SetClass("bold", true);
		TestsShell::RenderLoop();
		CHECK(div->GetComputedValues().font_weight() == Style::FontWeight::Normal);
		CHECK(p->GetComputedValues().font_weight() == Style::FontWeight::Bold);
	}
	SUBCASE("bold on body")
	{
		document->SetClass("bold", true);
		TestsShell::RenderLoop();
		CHECK(div->GetComputedValues().font_weight() == Style::FontWeight::Normal);
		CHECK(p->GetComputedValues().font_weight() == Style::FontWeight::Normal);
	}
	SUBCASE("bold-before has lower specifity, ignored")
	{
		div->SetClass("bold-before", true);
		TestsShell::RenderLoop();
		CHECK(div->GetComputedValues().font_weight() == Style::FontWeight::Normal);
		CHECK(p->GetComputedValues().font_weight() == Style::FontWeight::Normal);
	}
	SUBCASE("bold-before on p")
	{
		p->SetClass("bold-before", true);
		TestsShell::RenderLoop();
		CHECK(div->GetComputedValues().font_weight() == Style::FontWeight::Normal);
		CHECK(p->GetComputedValues().font_weight() == Style::FontWeight::Bold);
	}

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("custom_properties.data_model")
{
	static const String data_model_rml = R"(
<rml>
<head>
	<style>
		div {
			background-color: var(--bg-color);
		}
	</style>
</head>

<body data-model="vars">
<div id="div" data-style---bg-color="bg_color"></div>
</body>
</rml>
)";

	Context* context = TestsShell::GetContext();

	DataModelConstructor model = context->CreateDataModel("vars");
	String bg_color = "";
	model.Bind("bg_color", &bg_color);

	TestsShell::SetNumExpectedWarnings(1); // Invalid value during substitution.
	ElementDocument* document = context->LoadDocumentFromMemory(data_model_rml);
	document->Show();
	Element* div = document->GetElementById("div");
	CHECK(div->GetComputedValues().background_color() == Colourb(0, 0, 0, 0));

	TestsShell::SetNumExpectedWarnings(0);
	bg_color = "#f00";
	model.GetModelHandle().DirtyVariable("bg_color");
	TestsShell::RenderLoop();
	CHECK(div->GetComputedValues().background_color() == Colourb(255, 0, 0, 255));

	bg_color = "#0f0";
	model.GetModelHandle().DirtyVariable("bg_color");
	TestsShell::RenderLoop();
	CHECK(div->GetComputedValues().background_color() == Colourb(0, 255, 0, 255));

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("custom_properties.nested")
{
	static const String inheritance_rml = R"(
<rml>
<head>
	<style>
	body {
		--inner: 10px;
		font-size: 20px;
	}
	div {
		--outer: var(--inner);
	}
	p {
		font-size: var(--outer);
	}
	</style>
</head>

<body>
<div id="div">
	<p id="p"></p>
</div>
</body>
</rml>
)";

	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocumentFromMemory(inheritance_rml);
	document->Show();

	Element* div = document->GetElementById("div");
	Element* p = document->GetElementById("p");

	CHECK(p->GetProperty("--outer")->ToString() == "10px");
	CHECK(p->GetProperty("--inner")->ToString() == "10px");
	CHECK(p->GetProperty("font-size")->ToString() == "10px");
	CHECK(p->GetProperty(PropertyId::FontSize)->ToString() == "10px");
	CHECK(p->GetComputedValues().font_size() == 10.f);

	CHECK(div->GetProperty("--outer")->ToString() == "10px");
	CHECK(div->GetProperty("--inner")->ToString() == "10px");
	CHECK(div->GetProperty("font-size")->ToString() == "20px");
	CHECK(div->GetProperty(PropertyId::FontSize)->ToString() == "20px");
	CHECK(div->GetComputedValues().font_size() == 20.f);

	CHECK(document->GetProperty("--outer") == nullptr);
	CHECK(document->GetProperty("--inner")->ToString() == "10px");
	CHECK(document->GetProperty("font-size")->ToString() == "20px");
	CHECK(document->GetProperty(PropertyId::FontSize)->ToString() == "20px");
	CHECK(document->GetComputedValues().font_size() == 20.f);

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("custom_properties.inheritance")
{
	static const String inheritance_rml = R"(
<rml>
<head>
	<style>
	body {
		--bg-color: #fff;
	}
	div {
		--bg-color: #00ff00
	}
	p {
		background-color: var(--bg-color);
	}
	</style>
</head>

<body>
<div>
	<p id="p1"></p>
</div>
<p id="p2"></p>
</body>
</rml>
)";

	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocumentFromMemory(inheritance_rml);
	document->Show();

	CHECK(document->GetElementById("p1")->GetProperty(PropertyId::BackgroundColor)->ToString() == "#00ff00");
	CHECK(document->GetElementById("p1")->GetComputedValues().background_color() == Colourb(0, 255, 0, 255));

	CHECK(document->GetElementById("p2")->GetProperty(PropertyId::BackgroundColor)->ToString() == "#ffffff");
	CHECK(document->GetElementById("p2")->GetComputedValues().background_color() == Colourb(255, 255, 255, 255));

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("custom_properties.circular")
{
	static const String circular_rml = R"(
<rml>
<head>
	<style>
	body {
		--bg-color: var(--bg2-color);
		--bg2-color: var(--bg-color);
		background-color: var(--bg-color);
	}
	</style>
</head>

<body>
<div></div>
</body>
</rml>
)";

	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	// Should get error for resolution failure of the second variable (cycle detected)
	TestsShell::SetNumExpectedWarnings(3);

	ElementDocument* document = context->LoadDocumentFromMemory(circular_rml);
	document->Show();

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("custom_properties.semicircular")
{
	static const String rml = R"(
<rml>
<head>
	<style>
	body {
		--color-a: var(--color-b);
		--color-b: #f00;
		color: var(--color-a);
	}
	div {
		/* This is perfectly valid since --color-a refers to the parent's --color-b */ 
		--color-b: var(--color-a);
		color: var(--color-b);
	}
	span {
		--color-a: #00f;
		color: var(--color-b);
	}
	</style>
</head>

<body>
	<div id="div">
		<span id="span"></span>
	</div>
</body>
</rml>
)";

	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocumentFromMemory(rml);
	document->Show();

	CHECK(document->GetComputedValues().color() == Colourb(255, 0, 0, 255));
	CHECK(document->GetElementById("div")->GetComputedValues().color() == Colourb(255, 0, 0, 255));
	CHECK(document->GetElementById("span")->GetComputedValues().color() == Colourb(255, 0, 0, 255));

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("custom_properties.order")
{
	static const String order_rml = R"(
<rml>
<head>
	<style>
	body {
		--bg1-color: var(--bg2-color);
		--bg2-color: var(--bg3-color);
		--bg3-color: #ffffff;
		background-color: var(--bg1-color);
	}
	</style>
</head>

<body>
<div></div>
</body>
</rml>
)";

	Context* context = TestsShell::GetContext();

	// Should succeed showcasing order-independence of variable definition and usage
	ElementDocument* document = context->LoadDocumentFromMemory(order_rml);
	document->Show();

	CHECK(document->GetComputedValues().background_color() == Colourb(255, 255, 255, 255));

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("custom_properties.transition")
{
	static const String transition_rml = R"(
<rml>
<head>
	<style>
		div {
			background-color: red;
			transition: all 0.2s;
		}

		div.active {
			--color: blue;
			background-color: var(--color);
		}
	</style>
</head>

<body>
<div></div>
</body>
</rml>
)";

	TestsSystemInterface* system_interface = TestsShell::GetTestsSystemInterface();
	Context* context = TestsShell::GetContext();
	system_interface->SetManualTime(0.0);

	ElementDocument* document = context->LoadDocumentFromMemory(transition_rml);
	document->Show();

	Element* div = document->GetFirstChild();
	CHECK(div->GetComputedValues().background_color() == Colourb(255, 0, 0, 255));

	div->SetClass("active", true);
	TestsShell::RenderLoop();

	system_interface->SetManualTime(0.1);
	TestsShell::RenderLoop();
	CHECK(div->GetComputedValues().background_color() == Colourb(180, 0, 180, 255));

	system_interface->SetManualTime(0.3);
	TestsShell::RenderLoop();
	CHECK(div->GetComputedValues().background_color() == Colourb(0, 0, 255, 255));

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("custom_properties.transition_deep")
{
	static const String transition_deep_rml = R"(
<rml>
<head>
	<style>
		div {
			background-color: red;
			transition: all 0.2s;
		}

		div.active {
			--new-color: blue;
			--color: var(--new-color);
			background-color: var(--color);
		}
	</style>
</head>

<body>
<div></div>
</body>
</rml>
)";

	TestsSystemInterface* system_interface = TestsShell::GetTestsSystemInterface();
	Context* context = TestsShell::GetContext();
	system_interface->SetManualTime(0.0);

	ElementDocument* document = context->LoadDocumentFromMemory(transition_deep_rml);
	document->Show();

	Element* div = document->GetFirstChild();
	CHECK(div->GetComputedValues().background_color() == Colourb(255, 0, 0, 255));

	div->SetClass("active", true);
	TestsShell::RenderLoop();

	system_interface->SetManualTime(0.1);
	TestsShell::RenderLoop();
	CHECK(div->GetComputedValues().background_color() == Colourb(180, 0, 180, 255));

	system_interface->SetManualTime(0.3);
	TestsShell::RenderLoop();
	CHECK(div->GetComputedValues().background_color() == Colourb(0, 0, 255, 255));

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("custom_properties.shorthand_transition")
{
	static const String document_rml_template = R"(
<rml>
<head>
	<style>
%s
	</style>
</head>

<body>
<div></div>
</body>
</rml>
)";

	const char* rcss_rules = nullptr;
	SUBCASE("A")
	{
		rcss_rules = R"(
			div {
				padding: 10px;
				transition: all 0.2s;
			}
			div.active {
				--padding: 20px;
				padding: var(--padding);
			}
		)";
	}
	SUBCASE("B")
	{
		rcss_rules = R"(
			div {
				--padding: 10px;
				padding: var(--padding);
				transition: padding 0.2s;
			}
			div.active {
				--padding: 20px;
				padding: var(--padding);
			}
		)";
	}
	SUBCASE("C")
	{
		rcss_rules = R"(
			div {
				--padding: 10px;
				padding: var(--padding);
				transition: padding 0.2s;
			}
			div.active {
				--indirect: 20px;
				--padding: var(--indirect);
				padding: var(--padding);
			}
		)";
	}

	TestsSystemInterface* system_interface = TestsShell::GetTestsSystemInterface();
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	system_interface->SetManualTime(0.0);

	ElementDocument* document = context->LoadDocumentFromMemory(CreateString(document_rml_template.c_str(), rcss_rules));
	document->Show();

	Element* div = document->GetFirstChild();
	CHECK(div->GetProperty<float>("padding-top") == 10.f);

	div->SetClass("active", true);
	TestsShell::RenderLoop();

	system_interface->SetManualTime(0.1);
	TestsShell::RenderLoop();
	CHECK(div->GetProperty<float>("padding-top") == doctest::Approx(15.f));

	system_interface->SetManualTime(0.3);
	TestsShell::RenderLoop();
	CHECK(div->GetProperty<float>("padding-top") == 20.f);

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("custom_properties.animation")
{
	static const String animation_rml = R"(
<rml>
<head>
	<style>
		@keyframes change-color {
			from { background-color: var(--from-color); }
			to { background-color: var(--to-color); }
		}
		div {
			--from-color: #f00;
			--to-color: #00f;
			background-color: #0f0;
			animation: 0.2s linear change-color;
		}
	</style>
</head>

<body>
<div></div>
</body>
</rml>
)";

	TestsSystemInterface* system_interface = TestsShell::GetTestsSystemInterface();
	Context* context = TestsShell::GetContext();
	system_interface->SetManualTime(0.0);

	ElementDocument* document = context->LoadDocumentFromMemory(animation_rml);
	Element* div = document->GetFirstChild();
	document->Show();
	CHECK(div->GetComputedValues().background_color() == Colourb(0, 255, 0, 255));

	system_interface->SetManualTime(0.1);
	TestsShell::RenderLoop();
	CHECK(div->GetComputedValues().background_color() == Colourb(180, 0, 180, 255));

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("custom_properties.shorthand_animation")
{
	static const String shorthand_animation_rml = R"(
<rml>
<head>
	<style>
		@keyframes pad {
			from { padding: var(--from-padding); }
			to { padding: var(--to-padding); }
		}
		div {
			--from-padding: 10px;
			--to-padding: 20px 0px;
			padding: 0;
			animation: 0.2s linear pad;
		}
	</style>
</head>

<body>
<div></div>
</body>
</rml>
)";

	TestsSystemInterface* system_interface = TestsShell::GetTestsSystemInterface();
	Context* context = TestsShell::GetContext();
	system_interface->SetManualTime(0.0);

	ElementDocument* document = context->LoadDocumentFromMemory(shorthand_animation_rml);
	document->Show();
	TestsShell::RenderLoop();

	Element* div = document->GetFirstChild();

	system_interface->SetManualTime(0.1);
	TestsShell::RenderLoop();
	CHECK(div->GetComputedValues().padding_top().value == doctest::Approx(15.f));
	CHECK(div->GetComputedValues().padding_right().value == doctest::Approx(5.f));

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("custom_properties.combined_shorthand_animation")
{
	static const String shorthand_animation_rml = R"(
<rml>
<head>
	<style>
		@keyframes pad {
			from { padding-bottom: 30px; }
			to { padding: var(--to-padding); }
		}
		div {
			--from-padding: 10px;
			--to-padding: 20px 0px;
			padding: var(--from-padding);
			animation: 0.2s linear pad;
		}
	</style>
</head>

<body>
<div></div>
</body>
</rml>
)";

	TestsSystemInterface* system_interface = TestsShell::GetTestsSystemInterface();
	Context* context = TestsShell::GetContext();
	system_interface->SetManualTime(0.0);

	ElementDocument* document = context->LoadDocumentFromMemory(shorthand_animation_rml);
	document->Show();
	TestsShell::RenderLoop();

	Element* div = document->GetFirstChild();

	system_interface->SetManualTime(0.1);
	TestsShell::RenderLoop();
	CHECK(div->GetComputedValues().padding_top().value == doctest::Approx(15.f));
	CHECK(div->GetComputedValues().padding_right().value == doctest::Approx(5.f));
	CHECK(div->GetComputedValues().padding_bottom().value == doctest::Approx(25.f));

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("custom_properties.get_property_roundtrip")
{
	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->CreateDocument();

	CHECK(document->GetProperty("--primary") == nullptr);

	document->SetProperty("--primary", "red");
	CHECK(document->GetProperty("--primary")->ToString() == "red");
	CHECK(document->GetLocalProperty("--primary")->ToString() == "red");

	document->SetProperty("--primary", "blue");
	CHECK(document->GetProperty("--primary")->ToString() == "blue");
	CHECK(document->GetLocalProperty("--primary")->ToString() == "blue");

	document->SetProperty("--secondary", "16px");
	CHECK(document->GetProperty("--primary")->ToString() == "blue");
	CHECK(document->GetProperty("--secondary")->ToString() == "16px");

	document->SetProperty("--tertiary", "red white var(--primary)");
	context->Update();
	CHECK(document->GetProperty("--tertiary")->ToString() == "red white blue");
	CHECK(document->GetLocalProperty("--tertiary")->ToString() == "red white var(--primary)");

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("custom_properties.fallback")
{
	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->CreateDocument();

	SUBCASE("fallback wins when variable undefined")
	{
		REQUIRE(document->SetProperty("background-color", "var(--missing, blue)"));
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(0, 0, 255, 255));
	}

	SUBCASE("named variable wins over fallback when both present")
	{
		document->SetProperty("--accent", "lime");
		REQUIRE(document->SetProperty("background-color", "var(--accent, magenta)"));
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(0, 255, 0, 255));
	}

	SUBCASE("variable value containing var() is recursively resolved")
	{
		document->SetProperty("--accent", "red");
		document->SetProperty("--bg", "var(--accent)");
		REQUIRE(document->SetProperty("background-color", "var(--bg)"));
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(255, 0, 0, 255));
	}

	SUBCASE("nested fallback resolves when outer variable missing")
	{
		document->SetProperty("--inner", "yellow");
		REQUIRE(document->SetProperty("background-color", "var(--missing, var(--inner, magenta))"));
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(255, 255, 0, 255));
	}

	SUBCASE("named variable wins over fallback containing a function")
	{
		document->SetProperty("--accent", "lime");
		REQUIRE(document->SetProperty("background-color", "var(--accent, rgb(255, 0, 255))"));
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(0, 255, 0, 255));
	}

	SUBCASE("fallback containing a function is used when variable undefined")
	{
		REQUIRE(document->SetProperty("background-color", "var(--missing, rgb(255, 0, 255))"));
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(255, 0, 255, 255));
	}

	SUBCASE("named variable wins over nested var() fallback")
	{
		document->SetProperty("--accent", "lime");
		document->SetProperty("--other", "#f0f");
		REQUIRE(document->SetProperty("background-color", "var(--accent, var(--other))"));
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(0, 255, 0, 255));
	}

	SUBCASE("nested var() resolves in fallback")
	{
		document->SetProperty("--other", "#f0f");
		REQUIRE(document->SetProperty("background-color", "var(--accent, var(--other))"));
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(255, 0, 255, 255));
	}

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("custom_properties.media_query")
{
	static const String media_query_rml = R"(
<rml>
<head>
	<style>
	@media (min-width: 600px) {
		body {
			--bg-color: 255,255,255;
		}
	}
	@media (min-width: 800px) {
		body {
			--bg-color: 0,255,0;
		}
	}
	div {
		background-color: rgba(var(--bg-color), 127);
	}
	</style>
</head>

<body>
	<div id="div"/>
</body>
</rml>
)";

	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocumentFromMemory(media_query_rml);
	document->Show();

	context->SetDimensions(Vector2i(800, 320));
	TestsShell::RenderLoop();
	CHECK(document->GetElementById("div")->GetComputedValues().background_color() == Colourb(0, 255, 0, 127));

	context->SetDimensions(Vector2i(600, 320));
	TestsShell::RenderLoop();
	CHECK(document->GetElementById("div")->GetComputedValues().background_color() == Colourb(255, 255, 255, 127));

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("custom_properties.media_theme_integration")
{
	static const String rml_source = R"(
<rml>
<head>
	<style>
	@media (theme: dark) {
		div { --primary: red; }
	}
	</style>
</head>
<body>
	<div/>
</body>
</rml>
)";

	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocumentFromMemory(rml_source);
	Element* div = document->GetChild(0);
	REQUIRE(div->SetProperty("background-color", "var(--primary, blue)"));

	SUBCASE("variable from @media block is absent when theme is inactive")
	{
		context->Update();
		CHECK(div->GetComputedValues().background_color() == Colourb(0, 0, 255, 255));
	}

	SUBCASE("variable from @media block appears on activate, clears on deactivate")
	{
		context->ActivateTheme("dark", true);
		context->Update();
		CHECK(div->GetComputedValues().background_color() == Colourb(255, 0, 0, 255));

		context->ActivateTheme("dark", false);
		context->Update();
		CHECK(div->GetComputedValues().background_color() == Colourb(0, 0, 255, 255));
	}

	SUBCASE("programmatic SetVariable persists across theme toggles")
	{
		div->SetProperty("--primary", "lime");
		context->Update();
		CHECK(div->GetComputedValues().background_color() == Colourb(0, 255, 0, 255));

		context->ActivateTheme("dark", true);
		context->Update();
		CHECK(div->GetComputedValues().background_color() == Colourb(0, 255, 0, 255));

		context->ActivateTheme("dark", false);
		context->Update();
		CHECK(div->GetComputedValues().background_color() == Colourb(0, 255, 0, 255));
	}

	SUBCASE("document RemoveVariable falls through to RCSS @media value")
	{
		div->SetProperty("--primary", "lime");
		context->ActivateTheme("dark", true);
		context->Update();
		CHECK(div->GetComputedValues().background_color() == Colourb(0, 255, 0, 255));

		div->RemoveProperty("--primary");
		context->Update();
		CHECK(div->GetComputedValues().background_color() == Colourb(255, 0, 0, 255));
	}

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("custom_properties.body_media_combine")
{
	static const String rml_source = R"(
<rml>
<head>
	<style>
	body {
		--bg: blue;
		--width: 16px;
		background-color: var(--bg);
		border-top-width: var(--width);
	}
	@media (theme: dark) {
		body {
			--bg: red;
		}
	}
	</style>
</head>
<body/>
</rml>
)";

	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocumentFromMemory(rml_source);
	REQUIRE(document->GetComputedValues().background_color() == Colourb(0, 0, 255, 255));
	REQUIRE(document->GetComputedValues().border_top_width() == 16);

	SUBCASE("body var survives @media activation")
	{
		context->ActivateTheme("dark", true);
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(255, 0, 0, 255));
		CHECK(document->GetComputedValues().border_top_width() == 16);
	}

	SUBCASE("body var still present after @media deactivation")
	{
		context->ActivateTheme("dark", true);
		context->Update();
		context->ActivateTheme("dark", false);
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(0, 0, 255, 255));
		CHECK(document->GetComputedValues().border_top_width() == 16);
	}

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("custom_properties.variable_resolution_edge_cases")
{
	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->CreateDocument();

	SUBCASE("multiple var() refs inside one function expression")
	{
		document->SetProperty("--r", "255");
		document->SetProperty("--g", "128");
		document->SetProperty("--b", "0");
		REQUIRE(document->SetProperty("background-color", "rgb(var(--r), var(--g), var(--b))"));
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(255, 128, 0, 255));
	}

	SUBCASE("var() inside a shorthand component is resolved per-component")
	{
		document->SetProperty("--col", "red");
		REQUIRE(document->SetProperty("border-color", "var(--col)"));
		context->Update();
		CHECK(document->GetComputedValues().border_top_color() == Colourb(255, 0, 0, 255));
	}

	SUBCASE("late variable applies after being set")
	{
		TestsShell::SetNumExpectedWarnings(1);
		REQUIRE(document->SetProperty("background-color", "var(--late)"));
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(0, 0, 0, 0));

		TestsShell::SetNumExpectedWarnings(0);
		document->SetProperty("--late", "lime");
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(0, 255, 0, 255));

		document->SetProperty("--late", "red");
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(255, 0, 0, 255));
	}

	SUBCASE("equivalent sibling var() are not detected as a false cycle")
	{
		document->SetProperty("--leaf", "10px");
		document->SetProperty("--a", "var(--leaf)");
		document->SetProperty("--b", "var(--leaf)");
		REQUIRE(document->SetProperty("padding", "var(--a) var(--b)"));
		context->Update();
		CHECK(document->GetComputedValues().padding_top().value == 10);
		CHECK(document->GetComputedValues().padding_right().value == 10);
		CHECK(document->GetComputedValues().padding_bottom().value == 10);
		CHECK(document->GetComputedValues().padding_left().value == 10);
	}

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("custom_properties.misc_properties")
{
	// Some of the following properties resolve variables directly from the ElementStyle.
	static const char* const rml_template = R"(
<rml>
<head>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
	body {
		inset: 0;
		background: #ccc;
	}
%s
	</style>
</head>
<body>
	<div/>
</body>
</rml>
)";

	Context* context = TestsShell::GetContext();
	ElementDocument* document = nullptr;

	SUBCASE("box-shadow")
	{
		document = context->LoadDocumentFromMemory(CreateString(rml_template, R"(
	body { --shadow-80: #0f0; }
	div {
		width: 500px;
		height: 400px;
		background: red;
		box-shadow: var(--shadow-80) 0dp 8dp 24dp 0dp;
	}
)"));
		document->Show();
		Element* div = document->GetFirstChild();

		CHECK(div->GetComputedValues().has_box_shadow());
		CHECK(div->GetProperty(PropertyId::BoxShadow)->Get<BoxShadowList>() ==
			BoxShadowList{BoxShadow{
				ColourbPremultiplied{0, 255, 0, 255},
				NumericValue{0, Unit::DP},
				NumericValue{8, Unit::DP},
				NumericValue{24, Unit::DP},
				NumericValue{0, Unit::DP},
				false,
			}});
	}

	SUBCASE("flex")
	{
		document = context->LoadDocumentFromMemory(CreateString(rml_template, R"(
	body { --flex-value: 2 3 40px; display: flex; }
	div { flex: var(--flex-value); }
)"));
		document->Show();
		Element* div = document->GetFirstChild();
		CHECK(div->GetComputedValues().flex_grow() == 2.f);
		CHECK(div->GetComputedValues().flex_shrink() == 3.f);
		CHECK(div->GetComputedValues().flex_basis().type == Style::LengthPercentageAuto::Length);
		CHECK(div->GetComputedValues().flex_basis().value == 40.f);
		CHECK(div->GetProperty(PropertyId::FlexGrow)->Get<float>() == 2.f);
		CHECK(div->GetProperty(PropertyId::FlexShrink)->Get<float>() == 3.f);
		CHECK(div->GetProperty(PropertyId::FlexBasis)->ToString() == "40px");
	}

	SUBCASE("flex-grow")
	{
		document = context->LoadDocumentFromMemory(CreateString(rml_template, R"(
	body { --grow: 3; display: flex; }
	div { flex-grow: var(--grow); }
)"));
		document->Show();
		Element* div = document->GetFirstChild();

		CHECK(div->GetComputedValues().flex_grow() == 3.f);
		CHECK(div->GetProperty(PropertyId::FlexGrow)->Get<float>() == 3.f);

		div->SetProperty("--grow", "5");
		TestsShell::RenderLoop();
		CHECK(div->GetComputedValues().flex_grow() == 5.f);
		CHECK(div->GetProperty(PropertyId::FlexGrow)->Get<float>() == 5.f);
	}

	SUBCASE("transform")
	{
		document = context->LoadDocumentFromMemory(CreateString(rml_template, R"(
	body { --translate: 50px; }
	div { width: 500px; height: 400px; transform: translateX(var(--translate)); }
)"));
		document->Show();
		Element* div = document->GetFirstChild();
		CHECK(div->GetProperty(PropertyId::Transform)->ToString() == "translateX(50px)");

		TransformPtr transform = div->GetComputedValues().transform();
		REQUIRE(transform.get());
		REQUIRE(transform->GetNumPrimitives() == 1);
		CHECK(transform->GetPrimitive(0).type == TransformPrimitive::TRANSLATEX);
	}

	SUBCASE("decorator")
	{
		document = context->LoadDocumentFromMemory(CreateString(rml_template, R"(
	body { --decorator-color: #00f; }
	div { width: 500px; height: 400px; decorator: linear-gradient(to right, #f00, var(--decorator-color)); }
)"));
		document->Show();
		Element* div = document->GetFirstChild();
		const Property* decorator_property = div->GetProperty(PropertyId::Decorator);
		DecoratorsPtr decorators = decorator_property->Get<DecoratorsPtr>();
		REQUIRE(decorators.get());
		REQUIRE(decorators->list.size() == 1);
		CHECK(decorators->list[0].type == "linear-gradient");

		const Property* color_stops_property = nullptr;
		for (const auto& id_property : decorators->list.front().properties.GetProperties())
		{
			if (id_property.second.unit == Unit::COLORSTOPLIST)
				color_stops_property = &id_property.second;
		}
		REQUIRE(color_stops_property);
		ColorStopList color_stops = color_stops_property->Get<ColorStopList>();
		REQUIRE(color_stops.size() == 2);
		CHECK(color_stops[0].color == ColourbPremultiplied(255, 0, 0));
		CHECK(color_stops[1].color == ColourbPremultiplied(0, 0, 255));
	}

	SUBCASE("filter")
	{
		document = context->LoadDocumentFromMemory(CreateString(rml_template, R"(
	body { --blur-radius: 10px; }
	div { width: 500px; height: 400px; background: red; filter: blur(var(--blur-radius)); }
)"));
		document->Show();
		Element* div = document->GetFirstChild();
		const Property* filter_property = div->GetProperty(PropertyId::Filter);
		FiltersPtr filters = filter_property->Get<FiltersPtr>();
		REQUIRE(filters.get());
		REQUIRE(filters->list.size() == 1);
		CHECK(filters->list[0].type == "blur");

		const Property* sigma_property = nullptr;
		for (const auto& id_property : filters->list.front().properties.GetProperties())
		{
			if (id_property.second.unit == Unit::PX)
				sigma_property = &id_property.second;
		}
		REQUIRE(sigma_property);
		CHECK(sigma_property->Get<float>() == 10.f);
	}

	document->Close();
	TestsShell::ShutdownShell();
}
