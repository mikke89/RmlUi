#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
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
