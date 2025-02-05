#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Types.h>
#include <doctest.h>
#include <nanobench.h>

using namespace ankerl;
using namespace Rml;

// Start of the "CJK unified ideographs" Unicode block.
static constexpr int rml_font_texture_atlas_start_codepoint = 0x4E00;

static const String rml_font_texture_atlas_document = R"(
<rml>
<head>
    <title>Font texture atlas benchmark</title>
    <link type="text/rcss" href="/../Tests/Data/style.rcss"/>
	<style>
		body {
			font-family: "Noto Sans JP";
			font-size: %dpx;
		}
	</style>
</head>
<body id="body">
</body>
</rml>
)";

TEST_CASE("font_texture_atlas")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	LoadFontFace("assets/NotoSansJP-Regular.ttf");

	nanobench::Bench bench;
	bench.title("Font texture atlas");
	bench.relative(true);

	for (const int font_size : {12, 16, 24, 48, 96})
	{
		const String rml_document = CreateString(rml_font_texture_atlas_document.c_str(), font_size);

		ElementDocument *const document = context->LoadDocumentFromMemory(rml_document);
		REQUIRE(document);
		Element *const body = document->GetElementById("body");
		REQUIRE(body);
		document->Show();
		context->Update();
		context->Render();

		for (const int glyph_count : {10, 100, 1000})
		{
			String benchmark_name;
			FormatString(benchmark_name, "Size %d with %d glyphs", font_size, glyph_count);
			bench.run(benchmark_name.c_str(), [&]() {
				ReleaseFontResources();
				for (int i = 0; i < glyph_count; ++i)
				{
					body->SetInnerRML(StringUtilities::ToUTF8(static_cast<Character>(
						rml_font_texture_atlas_start_codepoint + i
					)));
					context->Update();
					context->Render();
				}
			});
		}

		document->Close();
	}

	TestsShell::ShutdownShell();
}
