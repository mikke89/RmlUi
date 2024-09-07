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
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <doctest.h>

using namespace Rml;

static const String document_flex_rml = R"(
<rml>
<head>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<style>
		body {
			width: 500px;
			height: 300px;
			background: #333;
		}
		#flex {
			background: #666;
			display: flex;
			width: 50%;
			height: 30%;
		}
		input.checkbox {
			background: #fff;
			border: 1px #000;
		}
	</style>
</head>

<body>
	<div id="flex">
		<input type="checkbox" id="checkbox"/>
	</div>
</body>
</rml>
)";

TEST_CASE("FlexFormatting")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_flex_rml);
	REQUIRE(document);
	document->Show();

	Element* checkbox = document->GetElementById("checkbox");
	Element* flex = document->GetElementById("flex");

	struct TestCase {
		String flex_direction;
		String align_items;
		Vector2f expected_size;
	};
	TestCase test_cases[] = {
		{"column", "stretch", Vector2f(248, 16)},
		{"column", "center", Vector2f(16, 16)},
		{"row", "stretch", Vector2f(16, 88)},
		{"row", "center", Vector2f(16, 16)},
	};

	for (auto& test_case : test_cases)
	{
		flex->SetProperty("flex-direction", test_case.flex_direction);
		flex->SetProperty("align-items", test_case.align_items);

		TestsShell::RenderLoop();

		CAPTURE(test_case.align_items);
		CAPTURE(test_case.flex_direction);
		CHECK(checkbox->GetBox().GetSize() == test_case.expected_size);
	}

	document->Close();

	TestsShell::ShutdownShell();
}
