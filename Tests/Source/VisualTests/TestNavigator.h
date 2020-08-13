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

#ifndef RMLUI_TESTS_VISUALTESTS_TESTNAVIGATOR_H
#define RMLUI_TESTS_VISUALTESTS_TESTNAVIGATOR_H

#include "TestSuite.h"
#include "CaptureScreen.h"
#include "TestViewer.h"
#include <RmlUi/Core/Types.h>
#include <RmlUi/Core/EventListener.h>

class ShellRenderInterfaceOpenGL;

class TestNavigator : public Rml::EventListener {
public:
	TestNavigator(ShellRenderInterfaceOpenGL* shell_renderer, Rml::Context* context, TestViewer* viewer, TestSuiteList test_suites);
	~TestNavigator();

	void Update();

protected:
	void ProcessEvent(Rml::Event& event) override;

private:
	enum class IterationState { None, Capture, Comparison };

	TestSuite& CurrentSuite() { return test_suites[suite_index]; }

	void LoadActiveTest();

	ComparisonResult CompareCurrentView();

	bool CaptureCurrentView();

	void StartTestSuiteIteration(IterationState iteration_state);
	void StopTestSuiteIteration();

	void UpdateGoToText(bool out_of_bounds = false);

	Rml::String GetImageFilenameFromCurrentTest();

	ShellRenderInterfaceOpenGL* shell_renderer;
	Rml::Context* context;
	TestViewer* viewer;
	TestSuiteList test_suites;

	Rml::String test_filter;

	int suite_index = 0;
	int goto_index = -1;
	SourceType source_state = SourceType::None;

	IterationState iteration_state = IterationState::None;

	int iteration_index = -1;
	int iteration_initial_index = -1;
	int iteration_wait_frames = -1;

	Rml::Vector<ComparisonResult> comparison_results;
};


#endif
