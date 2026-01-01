#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <doctest.h>

using namespace Rml;

static const String document_media_query1_rml = R"(
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
			height: 48px;
			width: 48px;
			background: white;
		}

		@media (min-width: 640px) {
			div {
				height: 32px;
				width: 32px;
			}
		}

		@media (max-width: 639px) {
			div {
				height: 64px;
				width: 64px;
			}
		}
	</style>
</head>

<body>
<div/>
</body>
</rml>
)";

static const String document_media_query2_rml = R"(
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
			height: 48px;
			width: 48px;
			background: white;
		}

		@media (orientation: landscape) {
			div {
				height: 32px;
				width: 32px;
			}
		}

		@media (max-aspect-ratio: 3/4) {
			div {
				height: 64px;
				width: 64px;
			}
		}

		@media (min-resolution: 2x) {
			div {
				height: 128px;
			}
		}
	</style>
</head>

<body>
<div/>
</body>
</rml>
)";

static const String document_media_query3_rml = R"(
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
			height: 48px;
			width: 48px;
			background: white;
		}

		@media (orientation: landscape) and (min-width: 640px) {
			div {
				height: 32px;
				width: 32px;
			}
		}

		@media (max-aspect-ratio: 4/3) {
			div {
				height: 64px;
				width: 64px;
			}
		}
	</style>
</head>

<body>
<div/>
</body>
</rml>
)";

static const String document_media_query4_rml = R"(
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
			height: 48px;
			width: 48px;
			background: white;
		}

		@media (theme: big) {
			div {
				height: 96px;
				width: 96px;
			}
		}

		@media (theme: tiny) {
			div {
				height: 32px;
				width: 32px;
			}
		}
	</style>
</head>

<body>
<div/>
</body>
</rml>
)";

static const String document_media_query5_rml = R"(
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
			height: 48px;
			width: 48px;
			background: white;
		}

		@media not (theme: tiny) {
			div {
				height: 32px;
				width: 32px;
			}
		}

		@media (theme: big) {
			div {
				height: 96px;
				width: 96px;
			}
		}
	</style>
</head>

<body>
<div/>
</body>
</rml>
)";

static const String document_media_query6_rml = R"(
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
			height: 48px;
			width: 48px;
			background: white;
		}

		@media not not (theme: big) {
			div {
				height: 32px;
				width: 32px;
			}
		}
	</style>
</head>

<body>
<div/>
</body>
</rml>
)";

static const String document_media_query7_rml = R"(
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
			height: 48px;
			width: 48px;
			background: white;
		}

		@media (theme: big) (theme: small) {
			div {
				height: 32px;
				width: 32px;
			}
		}

		@media not (theme: big) (theme: small) {
			div {
				height: 32px;
				width: 32px;
			}
		}
	</style>
</head>

<body>
<div/>
</body>
</rml>
)";

TEST_CASE("mediaquery.basic")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	// There should be no warnings loading this document. There should be one div of 32px width & height
	ElementDocument* document = context->LoadDocumentFromMemory(document_media_query1_rml);
	REQUIRE(document);
	document->Show();

	context->Update();
	context->Render();

	TestsShell::RenderLoop();

	// Shell default window dimensions are 1500, 800

	ElementList elems;
	document->GetElementsByTagName(elems, "div");
	CHECK(elems.size() == 1);

	CHECK(elems[0]->GetBox() == Box(Vector2f(32.0f, 32.0f)));

	document->Close();

	TestsShell::ShutdownShell();
}

TEST_CASE("mediaquery.dynamic")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	// There should be no warnings loading this document. There should be one div of 32px width & height
	ElementDocument* document = context->LoadDocumentFromMemory(document_media_query1_rml);
	REQUIRE(document);
	document->Show();

	context->Update();
	context->Render();

	TestsShell::RenderLoop();

	// Shell default window dimensions are 1500, 800

	ElementList elems;
	document->GetElementsByTagName(elems, "div");
	CHECK(elems.size() == 1);

	CHECK(elems[0]->GetBox() == Box(Vector2f(32.0f, 32.0f)));

	context->SetDimensions(Vector2i(480, 320));

	context->Update();

	CHECK(elems[0]->GetBox() == Box(Vector2f(64.0f, 64.0f)));

	document->Close();

	TestsShell::ShutdownShell();
}

TEST_CASE("mediaquery.custom_properties")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	context->SetDensityIndependentPixelRatio(2.0f);

	// There should be no warnings loading this document. There should be one div of 32px width & height
	ElementDocument* document = context->LoadDocumentFromMemory(document_media_query2_rml);
	REQUIRE(document);
	document->Show();

	context->Update();
	context->Render();

	TestsShell::RenderLoop();

	// Shell default window dimensions are 1500, 800

	ElementList elems;
	document->GetElementsByTagName(elems, "div");
	CHECK(elems.size() == 1);

	CHECK(elems[0]->GetBox() == Box(Vector2f(32.0f, 128.0f)));

	document->Close();

	TestsShell::ShutdownShell();
}

TEST_CASE("mediaquery.composite")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	// There should be no warnings loading this document. There should be one div of 32px width & height
	ElementDocument* document = context->LoadDocumentFromMemory(document_media_query3_rml);
	REQUIRE(document);
	document->Show();

	context->Update();
	context->Render();

	TestsShell::RenderLoop();

	// Shell default window dimensions are 1500, 800

	ElementList elems;
	document->GetElementsByTagName(elems, "div");
	CHECK(elems.size() == 1);

	CHECK(elems[0]->GetBox() == Box(Vector2f(32.0f, 32.0f)));

	document->Close();

	TestsShell::ShutdownShell();
}

TEST_CASE("mediaquery.theme")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_media_query4_rml);
	REQUIRE(document);
	document->Show();

	context->Update();
	context->Render();

	TestsShell::RenderLoop();

	ElementList elems;
	document->GetElementsByTagName(elems, "div");
	CHECK(elems.size() == 1);

	CHECK(elems[0]->GetBox().GetSize().x == 48.0f);

	context->ActivateTheme("big", true);
	context->Update();
	context->Render();

	CHECK(elems[0]->GetBox().GetSize().x == 96.0f);

	context->ActivateTheme("big", false);
	context->Update();
	context->Render();

	CHECK(elems[0]->GetBox().GetSize().x == 48.0f);

	context->ActivateTheme("tiny", true);
	context->Update();
	context->Render();

	CHECK(elems[0]->GetBox().GetSize().x == 32.0f);

	context->ActivateTheme("big", true);
	context->Update();
	context->Render();

	CHECK(elems[0]->GetBox().GetSize().x == 32.0f);

	document->Close();

	TestsShell::ShutdownShell();
}

// test of `not`; `not` should be an inverted case
TEST_CASE("mediaquery.theme.notonly")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_media_query5_rml);
	REQUIRE(document);
	document->Show();

	context->Update();
	context->Render();

	TestsShell::RenderLoop();

	ElementList elems;
	document->GetElementsByTagName(elems, "div");
	CHECK(elems.size() == 1);

	CHECK(elems[0]->GetBox().GetSize().x == 32.0f);

	context->ActivateTheme("big", true);
	context->Update();
	context->Render();

	CHECK(elems[0]->GetBox().GetSize().x == 96.0f);

	context->ActivateTheme("big", false);
	context->Update();
	context->Render();

	CHECK(elems[0]->GetBox().GetSize().x == 32.0f);

	context->ActivateTheme("tiny", true);
	context->Update();
	context->Render();

	CHECK(elems[0]->GetBox().GetSize().x == 48.0f);

	context->ActivateTheme("big", true);
	context->Update();
	context->Render();

	CHECK(elems[0]->GetBox().GetSize().x == 96.0f);

	document->Close();

	TestsShell::ShutdownShell();
}

// test that `not` cannot appear multiple times
TEST_CASE("mediaquery.theme.notonly_one")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	
	INFO("Expected warnings: unexpected 'not'.");
	TestsShell::SetNumExpectedWarnings(1);

	ElementDocument* document = context->LoadDocumentFromMemory(document_media_query6_rml);
	REQUIRE(document);
	document->Close();

	TestsShell::ShutdownShell();
}


// test that an `and` must be between multiple conditions.
TEST_CASE("mediaquery.theme.condition_checks")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);
	
	INFO("Expected warnings: expected 'and'.");
	TestsShell::SetNumExpectedWarnings(2);

	ElementDocument* document = context->LoadDocumentFromMemory(document_media_query7_rml);
	REQUIRE(document);
	document->Close();

	TestsShell::ShutdownShell();
}
