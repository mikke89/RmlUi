#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <doctest.h>

using namespace Rml;

static const char* document_handle_rml = R"(
<rml>
<head>
	<title>Handle Test</title>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
body {
	left: 0;
	top: 0;
	width: 500px;
	height: 500px;
	background-color: #ccc;
}
#handle {
	position: absolute;
	top: -10px;
	left: -10px;
	width: 20px;
	height: 20px;
	background-color: #000
}
#target {
	position: absolute;
	background: #a33;
}
	</style>
</head>

<body>
	<handle id="handle"></handle>
	<div id="target"></div>
</body>
</rml>
)";

TEST_CASE("ElementHandle")
{
	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocumentFromMemory(document_handle_rml, "assets/");
	document->Show();

	Element* target = document->GetElementById("target");
	Element* handle = document->GetElementById("handle");
	REQUIRE(target);
	REQUIRE(handle);

	auto dispatch_mouse_event = [](EventId id, Element* dispatch_target, Vector2f mouse_pos) {
		Dictionary drag_start_parameters;
		drag_start_parameters["mouse_x"] = mouse_pos.x;
		drag_start_parameters["mouse_y"] = mouse_pos.y;
		dispatch_target->DispatchEvent(id, drag_start_parameters);
	};

	auto document_set_size = [&](Vector2f new_size) {
		document->SetProperty(PropertyId::Width, Property{new_size.x, Unit::PX});
		document->SetProperty(PropertyId::Height, Property{new_size.y, Unit::PX});
		context->Update();
	};

	SUBCASE("MoveTargetWithTopLeft")
	{
		handle->SetAttribute("move_target", "target");

		target->SetProperty(PropertyId::Top, Property(0, Unit::PX));
		target->SetProperty(PropertyId::Left, Property(0, Unit::PX));
		target->SetProperty(PropertyId::Width, Property(100, Unit::PX));
		target->SetProperty(PropertyId::Height, Property(100, Unit::PX));

		dispatch_mouse_event(EventId::Dragstart, handle, {0, 0});
		dispatch_mouse_event(EventId::Drag, handle, {10, 10});

		CHECK(target->GetProperty(PropertyId::Top)->Get<float>() == 10);
		CHECK(target->GetProperty(PropertyId::Left)->Get<float>() == 10);

		context->Update();
		CHECK(target->GetAbsoluteOffset() == Vector2f(10, 10));
		CHECK(target->GetBox().GetSize() == Vector2f(100, 100));

		document_set_size({1000, 1000});
		CHECK(target->GetAbsoluteOffset() == Vector2f(10, 10));
		CHECK(target->GetBox().GetSize() == Vector2f(100, 100));
	}

	SUBCASE("MoveTargetWithTopLeftRightBottom")
	{
		handle->SetAttribute("move_target", "target");

		target->SetProperty(PropertyId::Top, Property(50, Unit::PX));
		target->SetProperty(PropertyId::Left, Property(50, Unit::PX));
		target->SetProperty(PropertyId::Right, Property(50, Unit::PX));
		target->SetProperty(PropertyId::Bottom, Property(50, Unit::PX));

		context->Update();
		REQUIRE(target->GetAbsoluteOffset() == Vector2f(50, 50));
		REQUIRE(target->GetBox().GetSize() == Vector2f(400, 400));

		dispatch_mouse_event(EventId::Dragstart, handle, {0, 0});
		dispatch_mouse_event(EventId::Drag, handle, {10, 10});

		context->Update();
		CHECK(target->GetAbsoluteOffset() == Vector2f(60, 60));
		CHECK(target->GetBox().GetSize() == Vector2f(400, 400));

		document_set_size({1000, 1000});
		CHECK(target->GetAbsoluteOffset() == Vector2f(60, 60));
		CHECK(target->GetBox().GetSize() == Vector2f(900, 900));
	}

	SUBCASE("MoveTargetWithWidthHeightBottomRight")
	{
		handle->SetAttribute("move_target", "target");

		target->SetProperty(PropertyId::Width, Property(100, Unit::PX));
		target->SetProperty(PropertyId::Height, Property(100, Unit::PX));
		target->SetProperty(PropertyId::Bottom, Property(50, Unit::PX));
		target->SetProperty(PropertyId::Right, Property(50, Unit::PX));

		context->Update();
		REQUIRE(target->GetBox().GetSize() == Vector2f(100, 100));

		dispatch_mouse_event(EventId::Dragstart, handle, {0, 0});
		dispatch_mouse_event(EventId::Drag, handle, {10, 10});

		context->Update();
		CHECK(target->GetAbsoluteOffset() == Vector2f(360, 360));
		CHECK(target->GetBox().GetSize() == Vector2f(100, 100));

		document_set_size({1000, 1000});
		CHECK(target->GetAbsoluteOffset() == Vector2f(860, 860));
		CHECK(target->GetBox().GetSize() == Vector2f(100, 100));
	}

	SUBCASE("MoveTargetWithTopLeftRightBottomAndWidthHeight")
	{
		handle->SetAttribute("move_target", "target");

		target->SetProperty(PropertyId::Top, Property(50, Unit::PX));
		target->SetProperty(PropertyId::Left, Property(50, Unit::PX));
		target->SetProperty(PropertyId::Right, Property(50, Unit::PX));
		target->SetProperty(PropertyId::Bottom, Property(50, Unit::PX));
		target->SetProperty(PropertyId::Width, Property(100, Unit::PX));
		target->SetProperty(PropertyId::Height, Property(100, Unit::PX));

		context->Update();
		REQUIRE(target->GetAbsoluteOffset() == Vector2f(50, 50));
		REQUIRE(target->GetBox().GetSize() == Vector2f(100, 100));

		dispatch_mouse_event(EventId::Dragstart, handle, {0, 0});
		dispatch_mouse_event(EventId::Drag, handle, {10, 10});

		context->Update();
		CHECK(target->GetAbsoluteOffset() == Vector2f(60, 60));
		CHECK(target->GetBox().GetSize() == Vector2f(100, 100));

		document_set_size({1000, 1000});
		CHECK(target->GetAbsoluteOffset() == Vector2f(60, 60));
		CHECK(target->GetBox().GetSize() == Vector2f(100, 100));
	}

	SUBCASE("EdgeMarginDefaultConstrainsMoveTarget")
	{
		handle->SetAttribute("move_target", "target");

		target->SetProperty(PropertyId::Top, Property(50, Unit::PX));
		target->SetProperty(PropertyId::Left, Property(50, Unit::PX));
		target->SetProperty(PropertyId::Width, Property(100, Unit::PX));
		target->SetProperty(PropertyId::Height, Property(100, Unit::PX));

		context->Update();
		dispatch_mouse_event(EventId::Dragstart, handle, {0, 0});
		dispatch_mouse_event(EventId::Drag, handle, {-1000, -1000});

		context->Update();
		CHECK(target->GetAbsoluteOffset() == Vector2f(0, 0));
		CHECK(target->GetBox().GetSize() == Vector2f(100, 100));
	}

	SUBCASE("EdgeMarginLengthConstrainsMoveTarget")
	{
		handle->SetAttribute("move_target", "target");
		handle->SetAttribute("edge_margin", "10px 10px 10px 20px");

		target->SetProperty(PropertyId::Top, Property(50, Unit::PX));
		target->SetProperty(PropertyId::Left, Property(50, Unit::PX));
		target->SetProperty(PropertyId::Width, Property(100, Unit::PX));
		target->SetProperty(PropertyId::Height, Property(100, Unit::PX));

		context->Update();
		dispatch_mouse_event(EventId::Dragstart, handle, {0, 0});
		dispatch_mouse_event(EventId::Drag, handle, {-1000, -1000});

		context->Update();
		CHECK(target->GetAbsoluteOffset() == Vector2f(20, 10));
		CHECK(target->GetBox().GetSize() == Vector2f(100, 100));
	}

	SUBCASE("EdgeMarginPercentageConstrainsMoveTarget")
	{
		handle->SetAttribute("move_target", "target");
		handle->SetAttribute("edge_margin", "-50%");

		target->SetProperty(PropertyId::Top, Property(50, Unit::PX));
		target->SetProperty(PropertyId::Left, Property(50, Unit::PX));
		target->SetProperty(PropertyId::Width, Property(100, Unit::PX));
		target->SetProperty(PropertyId::Height, Property(100, Unit::PX));

		context->Update();
		dispatch_mouse_event(EventId::Dragstart, handle, {0, 0});
		dispatch_mouse_event(EventId::Drag, handle, {-1000, -1000});

		context->Update();
		CHECK(target->GetAbsoluteOffset() == Vector2f(-50, -50));
		CHECK(target->GetBox().GetSize() == Vector2f(100, 100));
	}

	SUBCASE("EdgeMarginNoneUnconstrainsMoveTarget")
	{
		handle->SetAttribute("move_target", "target");
		handle->SetAttribute("edge_margin", "none");

		target->SetProperty(PropertyId::Top, Property(50, Unit::PX));
		target->SetProperty(PropertyId::Left, Property(50, Unit::PX));
		target->SetProperty(PropertyId::Width, Property(100, Unit::PX));
		target->SetProperty(PropertyId::Height, Property(100, Unit::PX));

		context->Update();
		dispatch_mouse_event(EventId::Dragstart, handle, {0, 0});
		dispatch_mouse_event(EventId::Drag, handle, {-1000, -1000});

		context->Update();
		CHECK(target->GetAbsoluteOffset() == Vector2f(-950, -950));
		CHECK(target->GetBox().GetSize() == Vector2f(100, 100));
	}

	SUBCASE("EdgeMarginLengthConstrainsSizeTarget")
	{
		handle->SetAttribute("size_target", "target");
		handle->SetAttribute("edge_margin", "10px");

		target->SetProperty(PropertyId::Bottom, Property(50, Unit::PX));
		target->SetProperty(PropertyId::Right, Property(50, Unit::PX));
		target->SetProperty(PropertyId::Width, Property(100, Unit::PX));
		target->SetProperty(PropertyId::Height, Property(100, Unit::PX));

		context->Update();
		CHECK(target->GetAbsoluteOffset() == Vector2f(350, 350));
		CHECK(target->GetBox().GetSize() == Vector2f(100, 100));

		dispatch_mouse_event(EventId::Dragstart, handle, {0, 0});
		dispatch_mouse_event(EventId::Drag, handle, {500, 500});

		context->Update();
		CHECK(target->GetAbsoluteOffset() == Vector2f(350, 350));
		CHECK(target->GetBox().GetSize() == Vector2f(140, 140));
	}

	SUBCASE("EdgeMarginPercentageConstrainsSizeTarget")
	{
		handle->SetAttribute("size_target", "target");
		handle->SetAttribute("edge_margin", "-50%");

		target->SetProperty(PropertyId::Bottom, Property(50, Unit::PX));
		target->SetProperty(PropertyId::Right, Property(50, Unit::PX));
		target->SetProperty(PropertyId::Width, Property(100, Unit::PX));
		target->SetProperty(PropertyId::Height, Property(100, Unit::PX));

		context->Update();
		CHECK(target->GetAbsoluteOffset() == Vector2f(350, 350));
		CHECK(target->GetBox().GetSize() == Vector2f(100, 100));

		dispatch_mouse_event(EventId::Dragstart, handle, {0, 0});
		dispatch_mouse_event(EventId::Drag, handle, {500, 500});

		context->Update();
		CHECK(target->GetAbsoluteOffset() == Vector2f(350, 350));
		CHECK(target->GetBox().GetSize() == Vector2f(200, 200));
	}

	SUBCASE("AutoMarginMove")
	{
		handle->SetAttribute("move_target", "target");

		target->SetProperty(PropertyId::Width, Property(100, Unit::PX));
		target->SetProperty(PropertyId::Height, Property(100, Unit::PX));
		target->SetProperty("margin", "auto");

		context->Update();
		CHECK(target->GetAbsoluteOffset() == Vector2f(200, 200));
		CHECK(target->GetBox().GetSize() == Vector2f(100, 100));

		dispatch_mouse_event(EventId::Dragstart, handle, {0, 0});
		dispatch_mouse_event(EventId::Drag, handle, {10, 10});

		CHECK(target->GetProperty(PropertyId::MarginTop)->Get<float>() == 200);
		CHECK(target->GetProperty(PropertyId::MarginRight)->Get<float>() == 200);
		CHECK(target->GetProperty(PropertyId::MarginBottom)->Get<float>() == 200);
		CHECK(target->GetProperty(PropertyId::MarginLeft)->Get<float>() == 200);
		CHECK(target->GetProperty(PropertyId::Top)->Get<float>() == 10);
		CHECK(target->GetProperty(PropertyId::Left)->Get<float>() == 10);

		context->Update();
		CHECK(target->GetAbsoluteOffset() == Vector2f(210, 210));
		CHECK(target->GetBox().GetSize() == Vector2f(100, 100));

		document_set_size({1000, 1000});
		CHECK(target->GetAbsoluteOffset() == Vector2f(210, 210));
		CHECK(target->GetBox().GetSize() == Vector2f(100, 100));
	}

	SUBCASE("AutoMarginMoveConstraints")
	{
		handle->SetAttribute("move_target", "target");

		target->SetProperty(PropertyId::Width, Property(100, Unit::PX));
		target->SetProperty(PropertyId::Height, Property(100, Unit::PX));
		target->SetProperty("margin", "auto");

		context->Update();
		CHECK(target->GetAbsoluteOffset() == Vector2f(200, 200));
		CHECK(target->GetBox().GetSize() == Vector2f(100, 100));

		dispatch_mouse_event(EventId::Dragstart, handle, {0, 0});
		dispatch_mouse_event(EventId::Drag, handle, {-1000, -1000});

		CHECK(target->GetProperty(PropertyId::Top)->Get<float>() == -200);
		CHECK(target->GetProperty(PropertyId::Left)->Get<float>() == -200);

		context->Update();
		CHECK(target->GetAbsoluteOffset() == Vector2f(0, 0));
		CHECK(target->GetBox().GetSize() == Vector2f(100, 100));
	}

	SUBCASE("AutoMarginSize")
	{
		handle->SetAttribute("size_target", "target");

		target->SetProperty(PropertyId::Width, Property(100, Unit::PX));
		target->SetProperty(PropertyId::Height, Property(100, Unit::PX));
		target->SetProperty("margin", "auto");

		context->Update();
		CHECK(target->GetAbsoluteOffset() == Vector2f(200, 200));
		CHECK(target->GetBox().GetSize() == Vector2f(100, 100));

		dispatch_mouse_event(EventId::Dragstart, handle, {0, 0});
		dispatch_mouse_event(EventId::Drag, handle, {10, 10});

		CHECK(target->GetProperty(PropertyId::MarginTop)->Get<float>() == 200);
		CHECK(target->GetProperty(PropertyId::MarginRight)->Get<float>() == 200);
		CHECK(target->GetProperty(PropertyId::MarginBottom)->Get<float>() == 200);
		CHECK(target->GetProperty(PropertyId::MarginLeft)->Get<float>() == 200);
		CHECK(target->GetProperty(PropertyId::Width)->Get<float>() == 110);
		CHECK(target->GetProperty(PropertyId::Height)->Get<float>() == 110);

		context->Update();
		CHECK(target->GetAbsoluteOffset() == Vector2f(200, 200));
		CHECK(target->GetBox().GetSize() == Vector2f(110, 110));
	}

	SUBCASE("AutoMarginSizeConstraints")
	{
		handle->SetAttribute("size_target", "target");

		target->SetProperty(PropertyId::Width, Property(100, Unit::PX));
		target->SetProperty(PropertyId::Height, Property(100, Unit::PX));
		target->SetProperty("margin", "auto");

		context->Update();
		CHECK(target->GetAbsoluteOffset() == Vector2f(200, 200));
		CHECK(target->GetBox().GetSize() == Vector2f(100, 100));

		dispatch_mouse_event(EventId::Dragstart, handle, {0, 0});
		dispatch_mouse_event(EventId::Drag, handle, {1000, 1000});

		context->Update();
		CHECK(target->GetAbsoluteOffset() == Vector2f(200, 200));
		CHECK(target->GetBox().GetSize() == Vector2f(300, 300));
	}

	TestsShell::RenderLoop();

	document->Close();
	TestsShell::ShutdownShell();
}
