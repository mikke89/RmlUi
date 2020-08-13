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
#include "TestConfig.h"
#include "CaptureScreen.h"
#include "TestViewer.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <Shell.h>
#include <cstdio>

// When capturing frames it seems we need to wait at least an extra frame for the newly submitted
// render to be read back out. If we don't wait, we end up saving a screenshot of the previous test.
constexpr int iteration_wait_frame_count = 2;


TestNavigator::TestNavigator(ShellRenderInterfaceOpenGL* shell_renderer, Rml::Context* context, TestViewer* viewer, TestSuiteList test_suites)
	: shell_renderer(shell_renderer), context(context), viewer(viewer), test_suites(std::move(test_suites))
{
	RMLUI_ASSERT(context);
	RMLUI_ASSERTMSG(!this->test_suites.empty(), "At least one test suite is required.");
	context->GetRootElement()->AddEventListener(Rml::EventId::Keydown, this, true);
	context->GetRootElement()->AddEventListener(Rml::EventId::Keydown, this);
	context->GetRootElement()->AddEventListener(Rml::EventId::Textinput, this);
	context->GetRootElement()->AddEventListener(Rml::EventId::Change, this);
	LoadActiveTest();
}

TestNavigator::~TestNavigator()
{
	context->GetRootElement()->RemoveEventListener(Rml::EventId::Keydown, this, true);
	context->GetRootElement()->RemoveEventListener(Rml::EventId::Keydown, this);
	context->GetRootElement()->RemoveEventListener(Rml::EventId::Textinput, this);
	context->GetRootElement()->RemoveEventListener(Rml::EventId::Change, this);
}

void TestNavigator::Update()
{
	if(iteration_state != IterationState::None)
	{
		RMLUI_ASSERT(iteration_index >= 0);

		// Capture test document screenshots iteratively every nth frame.
		if (iteration_wait_frames > 0)
		{
			iteration_wait_frames -= 1;
		}
		else
		{
			RMLUI_ASSERT(iteration_index < CurrentSuite().GetNumTests());
			iteration_wait_frames = iteration_wait_frame_count;

			if (iteration_state == IterationState::Capture)
			{
				if (!CaptureCurrentView())
				{
					StopTestSuiteIteration();
					return;
				}
			}
			else if (iteration_state == IterationState::Comparison)
			{
				RMLUI_ASSERT((int)comparison_results.size() == CurrentSuite().GetNumTests());
				int test_index = CurrentSuite().GetIndex();
				comparison_results[test_index] = CompareCurrentView();
			}

			iteration_index += 1;

			if (CurrentSuite().Next())
				LoadActiveTest();
			else
				StopTestSuiteIteration();
		}
	}
}

void TestNavigator::ProcessEvent(Rml::Event& event)
{
	// Keydown events in capture phase to override text input
	if (event == Rml::EventId::Keydown && event.GetPhase() == Rml::EventPhase::Capture)
	{
		const auto key_identifier = (Rml::Input::KeyIdentifier)event.GetParameter< int >("key_identifier", 0);
		const bool key_ctrl = event.GetParameter< bool >("ctrl_key", false);
		const bool key_shift = event.GetParameter< bool >("shift_key", false);

		Rml::Element* element_filter_input = event.GetCurrentElement()->GetElementById("filterinput");
		RMLUI_ASSERT(element_filter_input);

		if (key_identifier == Rml::Input::KI_F5)
		{
			if (key_ctrl && key_shift)
				StartTestSuiteIteration(IterationState::Comparison);
			else
			{
				ComparisonResult result = CompareCurrentView();
				const Rml::String compare_path = GetCompareInputDirectory() + '/' + GetImageFilenameFromCurrentTest();
				if (result.success)
				{
					if (result.is_equal)
					{
						Rml::Log::Message(Rml::Log::LT_INFO, "%s compares EQUAL to the reference image %s.",
							CurrentSuite().GetFilename().c_str(), compare_path.c_str());
					}
					else
					{
						Rml::Log::Message(Rml::Log::LT_INFO, "%s compares NOT EQUAL to the reference image %s.\nSee diff image written to %s.",
							CurrentSuite().GetFilename().c_str(), compare_path.c_str(), GetCaptureOutputDirectory().c_str());
					}

					if (!result.error_msg.empty())
						Rml::Log::Message(Rml::Log::LT_ERROR, "%s", result.error_msg.c_str());
				}
				else
				{
					Rml::Log::Message(Rml::Log::LT_ERROR, "Comparison of %s failed.\n%s", CurrentSuite().GetFilename().c_str(), result.error_msg.c_str());
				}
			}
		}
		else if (key_identifier == Rml::Input::KI_F7)
		{
			if (key_ctrl && key_shift)
			{
				StartTestSuiteIteration(IterationState::Capture);
			}
			else
			{
				const Rml::String filepath = GetCaptureOutputDirectory() + '/' + GetImageFilenameFromCurrentTest();
				if (CaptureCurrentView())
					Rml::Log::Message(Rml::Log::LT_INFO, "Succesfully captured and saved screenshot to %s", filepath.c_str());
				else
					Rml::Log::Message(Rml::Log::LT_ERROR, "Could not capture screenshot to %s", filepath.c_str());
			}
		}
		else if (key_identifier == Rml::Input::KI_F1)
		{
			viewer->ShowHelp(!viewer->IsHelpVisible());
		}
		else if (key_identifier == Rml::Input::KI_F && key_ctrl)
		{
			element_filter_input->Focus();
			context->ProcessKeyDown(Rml::Input::KI_A, Rml::Input::KeyModifier::KM_CTRL);
			context->ProcessKeyUp(Rml::Input::KI_A, Rml::Input::KeyModifier::KM_CTRL);
		}
		else if (key_identifier == Rml::Input::KI_R && key_ctrl)
		{
			LoadActiveTest();
		}
		else if (key_identifier == Rml::Input::KI_S && key_ctrl)
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
			if (iteration_state != IterationState::None)
			{
				StopTestSuiteIteration();
			}
			else if (viewer->IsHelpVisible())
			{
				viewer->ShowHelp(false);
			}
			else if (source_state != SourceType::None)
			{
				source_state = SourceType::None;
				viewer->ShowSource(source_state);
			}
			else if (element_filter_input->IsPseudoClassSet("focus"))
			{
				element_filter_input->Blur();
			}
			else if (goto_index >= 0)
			{
				goto_index = -1;
				UpdateGoToText();
			}
		}
		else if (key_identifier == Rml::Input::KI_RETURN)
		{
			element_filter_input->Blur();
		}
		else if (key_identifier == Rml::Input::KI_G && key_ctrl)
		{
			if (goto_index < 0)
			{
				goto_index = 0;
				UpdateGoToText();
				element_filter_input->Blur();
			}
		}
	}

	// Keydown events in target/bubble phase ignored when focusing on input.
	if (event == Rml::EventId::Keydown && event.GetPhase() != Rml::EventPhase::Capture)
	{
		const auto key_identifier = (Rml::Input::KeyIdentifier)event.GetParameter< int >("key_identifier", 0);

		if (key_identifier == Rml::Input::KI_LEFT)
		{
			if (CurrentSuite().Previous())
			{
				LoadActiveTest();
			}
		}
		else if (key_identifier == Rml::Input::KI_RIGHT)
		{
			if (CurrentSuite().Next())
			{
				LoadActiveTest();
			}
		}
		else if (key_identifier == Rml::Input::KI_UP)
		{
			const Rml::String& filter = CurrentSuite().GetFilter();
			int new_index = std::max(0, suite_index - 1);
			if (new_index != suite_index)
			{
				suite_index = new_index;
				CurrentSuite().SetFilter(filter);
				LoadActiveTest();
			}
		}
		else if (key_identifier == Rml::Input::KI_DOWN)
		{
			const Rml::String& filter = CurrentSuite().GetFilter();
			int new_index = std::min((int)test_suites.size() - 1, suite_index + 1);
			if (new_index != suite_index)
			{
				suite_index = new_index;
				CurrentSuite().SetFilter(filter);
				LoadActiveTest();
			}
		}
		else if (key_identifier == Rml::Input::KI_HOME)
		{
			CurrentSuite().SetIndex(0, TestSuite::Direction::Forward);
			LoadActiveTest();
		}
		else if (key_identifier == Rml::Input::KI_END)
		{
			CurrentSuite().SetIndex(CurrentSuite().GetNumTests() - 1, TestSuite::Direction::Backward);
			LoadActiveTest();
		}
		else if (goto_index >= 0 && key_identifier == Rml::Input::KI_BACK)
		{
			if (goto_index <= 0)
				goto_index = -1;
			else
				goto_index = goto_index / 10;
			
			UpdateGoToText();
		}
	}

	if (event == Rml::EventId::Textinput && goto_index >= 0)
	{
		const Rml::String text = event.GetParameter< Rml::String >("text", "");

		for (const char c : text)
		{
			if (c >= '0' && c <= '9')
			{
				goto_index = goto_index * 10 + int(c - '0');
				UpdateGoToText();
			}
			else if (goto_index >= 0 && c == '\n')
			{
				bool out_of_bounds = false;

				if (goto_index > 0)
				{
					if (CurrentSuite().SetIndex(goto_index - 1))
						LoadActiveTest();
					else
						out_of_bounds = true;
				}

				goto_index = -1;
				UpdateGoToText(out_of_bounds);
			}
		}
	}

	if (event == Rml::EventId::Change)
	{
		Rml::Element* element = event.GetTargetElement();
		if (element->GetId() == "filterinput")
		{
			CurrentSuite().SetFilter(event.GetParameter< Rml::String >("value", ""));
			LoadActiveTest();
		}
	}
}

void TestNavigator::LoadActiveTest()
{
	const TestSuite& suite = CurrentSuite();
	viewer->LoadTest(suite.GetDirectory(), suite.GetFilename(), suite.GetIndex(), suite.GetNumTests(), suite.GetFilterIndex(), suite.GetNumFilteredTests(), suite_index, (int)test_suites.size());
	viewer->ShowSource(source_state);
	UpdateGoToText();
}

Rml::String TestNavigator::GetImageFilenameFromCurrentTest()
{
	const Rml::String& filename = CurrentSuite().GetFilename();
	return filename.substr(0, filename.rfind('.')) + ".png";
}

ComparisonResult TestNavigator::CompareCurrentView()
{
	const Rml::String filename = GetImageFilenameFromCurrentTest();

	ComparisonResult result = CompareScreenToPreviousCapture(shell_renderer, filename);

	return result;
}


bool TestNavigator::CaptureCurrentView()
{
	const Rml::String filename = GetImageFilenameFromCurrentTest();
	
	bool result = CaptureScreenshot(shell_renderer, filename, 1060);
	
	return result;
}

void TestNavigator::StartTestSuiteIteration(IterationState new_iteration_state)
{
	if (iteration_state != IterationState::None || new_iteration_state == IterationState::None)
		return;

	source_state = SourceType::None;

	TestSuite& suite = CurrentSuite();

	if (new_iteration_state == IterationState::Comparison)
	{
		comparison_results.clear();
		comparison_results.resize(suite.GetNumTests());
	}

	iteration_initial_index = suite.GetIndex();
	iteration_wait_frames = iteration_wait_frame_count;

	iteration_state = new_iteration_state;
	iteration_index = 0;
	suite.SetIndex(iteration_index, TestSuite::Direction::Forward);
	LoadActiveTest();
}

static bool SaveFile(const Rml::String& file_path, const Rml::String& contents)
{
	std::FILE* file = std::fopen(file_path.c_str(), "wt");
	if (!file)
		return false;

	std::fputs(contents.c_str(), file);
	std::fclose(file);

	return true;
}

void TestNavigator::StopTestSuiteIteration()
{
	if (iteration_state == IterationState::None)
		return;

	const Rml::String output_directory = GetCaptureOutputDirectory();
	TestSuite& suite = CurrentSuite();
	const int num_tests = suite.GetNumTests();
	const int num_filtered_tests = suite.GetNumFilteredTests();

	if (iteration_state == IterationState::Capture)
	{
		if (iteration_index == num_tests)
		{
			Rml::Log::Message(Rml::Log::LT_INFO, "Successfully captured %d document screenshots to directory: %s", iteration_index, output_directory.c_str());
		}
		else if (iteration_index == num_filtered_tests)
		{
			Rml::Log::Message(Rml::Log::LT_INFO, "Successfully captured %d document screenshots (filtered out of %d total tests) to directory: %s", iteration_index, num_tests, output_directory.c_str());
		}
		else
		{
			Rml::Log::Message(Rml::Log::LT_ERROR, "Test suite capture aborted after %d of %d test(s). Output directory: %s", iteration_index, num_tests, output_directory.c_str());
		}
	}
	else if (iteration_state == IterationState::Comparison)
	{
		RMLUI_ASSERT(num_tests == (int)comparison_results.size());

		// Indices
		Rml::Vector<int> equal;
		Rml::Vector<int> not_equal;
		Rml::Vector<int> failed;
		Rml::Vector<int> skipped;
		 
		for(int i = 0; i < (int)comparison_results.size(); i++)
		{
			const ComparisonResult& comparison = comparison_results[i];

			if (comparison.skipped)
				skipped.push_back(i);
			else if (!comparison.success)
				failed.push_back(i);
			else if (comparison.is_equal)
				equal.push_back(i);
			else
				not_equal.push_back(i);
		}

		Rml::String summary = Rml::CreateString(256, "  Total tests: %d\n  Equal: %d\n  Not equal: %d\n  Failed: %d\n  Skipped: %d",
			num_tests, (int)equal.size(), (int)not_equal.size(), (int)failed.size(), (int)skipped.size());

		if (!suite.GetFilter().empty())
			summary += "\n  Filter applied: " + suite.GetFilter();

		if (iteration_index == num_tests)
		{
			Rml::Log::Message(Rml::Log::LT_INFO, "Compared all test documents to their screenshot captures.\n%s", summary.c_str());
		}
		else if (iteration_index == num_filtered_tests)
		{
			Rml::Log::Message(Rml::Log::LT_INFO, "Compared all filtered test documents to their screenshot captures.\n%s", summary.c_str());
		}
		else
		{
			Rml::Log::Message(Rml::Log::LT_ERROR, "Test suite comparison aborted after %d of %d test(s).\n%s", iteration_index, num_tests, summary.c_str());
		}

		Rml::String log;
		log.reserve(comparison_results.size() * 100);

		log += "RmlUi VisualTests comparison log output\n---------------------------------------\n\n" + summary;
		log += "\n\nNot Equal:\n";

		if (!not_equal.empty())
			log += "Percentages are similarity scores. Difference images written to " + GetCaptureOutputDirectory() + "/*-diff.png\n\n";
		for (int i : not_equal)
		{
			suite.SetIndex(i);
			log += Rml::CreateString(256, "%5d   %5.1f%%   %s\n", i + 1, comparison_results[i].similarity_score*100.0, suite.GetFilename().c_str());
			if (!comparison_results[i].error_msg.empty())
				log += "          " + comparison_results[i].error_msg + "\n";
		}
		log += "\nEqual:\n";
		for (int i : equal)
		{
			suite.SetIndex(i);
			log += Rml::CreateString(256, "%5d   %s\n", i + 1, suite.GetFilename().c_str());
		}
		log += "\nFailed:\n";
		for (int i : failed)
		{
			suite.SetIndex(i);
			log += Rml::CreateString(512, "%5d   %s\n", i + 1, suite.GetFilename().c_str());
			log += "          " + comparison_results[i].error_msg + "\n";
		}
		log += "\nSkipped:\n";
		for (int i : skipped)
		{
			suite.SetIndex(i);
			log += Rml::CreateString(256, "%5d   %s\n", i + 1, suite.GetFilename().c_str());
		}

		const Rml::String log_path = GetCaptureOutputDirectory() + "/comparison.log";
		bool save_result = SaveFile(log_path, log);
		if (save_result && failed.empty())
			Rml::Log::Message(Rml::Log::LT_INFO, "Comparison log output written to %s", log_path.c_str());
		else if(save_result && !failed.empty())
			Rml::Log::Message(Rml::Log::LT_ERROR, "Comparison log output written to %s.\nSome comparisons failed, see log output for details.", log_path.c_str());
		else
			Rml::Log::Message(Rml::Log::LT_ERROR, "Failed writing comparison log output to file %s", log_path.c_str());
	}

	suite.SetIndex(iteration_initial_index);
	LoadActiveTest();

	iteration_index = -1;
	iteration_initial_index = -1;
	iteration_wait_frames = -1;
	iteration_state = IterationState::None;
}

void TestNavigator::UpdateGoToText(bool out_of_bounds)
{
	if (out_of_bounds)
		viewer->SetGoToText("Go To out of bounds");
	else if (goto_index > 0)
		viewer->SetGoToText(Rml::CreateString(64, "Go To: %d", goto_index));
	else if(goto_index == 0)
		viewer->SetGoToText("Go To:");
	else
		viewer->SetGoToText("Press 'F1' for keyboard shortcuts.");
}



