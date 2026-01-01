#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <doctest.h>

using namespace Rml;

static const String document_decorator_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body {
			left: 0;
			top: 0;
			right: 0;
			bottom: 0;
		}
		div {
			background: #333;
			height: 64px;
			width: 64px;
		}
		div.decorate {
			decorator: image(high_scores_alien_1.tga);
		}
	</style>
</head>

<body>
<div class="decorate"/>
<div style="decorator: image(high_scores_alien_1.tga);"/>
<img src="high_scores_alien_1.tga"/>
</body>
</rml>
)";

TEST_CASE("elementstyle.inline_decorator_images")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	// There should be no warnings loading this document. There should be three images visible.
	ElementDocument* document = context->LoadDocumentFromMemory(document_decorator_rml, "assets/");
	REQUIRE(document);
	document->Show();

	context->Update();
	context->Render();

	TestsShell::RenderLoop();

	document->Close();

	TestsShell::ShutdownShell();
}
