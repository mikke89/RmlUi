#include "../../../Source/Debugger/ElementInfo.h"
#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Debugger.h>
#include <algorithm>
#include <doctest.h>

using namespace Rml;

static void SimulateClick(Context* context, Vector2i position)
{
	context->Update();
	context->ProcessMouseMove(position.x, position.y, 0);
	context->Update();
	context->ProcessMouseButtonDown(0, 0);
	context->Update();
	context->ProcessMouseButtonUp(0, 0);
	context->Update();
}

TEST_CASE("debugger")
{
	Context* context = TestsShell::GetContext(false);

	SUBCASE("no_shutdown")
	{
		Rml::Debugger::Initialise(context);

		ElementDocument* document = context->LoadDocument("assets/demo.rml");
		TestsShell::RenderLoop();

		document->Close();
		TestsShell::RenderLoop();
	}

	SUBCASE("shutdown")
	{
		Rml::Debugger::Initialise(context);

		ElementDocument* document = context->LoadDocument("assets/demo.rml");
		TestsShell::RenderLoop();

		document->Close();
		TestsShell::RenderLoop();

		Rml::Debugger::Shutdown();
		TestsShell::RenderLoop();
	}

	SUBCASE("shutdown_early")
	{
		ElementDocument* document = context->LoadDocument("assets/demo.rml");
		TestsShell::RenderLoop();

		Rml::Debugger::Initialise(context);
		TestsShell::RenderLoop();

		Rml::Debugger::Shutdown();
		TestsShell::RenderLoop();

		document->Close();
		TestsShell::RenderLoop();
	}

	TestsShell::ShutdownShell();
}

TEST_CASE("debugger.unload_documents")
{
	Context* context = TestsShell::GetContext(false);
	Rml::Debugger::Initialise(context);

	context->LoadDocument("assets/demo.rml");
	TestsShell::RenderLoop();

	// Closing documents from the debugger plugin is not allowed.
	TestsShell::SetNumExpectedWarnings(1);

	SUBCASE("UnloadDocument")
	{
		context->GetDocument("rmlui-debug-menu")->Close();
	}
	SUBCASE("UnloadAllDocuments")
	{
		context->UnloadAllDocuments();
	}

	context->Update();

	TestsShell::ShutdownShell();
}

TEST_CASE("debugger.focus")
{
	Context* context = TestsShell::GetContext(false);
	Rml::Debugger::Initialise(context);

	ElementDocument* document = context->LoadDocument("assets/demo.rml");

	SUBCASE("Normal")
	{
		document->Show();
	}
	SUBCASE("Modal")
	{
		document->Show(ModalFlag::Modal);
	}

	Rml::Debugger::SetVisible(true);

	SimulateClick(context, {200, 20});

	TestsShell::RenderLoop();

	Element* info_element = context->GetRootElement()->GetElementById("rmlui-debug-info");
	CHECK(info_element->IsVisible());

	auto info_document = rmlui_dynamic_cast<Rml::Debugger::ElementInfo*>(info_element);
	REQUIRE(info_document);

	SimulateClick(context, context->GetDimensions() / 2);

	Element* source_element = info_document->GetSourceElement();
	REQUIRE(source_element);
	CHECK(source_element->GetId() == "content");

	document->Close();
	TestsShell::ShutdownShell();
}
