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

#include "../Common/TestsInterface.h"
#include "../Common/TestsShell.h"
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Elements/ElementFormControl.h>
#include <RmlUi/Core/TextInputHandler.h>
#include <doctest.h>

using namespace Rml;

static const String document_text_input_handler_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/assets/rml.rcss"/>
</head>

<body>
	<input type="text" id="text_input"/>
	<textarea id="text_area"></textarea>
</body>
</rml>
)";

TEST_CASE("Text input handler")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_text_input_handler_rml);
	REQUIRE(document);
	document->Show();

	Element* input = document->GetElementById("text_input");
	REQUIRE(input);

	Element* text_area = document->GetElementById("text_area");
	REQUIRE(text_area);

	SUBCASE("focus")
	{
		struct TestTextInputHandler : public Rml::TextInputHandler {
			virtual void OnFocus(ElementFormControl* element) override
			{
				CHECK_MESSAGE(focused_input_id.empty(), "Focused: ", focused_input_id);
				focused_input_id = element->GetId();
			}

			virtual void OnBlur(ElementFormControl* element) override
			{
				CHECK(focused_input_id == element->GetId());
				focused_input_id.clear();
			}

			String focused_input_id;
		};

		TestTextInputHandler test_text_input_handler;
		context->SetTextInputHandler(&test_text_input_handler);

		input->Focus();
		CHECK(test_text_input_handler.focused_input_id == "text_input");

		text_area->Focus();
		CHECK(test_text_input_handler.focused_input_id == "text_area");

		// Unfocus any active text input element by focusing the document.
		document->Focus();
		CHECK(test_text_input_handler.focused_input_id.empty());

		context->SetTextInputHandler(nullptr);
	}

	document->Close();
	TestsShell::ShutdownShell();
}
