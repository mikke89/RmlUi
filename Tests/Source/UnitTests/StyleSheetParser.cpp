/*
* This source file is part of RmlUi, the HTML/CSS Interface Middleware
*
* For the latest information, see http://github.com/mikke89/RmlUi
*
* Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
* Copyright (c) 2019-2024 The RmlUi Team, and contributors
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

#include "../Common/TestsInterface.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/StyleSheetContainer.h>
#include <RmlUi/Core/StyleSheet.h>
#include <RmlUi/Core/Spritesheet.h>
#include <RmlUi/Core/StreamMemory.h>

#include <doctest.h>

static const char* spriteSheet = R"(
@spritesheet testSheet {
	src: /assets/high_scores_alien_3.tga;
	test00: 0px 0px 64px 64px;
	test01: 64px 0px 64px 64px;
	test10: 0px 64px 64px 64px;
	test11: 64px 64px 64px 64px;
	resolution: 1x;
}
)";

static const char* spriteSheetWithPathStringEncoding = R"(
@spritesheet testSheetWithPathStringEncoding {
	src: "/assets/test.tga";
	test00: 0px 0px 128px 128px;
	test01: 128px 0px 128px 128px;
	test10: 0px 128px 128px 128px;
	test11: 128px 128px 128px 128px;
	resolution: 2x;
}
)";

using namespace Rml;

TEST_CASE("spritesheet")
{

	TestsSystemInterface system_interface;
	TestsRenderInterface render_interface;

	SetRenderInterface(&render_interface);
	SetSystemInterface(&system_interface);

	Initialise();

	{
		StyleSheetContainer styleSheetContainer;
		StreamMemory spriteSheetStream{reinterpret_cast<const byte*>(spriteSheet), strlen(spriteSheet)};
		styleSheetContainer.LoadStyleSheetContainer(&spriteSheetStream, 0);

		Context* context = CreateContext("test", Vector2i(1024, 768));
		styleSheetContainer.UpdateCompiledStyleSheet(context);
		const auto* styleSheet = styleSheetContainer.GetCompiledStyleSheet();
		CHECK(styleSheet != nullptr);


		const auto* sprite00 = styleSheet->GetSprite("test00");
		CHECK(sprite00 != nullptr);
		CHECK(sprite00->sprite_sheet->name == "testSheet");
		CHECK(sprite00->sprite_sheet->image_source == "/assets/high_scores_alien_3.tga");
		CHECK(sprite00->rectangle.TopLeft() == Vector2f(0.f, 0.f));
		CHECK(sprite00->rectangle.BottomRight() == Vector2f(64.f, 64.f));
		CHECK(sprite00->sprite_sheet->display_scale == 1.f);

		const auto* sprite01 = styleSheet->GetSprite("test01");
		CHECK(sprite01 != nullptr);
		CHECK(sprite01->sprite_sheet->name == "testSheet");
		CHECK(sprite01->sprite_sheet->image_source == "/assets/high_scores_alien_3.tga");
		CHECK(sprite01->rectangle.TopLeft() == Vector2f(64.f, 0.f));
		CHECK(sprite01->rectangle.BottomRight() == Vector2f(128.f, 64.f));
		CHECK(sprite01->sprite_sheet->display_scale == 1.f);

		const auto* sprite10 = styleSheet->GetSprite("test10");
		CHECK(sprite10 != nullptr);
		CHECK(sprite10->sprite_sheet->name == "testSheet");
		CHECK(sprite10->sprite_sheet->image_source == "/assets/high_scores_alien_3.tga");
		CHECK(sprite10->rectangle.TopLeft() == Vector2f(0.f, 64.f));
		CHECK(sprite10->rectangle.BottomRight() == Vector2f(64.f, 128.f));
		CHECK(sprite10->sprite_sheet->display_scale == 1.f);

		const auto* sprite11 = styleSheet->GetSprite("test11");
		CHECK(sprite11 != nullptr);
		CHECK(sprite11->sprite_sheet->name == "testSheet");
		CHECK(sprite11->sprite_sheet->image_source == "/assets/high_scores_alien_3.tga");
		CHECK(sprite11->rectangle.TopLeft() == Vector2f(64.f, 64.f));
		CHECK(sprite11->rectangle.BottomRight() == Vector2f(128.f, 128.f));
		CHECK(sprite11->sprite_sheet->display_scale == 1.f);

	}
	RemoveContext("test");

	Shutdown();
}

TEST_CASE("spritesheet with path string encoding")
{

	TestsSystemInterface system_interface;
	TestsRenderInterface render_interface;

	SetRenderInterface(&render_interface);
	SetSystemInterface(&system_interface);

	Initialise();

	{
		StyleSheetContainer styleSheetContainer;
		StreamMemory spriteSheetStream{reinterpret_cast<const byte*>(spriteSheetWithPathStringEncoding), strlen(spriteSheetWithPathStringEncoding)};
		styleSheetContainer.LoadStyleSheetContainer(&spriteSheetStream, 0);

		Context* context = CreateContext("test", Vector2i(1024, 768));
		styleSheetContainer.UpdateCompiledStyleSheet(context);
		const auto* styleSheet = styleSheetContainer.GetCompiledStyleSheet();
		CHECK(styleSheet != nullptr);

		const auto* sprite00 = styleSheet->GetSprite("test00");
		CHECK(sprite00 != nullptr);
		CHECK(sprite00->sprite_sheet->name == "testSheetWithPathStringEncoding");
		CHECK(sprite00->sprite_sheet->image_source == "/assets/test.tga");
		CHECK(sprite00->rectangle.TopLeft() == Vector2f(0.f, 0.f));
		CHECK(sprite00->rectangle.BottomRight() == Vector2f(128.f, 128.f));
		CHECK(sprite00->sprite_sheet->display_scale == 0.5f);

		const auto* sprite01 = styleSheet->GetSprite("test01");
		CHECK(sprite01 != nullptr);
		CHECK(sprite01->sprite_sheet->name == "testSheetWithPathStringEncoding");
		CHECK(sprite01->sprite_sheet->image_source == "/assets/test.tga");
		CHECK(sprite01->rectangle.TopLeft() == Vector2f(128.f, 0.f));
		CHECK(sprite01->rectangle.BottomRight() == Vector2f(256.f, 128.f));
		CHECK(sprite01->sprite_sheet->display_scale == 0.5f);

		const auto* sprite10 = styleSheet->GetSprite("test10");
		CHECK(sprite10 != nullptr);
		CHECK(sprite10->sprite_sheet->name == "testSheetWithPathStringEncoding");
		CHECK(sprite10->sprite_sheet->image_source == "/assets/test.tga");
		CHECK(sprite10->rectangle.TopLeft() == Vector2f(0.f, 128.f));
		CHECK(sprite10->rectangle.BottomRight() == Vector2f(128.f, 256.f));
		CHECK(sprite10->sprite_sheet->display_scale == 0.5f);

		const auto* sprite11 = styleSheet->GetSprite("test11");
		CHECK(sprite11 != nullptr);
		CHECK(sprite11->sprite_sheet->name == "testSheetWithPathStringEncoding");
		CHECK(sprite11->sprite_sheet->image_source == "/assets/test.tga");
		CHECK(sprite11->rectangle.TopLeft() == Vector2f(128.f, 128.f));
		CHECK(sprite11->rectangle.BottomRight() == Vector2f(256.f, 256.f));
		CHECK(sprite11->sprite_sheet->display_scale == 0.5f);
	}

	RemoveContext("test");

	Shutdown();
}
