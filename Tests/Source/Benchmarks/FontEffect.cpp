/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
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
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Types.h>
#include <doctest.h>
#include <nanobench.h>

using namespace ankerl;
using namespace Rml;

static const String rml_font_effect_document = R"(
<rml>
<head>
    <title>Table</title>
    <link type="text/rcss" href="/../Tests/Data/style.rcss"/>
	<style>
		body {
			font-size: 25px;
			font-effect: %s(%dpx #ff6);
		}
	</style>
</head>
<body>
The quick brown fox jumps over the lazy dog.
</body>
</rml>
)";

TEST_CASE("font_effect")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	nanobench::Bench bench;
	bench.title("Font effect");
	bench.relative(true);

	for (const char* effect_name : {"shadow", "blur", "outline", "glow"})
	{
		constexpr int effect_size = 8;

		const String rml_document = CreateString(rml_font_effect_document.c_str(), effect_name, effect_size);

		ElementDocument* document = context->LoadDocumentFromMemory(rml_document);
		document->Show();
		context->Update();
		context->Render();

		bench.run(effect_name, [&]() {
			Rml::ReleaseFontResources();
			context->Render();
		});

		document->Close();
	}

	TestsShell::ShutdownShell();
}
