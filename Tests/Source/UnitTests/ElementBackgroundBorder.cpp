#include "../Common/TestsInterface.h"
#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Types.h>
#include <doctest.h>
#include <float.h>

using namespace Rml;

static String GenerateRowsRml(int num_rows, String row_rml)
{
	String rml;
	rml.reserve(num_rows * row_rml.size());
	for (int i = 0; i < num_rows; i++)
		rml += row_rml;
	return rml;
}

static const String document_basic_rml = R"(
<rml>
<head>
<title>Demo</title>
<link type="text/rcss" href="/assets/rml.rcss" />
<link type="text/rcss" href="/../Tests/Data/style.rcss" />
<style>
	body {
		width: 800px;
		height: 800px;
	}
	#wrapper {
		height: 300.3px;
		overflow-y: scroll;
		background-color: #333;
	}
	#wrapper > div {
		height: 100.25px;
		background-color: #c33;
		margin: 5.333px 0;
		position: relative;
	}
</style>
</head>
<body>
	<div id="wrapper">
	</div>
</body>
</rml>
)";

TEST_CASE("ElementBackgroundBorder.render_stats")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_basic_rml);
	REQUIRE(document);
	document->Show();

	constexpr int num_rows = 10;
	const String row_rml = "<div/>";
	const String inner_rml = GenerateRowsRml(num_rows, row_rml);

	Element* wrapper = document->GetElementById("wrapper");
	REQUIRE(wrapper);
	wrapper->SetInnerRML(inner_rml);

	context->Update();
	context->Render();

	TestsShell::RenderLoop();

	TestsRenderInterface* render_interface = TestsShell::GetTestsRenderInterface();
	if (!render_interface)
		return;

	MESSAGE(TestsShell::GetRenderStats());
	render_interface->Reset();

	for (int i = 1; i < 50; i++)
	{
		wrapper->SetScrollTop(1.3333f * float(i));
		context->Update();
		context->Render();
	}

	wrapper->SetScrollTop(FLT_MAX);
	context->Update();
	context->Render();

	// Ensure that we have no unnecessary compile geometry commands. The geometry for the background-border should not
	// change in size as long as scrolling occurs in integer increments.
	CHECK(render_interface->GetCounters().compile_geometry == 0);

	document->Close();
	TestsShell::ShutdownShell();
}

static const String document_relative_offset_rml = R"(
<rml>
<head>
<title>Demo</title>
<link type="text/rcss" href="/assets/rml.rcss" />
<link type="text/rcss" href="/../Tests/Data/style.rcss" />
<style>
	body {
		width: 800px;
		height: 800px;
	}
	#wrapper {
		height: 600px;
		background-color: #333;
	}
	#wrapper > div {
		height: 10.333px;
		background-color: #c33;
		position: relative;
	}
</style>
</head>
<body>
	<div id="wrapper">
	</div>
</body>
</rml>
)";

TEST_CASE("ElementBackgroundBorder.background_edges_line_up_with_relative_offset")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_relative_offset_rml);
	REQUIRE(document);
	document->Show();

	constexpr int num_children = 10;
	const String row_rml = "<div/>";
	const String inner_rml = GenerateRowsRml(num_children, row_rml);

	Element* wrapper = document->GetElementById("wrapper");
	REQUIRE(wrapper);
	wrapper->SetInnerRML(inner_rml);

	context->Update();
	context->Render();

	TestsShell::RenderLoop();

	TestsRenderInterface* render_interface = TestsShell::GetTestsRenderInterface();
	if (!render_interface)
		return;

	MESSAGE(TestsShell::GetRenderStats());
	render_interface->Reset();

	for (int i = 1; i < 100; i++)
	{
		for (int child_index = 0; child_index < num_children; child_index++)
		{
			Element* child = wrapper->GetChild(child_index);
			child->SetProperty(PropertyId::Top, Property(1.3333f * float(i), Unit::PX));
		}

		context->Update();
		context->Render();

		for (int child_index = 0; child_index < num_children - 1; child_index++)
		{
			Element* current_child = wrapper->GetChild(child_index);
			Element* next_child = wrapper->GetChild(child_index + 1);
			Vector2f current_bottom_right = current_child->GetAbsoluteOffset(BoxArea::Border).Round() + current_child->GetRenderBox().GetFillSize();
			Vector2f next_top_left = next_child->GetAbsoluteOffset(BoxArea::Border).Round();
			CHECK(current_bottom_right.y == next_top_left.y);
		}
	}

	// When changing the position using fractional increments we expect the size of the backgrounds to change, resulting
	// in new geometry. This is done to ensure that the top and bottom of each background lines up with the one for the
	// next element, thereby avoiding any gaps.
	CHECK(render_interface->GetCounters().compile_geometry > 0);
	MESSAGE("Compile geometry after movement: ", render_interface->GetCounters().compile_geometry);

	document->Close();
	TestsShell::ShutdownShell();
}
