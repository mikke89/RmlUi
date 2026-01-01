#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Types.h>
#include <doctest.h>
#include <nanobench.h>

using namespace ankerl;
using namespace Rml;

TEST_CASE("background_border")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	static String document_rml = R"(
<rml>
<head>
    <link type="text/rcss" href="/../Tests/Data/style.rcss"/>
	<style>
		div > div {
			margin: 50px auto;
			width: 300px;
			height: 200px;
			background: #c3c3c3;
			border-color: #55f #f57 #55f #afa;
		}
		#no-radius > div {
			border-width: 8px 10px;
		}
		#small-radius > div {
			border-width: 40px 20px;
			border-radius: 5px;
		}
		#large-radius > div {
			border-width: 10px 5px 25px 20px;
			border-radius: 80px 30px;
		}
	</style>
</head>

<body>
<div id="no-radius">
	<div/><div/><div/><div/><div/><div/><div/><div/><div/><div/>
</div>
<div id="small-radius">
	<div/><div/><div/><div/><div/><div/><div/><div/><div/><div/>
</div>
<div id="large-radius">
	<div/><div/><div/><div/><div/><div/><div/><div/><div/><div/>
</div>
</body>
</rml>
)";

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	REQUIRE(document);
	document->Show();

	const String msg = TestsShell::GetRenderStats();
	MESSAGE(msg);

	nanobench::Bench bench;
	bench.title("Backgrounds and borders");
	bench.relative(true);
	bench.minEpochIterations(100);
	bench.warmup(50);

	TestsShell::RenderLoop();

	bench.run("Reference (update + render)", [&] {
		context->Update();
		context->Render();
	});

	{
		ElementList elements;
		document->QuerySelectorAll(elements, "div > div");
		REQUIRE(!elements.empty());

		bench.run("Background all", [&] {
			// Force regeneration of backgrounds without changing layout
			for (auto& element : elements)
				element->SetProperty(Rml::PropertyId::BackgroundColor, Rml::Property(Colourb(), Unit::COLOUR));
			context->Update();
			context->Render();
		});

		bench.run("Border all", [&] {
			// Force regeneration of borders without changing layout
			for (auto& element : elements)
				element->SetProperty(Rml::PropertyId::BorderLeftColor, Rml::Property(Colourb(), Unit::COLOUR));
			context->Update();
			context->Render();
		});
	}

	for (const String id : {"no-radius", "small-radius", "large-radius"})
	{
		ElementList elements;
		document->QuerySelectorAll(elements, "#" + id + " > div");
		REQUIRE(!elements.empty());

		bench.run(("Border " + id).c_str(), [&] {
			for (auto& element : elements)
				element->SetProperty(Rml::PropertyId::BorderLeftColor, Rml::Property(Colourb(), Unit::COLOUR));
			context->Update();
			context->Render();
		});
	}

	document->Close();
}

TEST_CASE("box_shadow")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	static String document_rml = R"(
<rml>
<head>
    <link type="text/rcss" href="/../Tests/Data/style.rcss"/>
	<style>
		#boxshadow > div {
			width: 280dp;
			height: 70dp;
			border: 2dp #def6f7;
			margin: 10dp auto;
			padding: 15dp;
			border-radius: 30dp 8dp;
			box-sizing: border-box;
			margin-top: 100px;
			margin-bottom: 100px;
		}
		#boxshadow.blur > div {
			box-shadow:
				#f00f  40px  30px 25px 0px,
				#00ff -40px -30px 45px 0px,
				#0f08 -60px  70px 60px 0px,
				#333a  0px  0px 30px 15px inset;
		}
	</style>
</head>

<body>
<div id="boxshadow" class="blur">
	<div/><div/><div/><div/><div/><div/><div/><div/><div/><div/>
</div>
</body>
</rml>
)";

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	REQUIRE(document);
	document->Show();

	nanobench::Bench bench;
	bench.title("Box-shadow");
	bench.relative(true);
	bench.warmup(5);

	TestsShell::RenderLoop(true);

	Element* element_boxshadow = document->GetElementById("boxshadow");

	ElementList elements;
	document->QuerySelectorAll(elements, "#boxshadow > div");
	REQUIRE(!elements.empty());
	bench.run("Reference (update + render)", [&] { TestsShell::RenderLoop(false); });

	element_boxshadow->SetClass("blur", true);
	bench.run("Box-shadow (repeated)", [&] {
		// Force regeneration of backgrounds without changing layout
		for (auto& element : elements)
			element->SetProperty(Rml::PropertyId::BackgroundColor, Rml::Property(Colourb(), Unit::COLOUR));
		TestsShell::RenderLoop(false);
	});

	unsigned int unique_id = 0;
	element_boxshadow->SetClass("blur", false);
	bench.run("Box-shadow (unique)", [&] {
		for (Element* element : elements)
		{
			unique_id += 1;
			String id_string = CreateString("%x", unique_id);
			REQUIRE(id_string.size() < 12);
			id_string.resize(12, 'f');
			const auto id_index_to_color = [&](int color_index) { return id_string.substr(color_index * 3, 3); };
			const String value =
				CreateString("#%sf 40px 30px 25px 0px, #%sf -40px -30px 0px 0px, #%s8 -60px 70px 0px 0px, #%sa 0px 0px 30px 15px inset",
					id_index_to_color(0).c_str(), id_index_to_color(1).c_str(), id_index_to_color(2).c_str(), id_index_to_color(3).c_str());
			element->SetProperty("box-shadow", value);
		}
		TestsShell::RenderLoop(false);
	});

	document->Close();
}
