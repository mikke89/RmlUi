#include "../Common/Mocks.h"
#include "../Common/TestsInterface.h"
#include "../Common/TestsShell.h"
#include "../Common/TypesToString.h"
#include "RmlUi/Core/DecorationTypes.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <doctest.h>

using namespace Rml;

TEST_CASE("box_shadow")
{
	static const String document_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body { inset: 0; }
		hr { display: block; border-bottom: 1px #ccc; }

		div {
			width: 80px;
			height: 22px;
			margin: 20px auto;
			background: #050;
		}
		.border { border: 2px #f00; }
		.zero_height { height: 0; }

		.shadow { box-shadow: blue 5px 6px 4px 0px; }
		.spread { box-shadow: blue 5px 6px 4px 5px; }
		.inset { box-shadow: yellow 5px 6px 4px 5px inset; }
		.spread_and_inset { box-shadow: blue 5px 6px 4px 5px, yellow 5px 6px 4px 5px inset; }

		.transparent_shadow { box-shadow: transparent 5px 6px 4px 0px; }
		.transparent_spread { box-shadow: transparent 5px 6px 4px 5px; }
		.transparent_inset { box-shadow: transparent 5px 6px 4px 5px inset; }
		.transparent_spread_and_inset { box-shadow: transparent 5px 6px 4px 5px, transparent 5px 6px 4px 5px inset; }
	</style>
</head>

<body>
	<div class="shadow" />
	<div class="spread" />
	<div class="inset" />
	<div class="spread_and_inset" />
	<hr />
	<div class="border shadow" />
	<div class="border spread" />
	<div class="border inset" />
	<div class="border spread_and_inset" />
	<hr />
	<div class="zero_height shadow" />
	<div class="zero_height spread" />
	<div class="zero_height inset" />
	<div class="zero_height spread_and_inset" />
	<hr />
	<div class="border zero_height shadow" />
	<div class="border zero_height spread" />
	<div class="border zero_height inset" />
	<div class="border zero_height spread_and_inset" />
	<hr/>
	<div class="transparent_shadow" />
	<div class="transparent_spread" />
	<div class="transparent_inset" />
	<div class="transparent_spread_and_inset" />
</body>
</rml>
)";

	Context* context = TestsShell::GetContext();

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	document->Show();

	TestsShell::RenderLoop();

	document->Close();

	TestsShell::ShutdownShell();
}
