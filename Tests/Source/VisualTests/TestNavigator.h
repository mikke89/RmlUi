#pragma once

#include "CaptureScreen.h"
#include "TestSuite.h"
#include "TestViewer.h"
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Types.h>

class TestNavigator : public Rml::EventListener {
public:
	TestNavigator(Rml::RenderInterface* render_interface, Rml::Context* context, TestViewer* viewer, TestSuiteList test_suites, int start_suite,
		int start_case);
	~TestNavigator();

	void Update();

	void Render();

protected:
	void ProcessEvent(Rml::Event& event) override;

private:
	enum class IterationState { None, Capture, Comparison };
	enum class ReferenceState { None, ShowReference, ShowReferenceHighlight };

	TestSuite& CurrentSuite() { return test_suites[suite_index]; }

	void LoadActiveTest(bool keep_scroll_position = false);

	ComparisonResult CompareCurrentView();

	bool CaptureCurrentView();

	void StartTestSuiteIteration(IterationState iteration_state);
	void StopTestSuiteIteration();

	void StartGoTo();
	void CancelGoTo();
	void UpdateGoToText(bool out_of_bounds = false);

	void ShowReference(ReferenceState new_reference_state);

	Rml::String GetImageFilenameFromCurrentTest();

	Rml::RenderInterface* render_interface;
	Rml::Context* context;
	TestViewer* viewer;
	TestSuiteList test_suites;

	Rml::String test_filter;

	int suite_index = 0;
	int goto_index = -1;
	SourceType source_state = SourceType::None;

	ReferenceState reference_state = ReferenceState::None;
	ComparisonResult reference_comparison;
	TextureGeometry reference_geometry;
	TextureGeometry reference_highlight_geometry;

	IterationState iteration_state = IterationState::None;

	int iteration_index = -1;
	int iteration_initial_index = -1;
	int iteration_wait_frames = -1;

	Rml::Vector<ComparisonResult> comparison_results;
};
