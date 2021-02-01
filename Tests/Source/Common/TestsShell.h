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

#ifndef RMLUI_TESTS_COMMON_TESTSSHELL_H
#define RMLUI_TESTS_COMMON_TESTSSHELL_H

#include <RmlUi/Core/Types.h>
namespace Rml { class RenderInterface; }

namespace TestsShell {

	// Will initialize the shell and create a context on first use.
	Rml::Context* GetContext();

	void PrepareRenderBuffer();
	void PresentRenderBuffer();

	// Render the current state of the context. Press 'escape' or 'return' to break out of the loop.
	// Useful for viewing documents while building the RML to benchmark.
	// Applies only when compiled with the shell backend.
	void RenderLoop();

	void ShutdownShell();

	// Set the number of expected warnings and errors logged by RmlUi until the next call to this function
	// or until 'ShutdownShell()'.
	void SetNumExpectedWarnings(int num_warnings);

	// Stats only available for the dummy renderer.
	Rml::String GetRenderStats();
}

#endif
