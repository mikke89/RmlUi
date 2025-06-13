/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

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
