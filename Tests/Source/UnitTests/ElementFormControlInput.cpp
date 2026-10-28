#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <doctest.h>

using namespace Rml;

TEST_CASE("form.input.text.no_font")
{
	// Missing font style causes intrinsic width of 'input.text' to be set to zero pixels. Ensure we handle this gracefully without assertions.
	const String document_rml = R"(<rml>
	<head>
		<link type="text/rcss" href="/assets/rml.rcss" />
		<style>
			body {
				color: white;
				width: 300dp;
				height: 225dp;
			}
			input.text {
				border: 1px #000000;
				padding: 5dp 36dp 5dp 12dp;
				background: #fff;
				border-radius: 8dp;
			}
		</style>
	</head>

	<body>
		<input type="text"/>
	</body>
</rml>)";

	Context* context = TestsShell::GetContext();

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	document->Show();
	context->Update();
	context->Render();

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("form.input.password")
{
	const String document_rml = R"(<rml>
	<head>
		<link type="text/rcss" href="/assets/rml.rcss" />
		<style>
			body {
				font-family: LatoLatin;
				font-size: 16dp;
				color: white;
				width: 300dp;
				height: 225dp;
			}
		</style>
	</head>

	<body>
		<input type="password" id="password" value="aé€"/>
	</body>
</rml>)";

	Context* context = TestsShell::GetContext();

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	document->Show();
	context->Update();
	context->Render();

	auto input = rmlui_dynamic_cast<ElementFormControlInput*>(document->GetElementById("password"));
	REQUIRE(input);
	input->Focus();
	context->Update();

	// Each character of the value is displayed as one bullet, no matter how many bytes it takes up.
	String selected_text;
	input->SetSelectionRange(0, 3);
	input->GetSelection(nullptr, nullptr, &selected_text);
	CHECK(selected_text == "\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2");

	// Text is inserted at the cursor, between the characters of the value.
	input->SetSelectionRange(1, 1);
	context->ProcessTextInput("ü");
	CHECK(input->GetValue() == "aüé€");

	int selection_start = -1, selection_end = -1;
	input->GetSelection(&selection_start, &selection_end, nullptr);
	CHECK(selection_start == 2);
	CHECK(selection_end == 2);

	// A selection deletes the characters of the value it covers.
	input->SetSelectionRange(2, 3);
	context->ProcessKeyDown(Input::KI_BACK, 0);
	CHECK(input->GetValue() == "aü€");

	context->ProcessKeyDown(Input::KI_END, 0);
	context->ProcessKeyDown(Input::KI_BACK, 0);
	CHECK(input->GetValue() == "aü");

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("form.input.text.long_placeholder_mouse_click")
{
	const String document_rml = R"(<rml>
        <head>
                <link type="text/rcss" href="/assets/rml.rcss" />
                <style>
                        body {
                                width: 700dp;
                                height: 225dp;
                                margin: 0;
                        }
                        input[type="text"] {
                                font-family: LatoLatin;
                                display: block;
                                width: 600dp;
                                height: 40dp;
                                border: 1px #000000;
                                padding: 5dp;
                        }
                </style>
        </head>
        <body>
                <input type="text" placeholder="123456789-123456789-123456789-12345" />
        </body>
</rml>)";

	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	document->Show();
	context->Update();
	context->Render();

	context->ProcessMouseMove(500, 20, 0);
	context->Update();
	context->ProcessMouseButtonDown(0, 0);
	context->Update();
	context->ProcessMouseButtonUp(0, 0);

	document->Close();
	TestsShell::ShutdownShell();
}
