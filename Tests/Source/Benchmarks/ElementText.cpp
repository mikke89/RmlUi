#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
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
			width: 1000px;
			height: 600px;
		}
		#text {
			width: 800px;
			height: 500px;
		}
	</style>
</head>

<body>
<div id="text"/>
</body>
</rml>
)";

TEST_CASE("ElementText")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	REQUIRE(document);
	document->Show();

	Element* el = document->GetElementById("text");
	REQUIRE(el);

	struct WordBreakCase {
		const char* name;
		String base;
	};
	// The texts contain no white space, so a whole line is a single token. Whenever that token does not fit the line, it
	// has to be split up character by character in ElementText::GenerateLine, which is the loop marked @performance.
	const WordBreakCase test_cases[] = {
		{"ascii", "abcdefghij"},
		{"japanese", "こんにちは世界"},
		{"emoji", "😍🌐😎🔥🎉"},
	};

	nanobench::Bench bench;
	bench.title("ElementText");
	bench.timeUnit(std::chrono::microseconds(1), "us");
	bench.relative(true);

	// 'normal' acts as the baseline where the token is never split and simply overflows onto its own line.
	for (const char* word_break : {"normal", "break-all", "break-word"})
	{
		REQUIRE(el->SetProperty("word-break", word_break));

		for (const WordBreakCase& test_case : test_cases)
		{
			for (const int num_character_repeats : {10, 20, 50, 100, 200})
			{
				String text;
				for (int i = 0; i < num_character_repeats; i++)
					text += test_case.base;

				el->SetInnerRML(text);
				context->Update();
				context->Render();

				const String label = String(word_break) + "/" + test_case.name;
				bench.complexityN(num_character_repeats).epochs(1).epochIterations(num_character_repeats >= 100 ? 1 : 0).run(label, [&] {
					el->SetInnerRML(text);
					context->Update();
					context->Render();
				});
			}
		}
	}

	document->Close();
}
