#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Types.h>
#include <doctest.h>
#include <nanobench.h>

using namespace ankerl;
using namespace Rml;

static const String rml_font_effect_document = R"(
<rml>
<head>
    <title>Table</title>
    <link type="text/rcss" href="/../Tests/Data/style.rcss"/>
	<style>
		body {
			font-size: 25px;
			font-effect: %s(%dpx #ff6);
		}
	</style>
</head>
<body>
The quick brown fox jumps over the lazy dog.
</body>
</rml>
)";

TEST_CASE("font_effect")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	nanobench::Bench bench;
	bench.title("Font effect");
	bench.relative(true);

	for (const char* effect_name : {"shadow", "blur", "outline", "glow"})
	{
		constexpr int effect_size = 8;

		const String rml_document = CreateString(rml_font_effect_document.c_str(), effect_name, effect_size);

		ElementDocument* document = context->LoadDocumentFromMemory(rml_document);
		document->Show();
		context->Update();
		context->Render();

		bench.run(effect_name, [&]() {
			Rml::ReleaseFontResources();
			context->Render();
		});

		document->Close();
	}

	TestsShell::ShutdownShell();
}
