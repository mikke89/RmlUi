#include "../Common/TestsInterface.h"
#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <Shell.h>
#include <algorithm>
#include <doctest.h>

using namespace Rml;

static const String document_textures_rml = R"(
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
			font-family: LatoLatin;
		}
		div.file {
			height: 100px;
			decorator: image(/assets/high_scores_alien_2.tga);
		}
		@spritesheet aliens {
			src: /assets/high_scores_alien_3.tga;
			alien3: 0px 0px 64px 64px;
		}
		div.sprite {
			height: 100px;
			decorator: image(alien3);
		}
		progress {
			display: block;
			width: 50px;
			height: 50px;
			background: #333;
			fill-image: /assets/high_scores_defender.tga;
		}
	</style>
</head>

<body>
<div style="display: none">
	<img src="/assets/high_scores_alien_1.tga"/>
	<div class="file"/>
	<div class="sprite"/>
	<progress direction="clockwise" start-edge="bottom" value="0.5"/>
	abc
</div>
</body>
</rml>
)";

static const String document_basic_rml = R"(
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
			font-family: LatoLatin;
			font-size: 16px;
		}
		@spritesheet aliens {
			src: /assets/high_scores_alien_3.tga;
			alien3: 0px 0px 64px 64px;
		}
		div.sprite {
			height: 100px;
			decorator: image(alien3);
		}
	</style>
</head>

<body>
	<img src="/assets/high_scores_alien_1.tga"/>
	<div class="sprite"/>
	abc
</body>
</rml>
)";

static inline StringList GetSortedTextureSourceList()
{
	StringList list = Rml::GetTextureSourceList();
	std::sort(list.begin(), list.end());
	return list;
}

TEST_CASE("core.texture_source_list")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	REQUIRE(GetSortedTextureSourceList().size() == 0);

	// We should be able to detect all sources even though they are hidden.
	const StringList list_expected = {
		"assets/high_scores_alien_1.tga",
		"assets/high_scores_alien_2.tga",
		"assets/high_scores_alien_3.tga",
		"assets/high_scores_defender.tga",
	};

	ElementDocument* document = context->LoadDocumentFromMemory(document_textures_rml);
	REQUIRE(document);
	CHECK(GetSortedTextureSourceList() == list_expected);

	document->Show();
	CHECK(GetSortedTextureSourceList() == list_expected);

	context->Update();
	CHECK(GetSortedTextureSourceList() == list_expected);

	context->Render();
	CHECK(GetSortedTextureSourceList() == list_expected);

	TestsShell::RenderLoop();

	document->Close();

	TestsShell::ShutdownShell();
}

TEST_CASE("core.load_texture_only_when_visible")
{
	TestsRenderInterface* render_interface = TestsShell::GetTestsRenderInterface();
	// This test only works with the dummy renderer.
	if (!render_interface)
		return;

	const auto& counters = render_interface->GetCounters();
	REQUIRE(counters.load_texture == 0);
	REQUIRE(counters.generate_texture == 0);
	REQUIRE(counters.release_texture == 0);

	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_textures_rml);
	Element* child_div = document->GetFirstChild();

	SUBCASE("Invisible")
	{
		CHECK(counters.load_texture == 0);
		CHECK(counters.generate_texture == 0);
		CHECK(counters.release_texture == 0);

		TestsShell::RenderLoop();
		CHECK(counters.load_texture == 0);
		CHECK(counters.generate_texture == 0);
		CHECK(counters.release_texture == 0);

		document->Show();
		TestsShell::RenderLoop();
		CHECK(counters.load_texture == 0);
		CHECK(counters.generate_texture == 0);
		CHECK(counters.release_texture == 0);
	}

	SUBCASE("Visible")
	{
		child_div->SetProperty(PropertyId::Display, Style::Display::Block);
		document->Show();
		TestsShell::RenderLoop();
		REQUIRE(counters.load_texture == 4);
		REQUIRE(counters.generate_texture > 0);
		REQUIRE(counters.release_texture == 0);
	}

	SUBCASE("Partially visible")
	{
		child_div->SetProperty(PropertyId::Display, Style::Display::Block);
		ElementList elements;
		document->GetElementsByTagName(elements, "img");
		REQUIRE(elements.size() == 1);
		elements.front()->SetProperty(PropertyId::Display, Style::Display::None);

		document->Show();
		TestsShell::RenderLoop();
		REQUIRE(counters.load_texture == 3);
		REQUIRE(counters.generate_texture > 0);
		REQUIRE(counters.release_texture == 0);
	}

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("core.warn_missing_texture_once_when_visible")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	const String document_missing_textures_rml = StringUtilities::Replace(document_textures_rml, ".tga", "_invalid.tga");
	ElementDocument* document = context->LoadDocumentFromMemory(document_missing_textures_rml);
	Element* child_div = document->GetFirstChild();

	TestsShell::SetNumExpectedWarnings(0);
	document->Show();
	TestsShell::RenderLoop();

	TestsShell::SetNumExpectedWarnings(4);
	child_div->SetProperty(PropertyId::Display, Style::Display::Block);
	TestsShell::RenderLoop();

	TestsShell::SetNumExpectedWarnings(0);
	TestsShell::RenderLoop();

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("core.release_resources")
{
	TestsRenderInterface* render_interface = TestsShell::GetTestsRenderInterface();
	// This test only works with the dummy renderer.
	if (!render_interface)
		return;

	const auto& counters = render_interface->GetCounters();

	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocument("assets/demo.rml");
	document->Show();
	TestsShell::RenderLoop();

	Element* element = document->GetElementById("content");

	SUBCASE("ReleaseTextures")
	{
		const auto startup_counters = counters;
		REQUIRE(counters.load_texture > 0);
		REQUIRE(counters.generate_texture > 0);
		REQUIRE(counters.release_texture == 0);

		// Release all textures and verify that the render interface received the release call.
		Rml::ReleaseTextures();
		CHECK(counters.load_texture == startup_counters.load_texture);
		CHECK(counters.generate_texture == startup_counters.generate_texture);
		CHECK(counters.release_texture == startup_counters.generate_texture + startup_counters.load_texture);
		const size_t num_released_textures = counters.release_texture;

		// By doing a new context Update+Render the textures should be loaded again.
		TestsShell::RenderLoop();
		CHECK(counters.load_texture == 2 * startup_counters.load_texture);
		CHECK(counters.generate_texture == 2 * startup_counters.generate_texture);
		CHECK(counters.release_texture == num_released_textures);

		// Another loop should not affect the texture calls.
		TestsShell::RenderLoop();
		CHECK(counters.load_texture == 2 * startup_counters.load_texture);
		CHECK(counters.generate_texture == 2 * startup_counters.generate_texture);
		CHECK(counters.release_texture == num_released_textures);
	}

	SUBCASE("ReleaseTexture")
	{
		const auto startup_counters = counters;
		Rml::ReleaseTexture("assets/invader.tga");
		CHECK(counters.release_texture == startup_counters.release_texture + 1);

		TestsShell::RenderLoop();
		CHECK(counters.load_texture == startup_counters.load_texture + 1);
	}

	SUBCASE("ReleaseFontResources")
	{
		const auto counter_generate_before = counters.generate_texture;
		const auto counter_release_before = counters.release_texture;

		Rml::ReleaseFontResources();
		CHECK(counters.generate_texture == counter_generate_before);
		CHECK(counters.release_texture > counter_release_before);

		// Font texture is regenerated when rendered again.
		TestsShell::RenderLoop();
		CHECK(counters.generate_texture > counter_generate_before);
	}

	SUBCASE("FontGlyphCache")
	{
		const auto counter_generate_before = counters.generate_texture;
		const auto counter_release_before = counters.release_texture;

		// Verify that ASCII characters are cached during the first use of the font. Then the font texture should not be regenerated when adding ASCII
		// characters not previously shown.
		element->SetInnerRML("Abc!%&()");
		TestsShell::RenderLoop();
		CHECK(counters.generate_texture == counter_generate_before);

		// However, when we display a non-ASCII character not part of the initial cache, the font texture needs to be regenerated.
		element->SetInnerRML(reinterpret_cast<const char*>(u8"π"));
		TestsShell::RenderLoop();
		CHECK(counters.generate_texture == counter_generate_before + 1);
		CHECK(counters.release_texture == counter_release_before + 1);
	}

	SUBCASE("ReleaseGeometry")
	{
		CHECK(counters.compile_geometry > 0);
		CHECK(counters.release_geometry == 0);

		Rml::ReleaseCompiledGeometry();
		CHECK(counters.compile_geometry == counters.release_geometry);

		TestsShell::RenderLoop();
		CHECK(counters.compile_geometry > counters.release_geometry);
	}

	document->Close();

	TestsShell::ShutdownShell(false);

	// Counters are reset during shutdown.
	const auto& counters_at_shutdown = render_interface->GetCountersFromPreviousReset();

	// Finally, verify that all generated and loaded resources were released during shutdown.
	CHECK(counters_at_shutdown.generate_texture + counters_at_shutdown.load_texture == counters_at_shutdown.release_texture);
	CHECK(counters_at_shutdown.compile_geometry == counters_at_shutdown.release_geometry);

	TestsShell::ResetTestsRenderInterface();
}

TEST_CASE("core.initialize")
{
	TestsRenderInterface* render_interface = TestsShell::GetTestsRenderInterface();
	// This test only works with the dummy renderer.
	if (!render_interface)
		return;

	const Vector2i window_size = {1280, 720};

	SUBCASE("GlobalRenderInterface")
	{
		Rml::SetRenderInterface(render_interface);
		REQUIRE(Rml::CreateContext("invalid_before_initialise", window_size) == nullptr);
		REQUIRE(Rml::Initialise());
		REQUIRE(Rml::CreateContext("main", window_size) != nullptr);
	}

	SUBCASE("ContextRenderInterface")
	{
		REQUIRE(Rml::CreateContext("invalid_before_initialise", window_size) == nullptr);
		// We should be able to initialize without setting any interfaces.
		REQUIRE(Rml::Initialise());
		// But then we must pass a render interface to new contexts (this will emit a warning).
		REQUIRE(Rml::CreateContext("invalid_no_render_interface", window_size) == nullptr);
		REQUIRE(Rml::CreateContext("main", window_size, render_interface) != nullptr);
	}

	Rml::Shutdown();
}

TEST_CASE("core.observer_ptr")
{
	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocument("assets/demo.rml");

	ObserverPtr<Element> observer_ptr = document->GetObserverPtr();
	document->Close();

	// Keeping the observer pointer alive should prevent the memory pool from shutting down.
	TestsShell::ShutdownShell();

	// For that reason, the observer pointer can still be used as normal without crashing.
	REQUIRE(!observer_ptr);

	// Resetting the observer pointer (or destroying it) should not cause any crashes, and should release the memory pool.
	observer_ptr.reset();
}

TEST_CASE("core.RemoveContext")
{
	TestsSystemInterface* system_interface = TestsShell::GetTestsSystemInterface();
	TestsRenderInterface* render_interface = TestsShell::GetTestsRenderInterface();
	// This test only works with the dummy renderer.
	if (!render_interface)
		return;

	const Vector2i window_size = {1280, 720};

	Shell::Initialize();
	Rml::SetSystemInterface(system_interface);
	REQUIRE(Rml::GetRenderInterface() == nullptr);
	REQUIRE(Rml::Initialise());
	Shell::LoadFonts();

	Context* context = Rml::CreateContext("main", window_size, render_interface);
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_basic_rml);
	document->Show();

	context->Update();
	context->Render();

	REQUIRE(Rml::RemoveContext(context->GetName()));

	SUBCASE("Normal shutdown")
	{
		Rml::Shutdown();
		TestsShell::ResetTestsRenderInterface();
	}

	SUBCASE("Destroy render interface before shutdown")
	{
		ReleaseRenderManagers();

		const auto counters = render_interface->GetCounters();
		CHECK(counters.release_texture == counters.generate_texture + counters.load_texture);
		CHECK(counters.release_geometry == counters.compile_geometry);

		TestsShell::ResetTestsRenderInterface();

		Rml::Shutdown();
	}

	Shell::Shutdown();
}
