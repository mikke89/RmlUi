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

#include "../Common/TestsShell.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <doctest.h>
#include <algorithm>

using namespace Rml;

static const String document_textures_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body {
			left: 0;
			top: 0;
			right: 0;
			bottom: 0;
		}
		div.file {
			height: 100px;
			decorator: image(/assets/high_scores_alien_2.tga);
		}
		@spritesheet aliens {
			src: /assets/high_scores_alien_3.tga;
			alien3: 0px 0px 64px 64px;
		}
		div.sprite {
			height: 100px;
			decorator: image(alien3);
		}
		progress { 
			display: block;
			width: 50px;
			height: 50px;
			background: #333;
			fill-image: /assets/high_scores_defender.tga;
		}
	</style>
</head>

<body>
<div style="display: block">
	<img src="/assets/high_scores_alien_1.tga"/>
	<div class="file"/>
	<div class="sprite"/>
	<progress direction="clockwise" start-edge="bottom" value="0.5"/>
</div>
</body>
</rml>
)";

static inline StringList GetSortedTextureSourceList()
{
	StringList list = Rml::GetTextureSourceList();
	std::sort(list.begin(), list.end());
	return list;
}

TEST_CASE("core.texture_source_list")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	REQUIRE(GetSortedTextureSourceList().size() == 0);

	// We should be able to detect all sources even though they are hidden.
	const StringList list_expected = {
		"assets/high_scores_alien_1.tga",
		"assets/high_scores_alien_2.tga",
		"assets/high_scores_alien_3.tga",
		"assets/high_scores_defender.tga",
	};

	ElementDocument* document = context->LoadDocumentFromMemory(document_textures_rml);
	REQUIRE(document);
	CHECK(GetSortedTextureSourceList() == list_expected);

	document->Show();
	CHECK(GetSortedTextureSourceList() == list_expected);

	context->Update();
	CHECK(GetSortedTextureSourceList() == list_expected);

	context->Render();
	CHECK(GetSortedTextureSourceList() == list_expected);

	TestsShell::RenderLoop();

	document->Close();

	TestsShell::ShutdownShell();
}
