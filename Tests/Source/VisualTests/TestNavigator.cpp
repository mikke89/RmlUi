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

#include "TestNavigator.h"
#include "TestSuite.h"
#include "Screenshot.h"
#include "TestViewer.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <Shell.h>

// When capturing frames it seems we need to wait at least an extra frame for the newly submitted
// render to be read back out. If we don't wait, we end up saving a screenshot of the previous test.
constexpr int capture_wait_frame_count = 2;


TestNavigator::TestNavigator(ShellRenderInterfaceOpenGL* shell_renderer, Rml::Context* context, TestViewer* viewer, TestSuiteList test_suites)
	: shell_renderer(shell_renderer), context(context), viewer(viewer), test_suites(std::move(test_suites))
{
	RMLUI_ASSERT(context);
	RMLUI_ASSERTMSG(!this->test_suites.empty(), "At least one test suite is required.");
	context->GetRootElement()->AddEventListener(Rml::EventId::Keydown, this);
	context->GetRootElement()->AddEventListener(Rml::EventId::Textinput, this);
	LoadActiveTest();
}

TestNavigator::~TestNavigator()
{
	context->GetRootElement()->RemoveEventListener(Rml::EventId::Keydown, this);
	context->GetRootElement()->RemoveEventListener(Rml::EventId::Textinput, this);
}

void TestNavigator::Update()
{
	if (capture_index >= 0 && capture_wait_frames > 0)
	{
		capture_wait_frames -= 1;
	}
	else if (capture_index >= 0)
	{
		RMLUI_ASSERT(capture_index < CurrentSuite().GetNumTests());
		capture_wait_frames += capture_wait_frame_count;

		if (!CaptureCurrentView())
		{
			StopCaptureFullTestSuite();
			return;
		}

		capture_index += 1;

		TestSuite& suite = CurrentSuite();
		if (capture_index < suite.GetNumTests())
		{
			suite.SetIndex(capture_index);
			LoadActiveTest();
		}
		else
		{
			StopCaptureFullTestSuite();
		}
	}
}

void TestNavigator::ProcessEvent(Rml::Event& event)
{
	if (event == Rml::EventId::Keydown)
	{
		auto key_identifier = (Rml::Input::KeyIdentifier)event.GetParameter< int >("key_identifier", 0);
		bool key_ctrl = event.GetParameter< bool >("ctrl_key", false);
		bool key_shift = event.GetParameter< bool >("shift_key", false);

		if (key_identifier == Rml::Input::KI_LEFT)
		{
			if (CurrentSuite().SetIndex(CurrentSuite().GetIndex() - 1))
			{
				LoadActiveTest();
			}
		}
		else if (key_identifier == Rml::Input::KI_RIGHT)
		{
			if (CurrentSuite().SetIndex(CurrentSuite().GetIndex() + 1))
			{
				LoadActiveTest();
			}
		}
		else if (key_identifier == Rml::Input::KI_UP)
		{
			int new_index = std::max(0, index - 1);
			if (new_index != index)
			{
				index = new_index;
				LoadActiveTest();
			}
		}
		else if (key_identifier == Rml::Input::KI_DOWN)
		{
			int new_index = std::min((int)test_suites.size() - 1, index + 1);
			if (new_index != index)
			{
				index = new_index;
				LoadActiveTest();
			}
		}
		else if (key_identifier == Rml::Input::KI_F7)
		{
			if (key_ctrl && key_shift)
				StartCaptureFullTestSuite();
			else
				CaptureCurrentView();
		}
		else if (key_identifier == Rml::Input::KI_S)
		{
			if (source_state == SourceType::None)
			{
				source_state = (key_shift ? SourceType::Reference : SourceType::Test);
			}
			else
			{
				if (key_shift)
					source_state = (source_state == SourceType::Reference ? SourceType::Test : SourceType::Reference);
				else
					source_state = SourceType::None;
			}
			viewer->ShowSource(source_state);
		}
		else if (key_identifier == Rml::Input::KI_ESCAPE)
		{
			if (capture_index >= 0)
			{
				StopCaptureFullTestSuite();
			}
			else if (source_state != SourceType::None)
			{
				source_state = SourceType::None;
				viewer->ShowSource(source_state);
			}
			else
			{
				Shell::RequestExit();
			}
		}
		else if (key_identifier == Rml::Input::KI_C && key_ctrl)
		{
			if (key_shift)
				Shell::SetClipboardText(CurrentSuite().GetDirectory() + '/' + viewer->GetReferenceFilename());
			else
				Shell::SetClipboardText(CurrentSuite().GetPath());
		}
		else if (key_identifier == Rml::Input::KI_HOME)
		{
			CurrentSuite().SetIndex(0);
			LoadActiveTest();
		}
		else if (key_identifier == Rml::Input::KI_END)
		{
			CurrentSuite().SetIndex(CurrentSuite().GetNumTests() - 1);
			LoadActiveTest();
		}
		else if (goto_index >= 0 && key_identifier == Rml::Input::KI_BACK)
		{
			if (goto_index <= 0)
			{
				goto_index = -1;
				viewer->SetGoToText("");
			}
			else
			{
				goto_index = goto_index / 10;
				viewer->SetGoToText(Rml::CreateString(64, "Go To: %d", goto_index));
			}
		}
	}

	if (event == Rml::EventId::Textinput)
	{
		const Rml::String text = event.GetParameter< Rml::String >("text", "");
		for (const char c : text)
		{
			if (c >= '0' && c <= '9')
			{
				if (goto_index < 0)
					goto_index = 0;

				goto_index = goto_index * 10 + int(c - '0');
				viewer->SetGoToText(Rml::CreateString(64, "Go To: %d", goto_index));
			}
			else if (goto_index >= 0 && c == '\n')
			{
				if (goto_index > 0)
				{
					if (CurrentSuite().SetIndex(goto_index))
					{
						LoadActiveTest();
					}
					else
					{
						viewer->SetGoToText(Rml::CreateString(64, "Go To out of bounds.", goto_index));
					}
				}
				goto_index = -1;
			}
		}
	}
}

void TestNavigator::LoadActiveTest()
{
	const TestSuite& suite = CurrentSuite();
	viewer->LoadTest(suite.GetDirectory(), suite.GetFilename(), suite.GetIndex(), suite.GetNumTests(), index, (int)test_suites.size());
	viewer->ShowSource(source_state);
}

bool TestNavigator::CaptureCurrentView()
{
	Rml::String filename = CurrentSuite().GetFilename();
	filename = filename.substr(0, filename.rfind('.')) + ".png";
	
	bool result = CaptureScreenshot(shell_renderer, filename, 1060);
	
	return result;
}

void TestNavigator::StopCaptureFullTestSuite()
{
	const Rml::String output_directory = GetOutputDirectory();

	TestSuite& suite = CurrentSuite();
	const int num_tests = suite.GetNumTests();

	if (capture_index == num_tests)
	{
		Rml::Log::Message(Rml::Log::LT_INFO, "Successfully captured %d document screenshots to directory: %s", capture_index, output_directory.c_str());
	}
	else
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Test suite capture aborted after %d of %d test(s). Output directory: %s", capture_index, num_tests, output_directory.c_str());
	}

	suite.SetIndex(capture_initial_index);
	LoadActiveTest();

	capture_index = -1;
	capture_initial_index = -1;
	capture_wait_frames = -1;
}

void TestNavigator::StartCaptureFullTestSuite()
{
	if(capture_index == -1)
	{
		source_state = SourceType::None;

		TestSuite& suite = CurrentSuite();
		capture_initial_index = suite.GetIndex();
		capture_wait_frames = capture_wait_frame_count;
		
		capture_index = 0;
		suite.SetIndex(capture_index);
		LoadActiveTest();
	}
}



