#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/ElementText.h>
#include <doctest.h>

using namespace Rml;

TEST_CASE("text_transform")
{
	static const String document_rml = R"(
<rml>
<head>
	<style>
		body {
			font-family: LatoLatin;
			font-size: 16px;
			width: 1000px;
		}
	</style>
</head>
<body>
	<p id="p"/>
</body>
</rml>
)";

	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	REQUIRE(document);
	document->Show();

	struct TestCase {
		String text_transform;
		String text;
		String expected;
	};

	const TestCase test_cases[] = {
		{"none", "Lärchenalp Qualità", "Lärchenalp Qualità"},
		{"uppercase", "Lärchenalp Qualità", "LÄRCHENALP QUALITÀ"},
		{"lowercase", "LÄRCHENALP QUALITÀ", "lärchenalp qualità"},
		{"uppercase", "București, Łódź, Tiếng Việt", "BUCUREȘTI, ŁÓDŹ, TIẾNG VIỆT"},
		{"uppercase", "Ελληνικά σοφός", "ΕΛΛΗΝΙΚΆ ΣΟΦΌΣ"},
		{"uppercase", "ılık µ ſ", "ILIK Μ S"},
		{"uppercase", "Привет, мир", "ПРИВЕТ, МИР"},
		{"lowercase", "ПРИВЕТ, МИР", "привет, мир"},
		{"uppercase", "ÿ ß € 😀", "Ÿ ß € 😀"},
		{"uppercase", "&lt;é&gt;", "<É>"},
	};

	Element* p = document->GetElementById("p");
	for (const TestCase& test_case : test_cases)
	{
		p->SetProperty("text-transform", test_case.text_transform);
		p->SetInnerRML(test_case.text);
		context->Update();

		ElementText* text_element = rmlui_dynamic_cast<ElementText*>(p->GetFirstChild());
		REQUIRE(text_element);

		String result;
		for (const ElementText::Line& line : text_element->GetLines())
			result += line.text;

		CAPTURE(test_case.text_transform);
		CHECK(result == test_case.expected);
	}

	document->Close();
	TestsShell::ShutdownShell();
}
