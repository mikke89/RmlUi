#include "../Common/TestsInterface.h"
#include <RmlUi/Core/ComputedValues.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/DecorationTypes.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/FontEngineInterface.h>
#include <RmlUi/Core/PropertyDictionary.h>
#include <RmlUi/Core/StyleSheetSpecification.h>
#include <RmlUi/Core/StyleSheetTypes.h>
#include <doctest.h>

using namespace Rml;

TEST_CASE("Properties")
{
	const Vector2i window_size(1024, 768);

	TestsSystemInterface system_interface;
	TestsRenderInterface render_interface;

	SetRenderInterface(&render_interface);
	SetSystemInterface(&system_interface);

	Rml::Initialise();

	Context* context = Rml::CreateContext("main", window_size);
	ElementDocument* document = context->CreateDocument();

	SUBCASE("inset")
	{
		struct InsetTestCase {
			String inset_value;

			struct ExpectedValues {
				String top;
				String right;
				String bottom;
				String left;
			} expected;
		};

		InsetTestCase tests[] = {
			{"auto", {"auto", "auto", "auto", "auto"}},
			{"0", {"0px", "0px", "0px", "0px"}},
			{"1px", {"1px", "1px", "1px", "1px"}},
			{"1dp", {"1dp", "1dp", "1dp", "1dp"}},
			{"1%", {"1%", "1%", "1%", "1%"}},
			{"10px 20px", {"10px", "20px", "10px", "20px"}},
			{"10px 20px 30px", {"10px", "20px", "30px", "20px"}},
			{"10px 20px 30px 40px", {"10px", "20px", "30px", "40px"}},
		};

		for (const InsetTestCase& test : tests)
		{
			CHECK(document->SetProperty("inset", test.inset_value));

			CHECK(document->GetProperty("top")->ToString() == test.expected.top);
			CHECK(document->GetProperty("right")->ToString() == test.expected.right);
			CHECK(document->GetProperty("bottom")->ToString() == test.expected.bottom);
			CHECK(document->GetProperty("left")->ToString() == test.expected.left);
		}
	}

	SUBCASE("flex")
	{
		struct FlexTestCase {
			String flex_value;

			struct ExpectedValues {
				float flex_grow;
				float flex_shrink;
				String flex_basis;
			} expected;
		};

		FlexTestCase tests[] = {
			{"", {0.f, 1.f, "auto"}},
			{"none", {0.f, 0.f, "auto"}},
			{"auto", {1.f, 1.f, "auto"}},
			{"1", {1.f, 1.f, "0px"}},
			{"2", {2.f, 1.f, "0px"}},
			{"2 0", {2.f, 0.f, "0px"}},
			{"2 3", {2.f, 3.f, "0px"}},
			{"2 auto", {2.f, 1.f, "auto"}},
			{"2 0 auto", {2.f, 0.f, "auto"}},
			{"0 0 auto", {0.f, 0.f, "auto"}},
			{"0 0 50px", {0.f, 0.f, "50px"}},
			{"0 0 50px", {0.f, 0.f, "50px"}},
			{"0 0 0", {0.f, 0.f, "0px"}},
		};

		for (const FlexTestCase& test : tests)
		{
			if (!test.flex_value.empty())
			{
				CHECK(document->SetProperty("flex", test.flex_value));
			}

			CHECK(document->GetProperty<float>("flex-grow") == test.expected.flex_grow);
			CHECK(document->GetProperty<float>("flex-shrink") == test.expected.flex_shrink);
			CHECK(document->GetProperty("flex-basis")->ToString() == test.expected.flex_basis);
		}
	}

	SUBCASE("gradient")
	{
		auto ParseGradient = [&](const String& value) -> Property {
			document->SetProperty("decorator", "linear-gradient(" + value + ")");
			auto decorators = document->GetProperty<DecoratorsPtr>("decorator");
			if (!decorators || decorators->list.size() != 1)
				return {};
			for (auto& id_property : decorators->list.front().properties.GetProperties())
			{
				if (id_property.second.unit == Unit::COLORSTOPLIST)
					return id_property.second;
			}
			return {};
		};

		struct GradientTestCase {
			String value;
			String expected_parsed_string;
			ColorStopList expected_color_stops;
		};

		GradientTestCase test_cases[] = {
			{
				"red, blue",
				"#ff0000, #0000ff",
				{
					ColorStop{ColourbPremultiplied(255, 0, 0), NumericValue{}},
					ColorStop{ColourbPremultiplied(0, 0, 255), NumericValue{}},
				},
			},
			{
				"red 5px, blue 50%",
				"#ff0000 5px, #0000ff 50%",
				{
					ColorStop{ColourbPremultiplied(255, 0, 0), NumericValue{5.f, Unit::PX}},
					ColorStop{ColourbPremultiplied(0, 0, 255), NumericValue{50.f, Unit::PERCENT}},
				},
			},
			{
				"red, #00f 50%, rgba(0, 255,0, 150) 10dp",
				"#ff0000, #0000ff 50%, #00ff0096 10dp",
				{
					ColorStop{ColourbPremultiplied(255, 0, 0), NumericValue{}},
					ColorStop{ColourbPremultiplied(0, 0, 255), NumericValue{50.f, Unit::PERCENT}},
					ColorStop{ColourbPremultiplied(0, 150, 0, 150), NumericValue{10.f, Unit::DP}},
				},
			},
			{
				"hsl(10000, 0%, 50%), hsl(240, 100%, 50%) 50%, hsla(-240, 100%, 50%, 0.5) 10dp",
				"#7f7f7f, #0000ff 50%, #00ff007f 10dp",
				{
					ColorStop{ColourbPremultiplied(127, 127, 127), NumericValue{}},
					ColorStop{ColourbPremultiplied(0, 0, 255), NumericValue{50.f, Unit::PERCENT}},
					ColorStop{ColourbPremultiplied(0, 127, 0, 127), NumericValue{10.f, Unit::DP}},
				},
			},
			{
				"lab(55% none none), lab(30% 67% -110) 50%, lab(90% -90 80 / 0.5) 10dp",
				"#838383, #0000fb 50%, #00ff267f 10dp",
				{
					ColorStop{ColourbPremultiplied(131, 131, 131), NumericValue{}},
					ColorStop{ColourbPremultiplied(0, 0, 251), NumericValue{50.f, Unit::PERCENT}},
					ColorStop{ColourbPremultiplied(0, 127, 19, 127), NumericValue{10.f, Unit::DP}},
				},
			},
			{
				"lch(55% none 300.0), lch(30% 85% 180.0) 50%, lch(90% 90 100.0 / 50%) 10dp",
				"#838383, #006044 50%, #f0e6007f 10dp",
				{
					ColorStop{ColourbPremultiplied(131, 131, 131), NumericValue{}},
					ColorStop{ColourbPremultiplied(0, 96, 68), NumericValue{50.f, Unit::PERCENT}},
					ColorStop{ColourbPremultiplied(120, 115, 0, 127), NumericValue{10.f, Unit::DP}},
				},
			},
			{
				"oklab(85% -50% 70%), oklab(0.2 0.4 -0.4) 50%, oklab(100% none 0.4 / 0.25) 10dp",
				"#98ed00, #6600c1 50%, #ffde003f 10dp",
				{
					ColorStop{ColourbPremultiplied(152, 237, 0), NumericValue{}},
					ColorStop{ColourbPremultiplied(102, 0, 193), NumericValue{50.f, Unit::PERCENT}},
					ColorStop{ColourbPremultiplied(63, 55, 0, 63), NumericValue{10.f, Unit::DP}},
				},
			},
			{
				"oklch(75% 100% 30.0), oklch(0.5 0.2 270) 50%, oklch(1.0 0.1 none / 0.5) 10dp",
				"#ff0000, #3a50d2 50%, #ffe2fa7f 10dp",
				{
					ColorStop{ColourbPremultiplied(255, 0, 0), NumericValue{}},
					ColorStop{ColourbPremultiplied(58, 80, 210), NumericValue{50.f, Unit::PERCENT}},
					ColorStop{ColourbPremultiplied(127, 113, 125, 127), NumericValue{10.f, Unit::DP}},
				},
			},
			{
				"red 50px 20%, blue 10in",
				"#ff0000 50px, #ff0000 20%, #0000ff 10in",
				{
					ColorStop{ColourbPremultiplied(255, 0, 0), NumericValue{50.f, Unit::PX}},
					ColorStop{ColourbPremultiplied(255, 0, 0), NumericValue{20.f, Unit::PERCENT}},
					ColorStop{ColourbPremultiplied(0, 0, 255), NumericValue{10.f, Unit::INCH}},
				},
			},
		};

		for (const GradientTestCase& test_case : test_cases)
		{
			const Property result = ParseGradient(test_case.value);
			CHECK(result.ToString() == test_case.expected_parsed_string);
			CHECK(result.Get<ColorStopList>() == test_case.expected_color_stops);
		}
	}

	Rml::Shutdown();
}

TEST_CASE("Context custom properties")
{
	TestsSystemInterface system_interface;
	TestsRenderInterface render_interface;
	FontEngineInterface font_interface;
	SetRenderInterface(&render_interface);
	SetSystemInterface(&system_interface);
	SetFontEngineInterface(&font_interface);

	Rml::Initialise();

	Context* context = Rml::CreateContext("main", Vector2i(1024, 768));
	REQUIRE(context != nullptr);

	SUBCASE("storage round-trip")
	{
		CHECK(context->FindVariable("--primary") == nullptr);

		context->SetVariable("--primary", "red");
		REQUIRE(context->FindVariable("--primary") != nullptr);
		CHECK(*context->FindVariable("--primary") == "red");

		context->SetVariable("--primary", "blue");
		CHECK(*context->FindVariable("--primary") == "blue");

		context->SetVariable("--secondary", "16px");
		CHECK(*context->FindVariable("--primary") == "blue");
		CHECK(*context->FindVariable("--secondary") == "16px");

		CHECK(context->FindVariable("--undefined") == nullptr);
	}

	Rml::Shutdown();
}

TEST_CASE("Property var() detection")
{
	TestsSystemInterface system_interface;
	TestsRenderInterface render_interface;
	FontEngineInterface font_interface;
	SetRenderInterface(&render_interface);
	SetSystemInterface(&system_interface);
	SetFontEngineInterface(&font_interface);

	Rml::Initialise();

	Context* context = Rml::CreateContext("main", Vector2i(1024, 768));
	REQUIRE(context != nullptr);
	ElementDocument* document = context->CreateDocument();
	REQUIRE(document != nullptr);

	SUBCASE("var() reference defers typed parsing")
	{
		REQUIRE(document->SetProperty("color", "var(--primary)"));
		const Property* p = document->GetProperty(PropertyId::Color);
		REQUIRE(p != nullptr);
		CHECK(p->contains_variable == true);
		CHECK(p->unit == Unit::STRING);
		CHECK(p->Get<String>() == "var(--primary)");
	}

	SUBCASE("plain value parses normally")
	{
		REQUIRE(document->SetProperty("color", "red"));
		const Property* p = document->GetProperty(PropertyId::Color);
		REQUIRE(p != nullptr);
		CHECK(p->contains_variable == false);
		CHECK(p->unit == Unit::COLOUR);
	}

	SUBCASE("var() inside a longer value also defers")
	{
		REQUIRE(document->SetProperty("background-color", "var(--bg, #fff)"));
		const Property* p = document->GetProperty(PropertyId::BackgroundColor);
		REQUIRE(p != nullptr);
		CHECK(p->contains_variable == true);
		CHECK(p->Get<String>() == "var(--bg, #fff)");
	}

	Rml::Shutdown();
}

TEST_CASE("Variable resolution at compute time")
{
	TestsSystemInterface system_interface;
	TestsRenderInterface render_interface;
	FontEngineInterface font_interface;
	SetRenderInterface(&render_interface);
	SetSystemInterface(&system_interface);
	SetFontEngineInterface(&font_interface);

	Rml::Initialise();

	Context* context = Rml::CreateContext("main", Vector2i(1024, 768));
	REQUIRE(context != nullptr);
	ElementDocument* document = context->CreateDocument();
	REQUIRE(document != nullptr);

	SUBCASE("simple var() substitutes into computed background-color")
	{
		context->SetVariable("--primary", "red");
		REQUIRE(document->SetProperty("background-color", "var(--primary)"));
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(255, 0, 0, 255));
	}

	SUBCASE("fallback wins when variable undefined")
	{
		REQUIRE(document->SetProperty("background-color", "var(--missing, blue)"));
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(0, 0, 255, 255));
	}

	SUBCASE("named variable wins over fallback when both present")
	{
		context->SetVariable("--accent", "lime");
		REQUIRE(document->SetProperty("background-color", "var(--accent, magenta)"));
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(0, 255, 0, 255));
	}

	SUBCASE("variable value containing var() is recursively resolved")
	{
		context->SetVariable("--accent", "red");
		context->SetVariable("--bg", "var(--accent)");
		REQUIRE(document->SetProperty("background-color", "var(--bg)"));
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(255, 0, 0, 255));
	}

	SUBCASE("cycle between two variables resolves to fallback without hanging")
	{
		context->SetVariable("--a", "var(--b)");
		context->SetVariable("--b", "var(--a)");
		REQUIRE(document->SetProperty("background-color", "var(--a, blue)"));
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(0, 0, 255, 255));
	}

	SUBCASE("nested fallback resolves when outer variable missing")
	{
		context->SetVariable("--inner", "yellow");
		REQUIRE(document->SetProperty("background-color", "var(--missing, var(--inner, magenta))"));
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(255, 255, 0, 255));
	}

	SUBCASE("document-scoped variable resolves")
	{
		document->SetVariable("--doc-color", "red");
		REQUIRE(document->SetProperty("background-color", "var(--doc-color)"));
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(255, 0, 0, 255));
	}

	SUBCASE("context variable shadows document variable of the same name")
	{
		document->SetVariable("--shared", "blue");
		context->SetVariable("--shared", "red");
		REQUIRE(document->SetProperty("background-color", "var(--shared)"));
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(255, 0, 0, 255));
	}

	SUBCASE("context variable visible when document has no shadowing entry")
	{
		context->SetVariable("--ctx-only", "red");
		REQUIRE(document->SetProperty("background-color", "var(--ctx-only)"));
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(255, 0, 0, 255));
	}

	Rml::Shutdown();
}

TEST_CASE("RCSS custom property declarations")
{
	TestsSystemInterface system_interface;
	TestsRenderInterface render_interface;
	FontEngineInterface font_interface;
	SetRenderInterface(&render_interface);
	SetSystemInterface(&system_interface);
	SetFontEngineInterface(&font_interface);

	Rml::Initialise();

	Context* context = Rml::CreateContext("main", Vector2i(1024, 768));
	REQUIRE(context != nullptr);

	const String rml_source =
		"<rml>"
		"<head>"
		"<style>"
		":root { --primary: red; --spacing: 16px; }"
		"</style>"
		"</head>"
		"<body/>"
		"</rml>";

	ElementDocument* document = context->LoadDocumentFromMemory(rml_source);
	REQUIRE(document != nullptr);

	SUBCASE("declarations populate the document variable map")
	{
		REQUIRE(document->FindVariable("--primary") != nullptr);
		CHECK(*document->FindVariable("--primary") == "red");
		REQUIRE(document->FindVariable("--spacing") != nullptr);
		CHECK(*document->FindVariable("--spacing") == "16px");
	}

	SUBCASE("RCSS-declared variables resolve through var() at compute time")
	{
		REQUIRE(document->SetProperty("background-color", "var(--primary)"));
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(255, 0, 0, 255));
	}

	SUBCASE("context variable overrides RCSS-declared default")
	{
		context->SetVariable("--primary", "blue");
		REQUIRE(document->SetProperty("background-color", "var(--primary)"));
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(0, 0, 255, 255));
	}

	Rml::Shutdown();
}

TEST_CASE("RCSS custom properties: @media theme integration")
{
	TestsSystemInterface system_interface;
	TestsRenderInterface render_interface;
	FontEngineInterface font_interface;
	SetRenderInterface(&render_interface);
	SetSystemInterface(&system_interface);
	SetFontEngineInterface(&font_interface);

	Rml::Initialise();

	Context* context = Rml::CreateContext("main", Vector2i(1024, 768));
	REQUIRE(context != nullptr);

	const String rml_source =
		"<rml>"
		"<head>"
		"<style>"
		"@media (theme: dark) { :root { --primary: red; } }"
		"</style>"
		"</head>"
		"<body/>"
		"</rml>";

	ElementDocument* document = context->LoadDocumentFromMemory(rml_source);
	REQUIRE(document != nullptr);
	REQUIRE(document->SetProperty("background-color", "var(--primary, blue)"));

	SUBCASE("variable from @media block is absent when theme is inactive")
	{
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(0, 0, 255, 255));
	}

	SUBCASE("variable from @media block appears on activate, clears on deactivate")
	{
		context->ActivateTheme("dark", true);
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(255, 0, 0, 255));

		context->ActivateTheme("dark", false);
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(0, 0, 255, 255));
	}

	SUBCASE("programmatic SetVariable persists across theme toggles")
	{
		document->SetVariable("--primary", "lime");
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(0, 255, 0, 255));

		context->ActivateTheme("dark", true);
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(0, 255, 0, 255));

		context->ActivateTheme("dark", false);
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(0, 255, 0, 255));
	}

	SUBCASE("document RemoveVariable falls through to RCSS @media value")
	{
		document->SetVariable("--primary", "lime");
		context->ActivateTheme("dark", true);
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(0, 255, 0, 255));

		document->RemoveVariable("--primary");
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(255, 0, 0, 255));
	}

	Rml::Shutdown();
}

TEST_CASE("RCSS custom properties: base + @media combine correctly")
{
	// Regression: variables declared in an always-on :root block must survive @media block
	// activation. CombineStyleSheet must merge custom_properties along with keyframes / decorators.
	TestsSystemInterface system_interface;
	TestsRenderInterface render_interface;
	FontEngineInterface font_interface;
	SetRenderInterface(&render_interface);
	SetSystemInterface(&system_interface);
	SetFontEngineInterface(&font_interface);

	Rml::Initialise();

	Context* context = Rml::CreateContext("main", Vector2i(1024, 768));
	REQUIRE(context != nullptr);

	const String rml_source =
		"<rml>"
		"<head>"
		"<style>"
		":root { --bg: blue; --width: 16px; }"
		"@media (theme: dark) { :root { --bg: red; } }"
		"</style>"
		"</head>"
		"<body/>"
		"</rml>";

	ElementDocument* document = context->LoadDocumentFromMemory(rml_source);
	REQUIRE(document != nullptr);
	REQUIRE(document->SetProperty("background-color", "var(--bg)"));
	REQUIRE(document->SetProperty("border-top-width", "var(--width)"));

	SUBCASE("base structural var survives @media activation")
	{
		context->ActivateTheme("dark", true);
		context->Update();
		// --bg overridden by @media (red), --width inherited from base (16px), must NOT vanish.
		CHECK(document->GetComputedValues().background_color() == Colourb(255, 0, 0, 255));
		CHECK(document->GetComputedValues().border_top_width() == 16);
	}

	SUBCASE("base structural var still present after @media deactivation")
	{
		context->ActivateTheme("dark", true);
		context->Update();
		context->ActivateTheme("dark", false);
		context->Update();
		// --bg falls back to base (blue), --width still from base (16px).
		CHECK(document->GetComputedValues().background_color() == Colourb(0, 0, 255, 255));
		CHECK(document->GetComputedValues().border_top_width() == 16);
	}

	Rml::Shutdown();
}

TEST_CASE("Variable resolution edge cases")
{
	TestsSystemInterface system_interface;
	TestsRenderInterface render_interface;
	FontEngineInterface font_interface;
	SetRenderInterface(&render_interface);
	SetSystemInterface(&system_interface);
	SetFontEngineInterface(&font_interface);

	Rml::Initialise();

	Context* context = Rml::CreateContext("main", Vector2i(1024, 768));
	REQUIRE(context != nullptr);
	ElementDocument* document = context->CreateDocument();
	REQUIRE(document != nullptr);

	SUBCASE("multiple var() refs inside one function expression")
	{
		context->SetVariable("--r", "255");
		context->SetVariable("--g", "128");
		context->SetVariable("--b", "0");
		REQUIRE(document->SetProperty("background-color", "rgb(var(--r), var(--g), var(--b))"));
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(255, 128, 0, 255));
	}

	SUBCASE("var() inside a shorthand component is resolved per-component")
	{
		context->SetVariable("--col", "red");
		REQUIRE(document->SetProperty("border-color", "var(--col)"));
		context->Update();
		CHECK(document->GetComputedValues().border_top_color() == Colourb(255, 0, 0, 255));
	}

	SUBCASE("Context::SetVariable triggers re-resolution on next Update")
	{
		// Property is set first; variable is undefined so the fallback resolves.
		REQUIRE(document->SetProperty("background-color", "var(--late, blue)"));
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(0, 0, 255, 255));

		// Defining the variable later dirties consuming properties via DirtyVariableConsumers, so
		// the next Update re-resolves them without the caller having to re-set the property.
		context->SetVariable("--late", "lime");
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(0, 255, 0, 255));

		// Changing the variable to a new value also triggers re-resolution.
		context->SetVariable("--late", "red");
		context->Update();
		CHECK(document->GetComputedValues().background_color() == Colourb(255, 0, 0, 255));
	}

	Rml::Shutdown();
}

TEST_CASE("Property.ToString")
{
	TestsSystemInterface system_interface;
	TestsRenderInterface render_interface;
	SetRenderInterface(&render_interface);
	SetSystemInterface(&system_interface);

	Rml::Initialise();

	CHECK(Property(5.2f, Unit::CM).ToString() == "5.2cm");
	CHECK(Property(150, Unit::PERCENT).ToString() == "150%");
	CHECK(Property(Colourb{170, 187, 204, 255}, Unit::COLOUR).ToString() == "#aabbcc");

	auto ParsedValue = [](const String& name, const String& value) -> String {
		PropertyDictionary properties;
		StyleSheetSpecification::ParsePropertyDeclaration(properties, name, value);
		REQUIRE(properties.GetNumProperties() == 1);
		return properties.GetProperties().begin()->second.ToString();
	};

	CHECK(ParsedValue("width", "10px") == "10px");
	CHECK(ParsedValue("width", "10.00em") == "10em");
	CHECK(ParsedValue("width", "auto") == "auto");

	CHECK(ParsedValue("background-color", "#abc") == "#aabbcc");
	CHECK(ParsedValue("background-color", "red") == "#ff0000");

	CHECK(ParsedValue("transform", "translateX(10px)") == "translateX(10px)");
	CHECK(ParsedValue("transform", "translate(20in, 50em)") == "translate(20in, 50em)");

	CHECK(ParsedValue("box-shadow", "2px 2px 0px, rgba(0, 0, 255, 255) 4px 4px 2em") == "#000000 2px 2px 0px, #0000ff 4px 4px 2em");
	CHECK(ParsedValue("box-shadow", "2px 2px 0px, #00ff 4px 4px 2em") == "#000000 2px 2px 0px, #0000ff 4px 4px 2em");

	// Due to conversion to and from premultiplied alpha, some color information is lost.
	CHECK(ParsedValue("box-shadow", "#fff0 2px 2px 0px") == "#00000000 2px 2px 0px");

	CHECK(ParsedValue("decorator", "linear-gradient(110deg, #fff3, #fff 10%, #c33 250dp, #3c3, #33c, #000 90%, #0003) border-box") ==
		"linear-gradient(110deg, #fff3, #fff 10%, #c33 250dp, #3c3, #33c, #000 90%, #0003) border-box");

	CHECK(ParsedValue("filter", "drop-shadow(#000 30px 20px 5px) opacity(0.2) sepia(0.2)") ==
		"drop-shadow(#000 30px 20px 5px) opacity(0.2) sepia(0.2)");

	Rml::Shutdown();
}
