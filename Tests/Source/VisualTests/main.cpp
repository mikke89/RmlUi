/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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

#include "Window.h"
#include "../Common/TestsShell.h"
#include <RmlUi/Core/StringUtilities.h>

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest.h>

#include <Shell.h>


using namespace Rml;

Rml::Context* context = nullptr;

bool run_loop = true;
bool single_loop = true;

void GameLoop()
{
	if (run_loop || single_loop)
	{
		single_loop = false;

		context->Update();

		TestsShell::PrepareRenderBuffer();
		context->Render();
		TestsShell::PresentRenderBuffer();
	}
}


TEST_CASE("Run VisualTests")
{
	// Start the visual test suite
	context = TestsShell::GetMainContext();
	REQUIRE(context);

	const String samples_root = Shell::FindSamplesRoot();

	StringList directories = { samples_root + "../Tests/Data/VisualTests" };

#ifdef RMLUI_VISUAL_TESTS_DIRECTORIES
	StringUtilities::ExpandString(directories, RMLUI_VISUAL_TESTS_DIRECTORIES, ';');
#endif

	TestSuiteList test_suites;

	for (const String& directory : directories)
	{
		const StringList files = Shell::ListFiles(directory, "rml");

		if (files.empty())
		{
			MESSAGE("Could not find any *.rml* files in directory '" << directory << "'. Ignoring.'");
		}

		test_suites.push_back(
			TestSuite{ directory, std::move(files) }
		);
	}

	REQUIRE_MESSAGE(!test_suites.empty(), "RML test files directory not found or empty.");

	Window window(context, std::move(test_suites));

	TestsShell::EventLoop(GameLoop);
}



int main(int argc, char** argv) {

    // Initialize and run doctest
    doctest::Context doctest_context;

    doctest_context.applyCommandLine(argc, argv);

    int doctest_result = doctest_context.run();

    if (doctest_context.shouldExit())
        return doctest_result;
	
    // RmlUi is initialized during doctest run above as necessary.
    // Clean everything up here.
    TestsShell::ShutdownShell();

    return doctest_result;
}
