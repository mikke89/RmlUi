#include "../Common/TestsShell.h"
#include "../../../Source/Core/ElementDefinition.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Factory.h>
#include <RmlUi/Core/ID.h>
#include <RmlUi/Core/Spritesheet.h>
#include <RmlUi/Core/StreamMemory.h>
#include <RmlUi/Core/StyleSheet.h>
#include <RmlUi/Core/StyleSheetContainer.h>
#include <doctest.h>

static const char spritesheet[] = R"(
@spritesheet test_sheet {
	src: /assets/high_scores_alien_3.tga;
	test00: 0px 0px 64px 64px;
	test01: 64px 0px 64px 64px;
	test10: 0px 64px 64px 64px;
	test11: 64px 64px 64px 64px;
	resolution: 1x;
}
)";

static const char spritesheet_with_path_string_encoding[] = R"(
@spritesheet test_sheet_with_path_string_encoding {
	src: "/assets/test.tga";
	test00: 0px 0px 128px 128px;
	test01: 128px 0px 128px 128px;
	test10: 0px 128px 128px 128px;
	test11: 128px 128px 128px 128px;
	resolution: 2x;
}
)";

static const char comments[] = R"(
/* A single line comment */
/* A multiline
   comment */
/** A comment starting with two '*' */
/* A comment ending with two '*' **/
/**
 * A multiline comment
 * starting with two '*'
 * and ending with two '*'
 **/
@spritesheet test_sheet {
	src: /assets/high_scores_alien_3.tga;
	/* A comment in-between rules */
	test00: 0px 0px 64px 64px;
	test01: 64px 0px 64px 64px;
	test10: 0px 64px 64px 64px; /* A comment after a rule */
	test11: 64px 64px 64px 64px;
	/* A comment before a rule */ resolution: 1x;
}
)";

static const char selector_list_with_escaped_comma[] = R"(
.escaped\,class, #normal_match {
	width: 10px;
}
)";

using namespace Rml;

TEST_CASE("style_sheet_parser.spritesheet")
{
	Context* context = TestsShell::GetContext();

	{
		StyleSheetContainer style_sheet_container;
		StreamMemory spritesheet_stream{reinterpret_cast<const byte*>(spritesheet), sizeof(spritesheet) - 1};
		style_sheet_container.LoadStyleSheetContainer(&spritesheet_stream, 0);

		style_sheet_container.UpdateCompiledStyleSheet(context);
		const auto* style_sheet = style_sheet_container.GetCompiledStyleSheet();
		CHECK(style_sheet != nullptr);

		const auto* sprite00 = style_sheet->GetSprite("test00");
		CHECK(sprite00 != nullptr);
		CHECK(sprite00->sprite_sheet->name == "test_sheet");
		CHECK(sprite00->sprite_sheet->texture_source.GetSource() == "/assets/high_scores_alien_3.tga");
		CHECK(sprite00->rectangle.TopLeft() == Vector2f(0.f, 0.f));
		CHECK(sprite00->rectangle.BottomRight() == Vector2f(64.f, 64.f));
		CHECK(sprite00->sprite_sheet->display_scale == 1.f);

		const auto* sprite01 = style_sheet->GetSprite("test01");
		CHECK(sprite01 != nullptr);
		CHECK(sprite01->sprite_sheet->name == "test_sheet");
		CHECK(sprite01->sprite_sheet->texture_source.GetSource() == "/assets/high_scores_alien_3.tga");
		CHECK(sprite01->rectangle.TopLeft() == Vector2f(64.f, 0.f));
		CHECK(sprite01->rectangle.BottomRight() == Vector2f(128.f, 64.f));
		CHECK(sprite01->sprite_sheet->display_scale == 1.f);

		const auto* sprite10 = style_sheet->GetSprite("test10");
		CHECK(sprite10 != nullptr);
		CHECK(sprite10->sprite_sheet->name == "test_sheet");
		CHECK(sprite10->sprite_sheet->texture_source.GetSource() == "/assets/high_scores_alien_3.tga");
		CHECK(sprite10->rectangle.TopLeft() == Vector2f(0.f, 64.f));
		CHECK(sprite10->rectangle.BottomRight() == Vector2f(64.f, 128.f));
		CHECK(sprite10->sprite_sheet->display_scale == 1.f);

		const auto* sprite11 = style_sheet->GetSprite("test11");
		CHECK(sprite11 != nullptr);
		CHECK(sprite11->sprite_sheet->name == "test_sheet");
		CHECK(sprite11->sprite_sheet->texture_source.GetSource() == "/assets/high_scores_alien_3.tga");
		CHECK(sprite11->rectangle.TopLeft() == Vector2f(64.f, 64.f));
		CHECK(sprite11->rectangle.BottomRight() == Vector2f(128.f, 128.f));
		CHECK(sprite11->sprite_sheet->display_scale == 1.f);
	}

	TestsShell::ShutdownShell();
}

TEST_CASE("style_sheet_parser.spritesheet_with_path_string_encoding")
{
	Context* context = TestsShell::GetContext();

	{
		StyleSheetContainer style_sheet_container;
		StreamMemory spritesheet_stream{reinterpret_cast<const byte*>(spritesheet_with_path_string_encoding),
			sizeof(spritesheet_with_path_string_encoding) - 1};
		style_sheet_container.LoadStyleSheetContainer(&spritesheet_stream, 0);

		style_sheet_container.UpdateCompiledStyleSheet(context);
		const auto* style_sheet = style_sheet_container.GetCompiledStyleSheet();
		CHECK(style_sheet != nullptr);

		const auto* sprite00 = style_sheet->GetSprite("test00");
		CHECK(sprite00 != nullptr);
		CHECK(sprite00->sprite_sheet->name == "test_sheet_with_path_string_encoding");
		CHECK(sprite00->sprite_sheet->texture_source.GetSource() == "/assets/test.tga");
		CHECK(sprite00->rectangle.TopLeft() == Vector2f(0.f, 0.f));
		CHECK(sprite00->rectangle.BottomRight() == Vector2f(128.f, 128.f));
		CHECK(sprite00->sprite_sheet->display_scale == 0.5f);

		const auto* sprite01 = style_sheet->GetSprite("test01");
		CHECK(sprite01 != nullptr);
		CHECK(sprite01->sprite_sheet->name == "test_sheet_with_path_string_encoding");
		CHECK(sprite01->sprite_sheet->texture_source.GetSource() == "/assets/test.tga");
		CHECK(sprite01->rectangle.TopLeft() == Vector2f(128.f, 0.f));
		CHECK(sprite01->rectangle.BottomRight() == Vector2f(256.f, 128.f));
		CHECK(sprite01->sprite_sheet->display_scale == 0.5f);

		const auto* sprite10 = style_sheet->GetSprite("test10");
		CHECK(sprite10 != nullptr);
		CHECK(sprite10->sprite_sheet->name == "test_sheet_with_path_string_encoding");
		CHECK(sprite10->sprite_sheet->texture_source.GetSource() == "/assets/test.tga");
		CHECK(sprite10->rectangle.TopLeft() == Vector2f(0.f, 128.f));
		CHECK(sprite10->rectangle.BottomRight() == Vector2f(128.f, 256.f));
		CHECK(sprite10->sprite_sheet->display_scale == 0.5f);

		const auto* sprite11 = style_sheet->GetSprite("test11");
		CHECK(sprite11 != nullptr);
		CHECK(sprite11->sprite_sheet->name == "test_sheet_with_path_string_encoding");
		CHECK(sprite11->sprite_sheet->texture_source.GetSource() == "/assets/test.tga");
		CHECK(sprite11->rectangle.TopLeft() == Vector2f(128.f, 128.f));
		CHECK(sprite11->rectangle.BottomRight() == Vector2f(256.f, 256.f));
		CHECK(sprite11->sprite_sheet->display_scale == 0.5f);
	}

	TestsShell::ShutdownShell();
}

TEST_CASE("style_sheet_parser.comments")
{
	Context* context = TestsShell::GetContext();

	{
		StyleSheetContainer style_sheet_container;
		StreamMemory comments_stream{reinterpret_cast<const byte*>(comments), sizeof(comments) - 1};
		style_sheet_container.LoadStyleSheetContainer(&comments_stream, 0);

		style_sheet_container.UpdateCompiledStyleSheet(context);
		const auto* style_sheet = style_sheet_container.GetCompiledStyleSheet();
		CHECK(style_sheet != nullptr);

		const auto* sprite00 = style_sheet->GetSprite("test00");
		CHECK(sprite00 != nullptr);
		CHECK(sprite00->sprite_sheet->name == "test_sheet");
		CHECK(sprite00->sprite_sheet->texture_source.GetSource() == "/assets/high_scores_alien_3.tga");
		CHECK(sprite00->rectangle.TopLeft() == Vector2f(0.f, 0.f));
		CHECK(sprite00->rectangle.BottomRight() == Vector2f(64.f, 64.f));
		CHECK(sprite00->sprite_sheet->display_scale == 1.f);

		const auto* sprite01 = style_sheet->GetSprite("test01");
		CHECK(sprite01 != nullptr);
		CHECK(sprite01->sprite_sheet->name == "test_sheet");
		CHECK(sprite01->sprite_sheet->texture_source.GetSource() == "/assets/high_scores_alien_3.tga");
		CHECK(sprite01->rectangle.TopLeft() == Vector2f(64.f, 0.f));
		CHECK(sprite01->rectangle.BottomRight() == Vector2f(128.f, 64.f));
		CHECK(sprite01->sprite_sheet->display_scale == 1.f);

		const auto* sprite10 = style_sheet->GetSprite("test10");
		CHECK(sprite10 != nullptr);
		CHECK(sprite10->sprite_sheet->name == "test_sheet");
		CHECK(sprite10->sprite_sheet->texture_source.GetSource() == "/assets/high_scores_alien_3.tga");
		CHECK(sprite10->rectangle.TopLeft() == Vector2f(0.f, 64.f));
		CHECK(sprite10->rectangle.BottomRight() == Vector2f(64.f, 128.f));
		CHECK(sprite10->sprite_sheet->display_scale == 1.f);

		const auto* sprite11 = style_sheet->GetSprite("test11");
		CHECK(sprite11 != nullptr);
		CHECK(sprite11->sprite_sheet->name == "test_sheet");
		CHECK(sprite11->sprite_sheet->texture_source.GetSource() == "/assets/high_scores_alien_3.tga");
		CHECK(sprite11->rectangle.TopLeft() == Vector2f(64.f, 64.f));
		CHECK(sprite11->rectangle.BottomRight() == Vector2f(128.f, 128.f));
		CHECK(sprite11->sprite_sheet->display_scale == 1.f);
	}

	TestsShell::ShutdownShell();
}

TEST_CASE("style_sheet_parser.selector_list_with_escaped_comma")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	StyleSheetContainer style_sheet_container;
	StreamMemory stylesheet_stream{reinterpret_cast<const byte*>(selector_list_with_escaped_comma), sizeof(selector_list_with_escaped_comma) - 1};
	REQUIRE(style_sheet_container.LoadStyleSheetContainer(&stylesheet_stream, 0));

	style_sheet_container.UpdateCompiledStyleSheet(context);
	const auto* style_sheet = style_sheet_container.GetCompiledStyleSheet();
	REQUIRE(style_sheet != nullptr);

	{
		ElementPtr class_match = Factory::InstanceElement(nullptr, "*", "div", {});
		ElementPtr normal_match = Factory::InstanceElement(nullptr, "*", "div", {});
		ElementPtr no_match = Factory::InstanceElement(nullptr, "*", "div", {});
		REQUIRE(class_match);
		REQUIRE(normal_match);
		REQUIRE(no_match);

		class_match->SetClassNames("escaped,class");
		normal_match->SetId("normal_match");
		no_match->SetId("no_match");

		SharedPtr<const ElementDefinition> class_definition = style_sheet->GetElementDefinition(class_match.get());
		SharedPtr<const ElementDefinition> normal_definition = style_sheet->GetElementDefinition(normal_match.get());
		SharedPtr<const ElementDefinition> no_definition = style_sheet->GetElementDefinition(no_match.get());

		REQUIRE(bool(class_definition));
		REQUIRE(bool(normal_definition));
		CHECK_FALSE(bool(no_definition));

		const Property* class_width = class_definition->GetProperty(PropertyId::Width);
		const Property* normal_width = normal_definition->GetProperty(PropertyId::Width);
		REQUIRE(class_width);
		REQUIRE(normal_width);
		REQUIRE(bool(class_width->source));
		REQUIRE(bool(normal_width->source));
		CHECK(class_width->source->rule_name == ".escaped,class");
		CHECK(normal_width->source->rule_name == "#normal_match");
	}

	TestsShell::ShutdownShell();
}
