#include "../Common/TestsInterface.h"
#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Elements/ElementFormControlTextArea.h>
#include <RmlUi/Core/Types.h>
#include <doctest.h>
#include <nanobench.h>

using namespace ankerl;
using namespace Rml;

static const String document_rml = R"(
<rml>
<head>
	<link type="text/rcss" href="/../Tests/Data/style.rcss"/>
	<title>Benchmark Sample</title>
	<style>
		body {
			width: 800px;
			height: 600px;
		}
		textarea {
			width: 500px;
			height: 300px;
			border: 1px #000;
			background: #fff;
		}
	</style>
</head>

<body>
<textarea id="textarea">Hello, World!</textarea>
</body>
</rml>
)";

TEST_CASE("WidgetTextInput")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	REQUIRE(document);
	document->Show();

	auto el = rmlui_dynamic_cast<ElementFormControlTextArea*>(document->GetElementById("textarea"));
	REQUIRE(el);

	auto IncrementTime = [system_interface = TestsShell::GetTestsSystemInterface(), t = 0.0]() mutable {
		constexpr double dt = 0.5;
		t += dt;
		system_interface->SetManualTime(t);
	};
	struct TestCase {
		String name;
		String base;
	};
	const TestCase test_cases[] = {
		{"ascii", "abc"},
		{"emoji", "😍🌐😎"},
		{"japanese", "こんに"},
	};

	nanobench::Bench bench;
	bench.timeUnit(std::chrono::microseconds(1), "us");
	bench.relative(true);
	bench.complexityN(1);

	SUBCASE("FormatText")
	{
		bench.title("WidgetTextInput.FormatText");

		for (const TestCase& test_case : test_cases)
		{
			for (int num_character_repeats : {10, 20, 50, 100, 200})
			{
				String value;
				for (int i = 0; i < num_character_repeats; i++)
					value += test_case.base;

				bench.complexityN(num_character_repeats).epochs(1).epochIterations(num_character_repeats >= 100 ? 1 : 0).run(test_case.name, [&] {
					el->SetValue(value);
					context->Update();
				});
			}
		}
	}

	SUBCASE("CalculateCharacterIndex")
	{
		bench.title("WidgetTextInput.CalculateCharacterIndex");

		el->SetWordWrap(false);
		context->Update();

		for (const TestCase& test_case : test_cases)
		{
			for (int num_character_repeats : {20, 50, 100, 200, 500})
			{
				String value;
				for (int i = 0; i < num_character_repeats; i++)
					value += test_case.base;

				el->SetValue(value);
				el->Focus();
				context->ProcessKeyDown(Input::KI_END, 0);
				context->ProcessKeyUp(Input::KI_END, 0);
				context->Update();

				bench.complexityN(num_character_repeats).epochs(1).epochIterations(num_character_repeats >= 100 ? 1 : 0).run(test_case.name, [&] {
					context->ProcessMouseMove(250, 50, 0);
					context->ProcessMouseButtonDown(0, 0);
					context->ProcessMouseMove(350, 50, 0);
					context->ProcessMouseButtonUp(0, 0);
					IncrementTime();
					context->Update();
				});

				// Sanity check that the above produces a selection as intended.
				int selection_start = 0, selection_end = 0;
				el->GetSelection(&selection_start, &selection_end, nullptr);
				REQUIRE(selection_end - selection_start > 0);
			}
		}
	}

	TestsShell::RenderLoop();

	document->Close();
}
