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
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <doctest.h>
#include <float.h>

using namespace Rml;

static const String document_decorator_rml = R"(
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

		@decorator my-gradient : horizontal-gradient {
			start-color: #f0f;
			stop-color: #fff;
		}

		div {
			border: 20px transparent;
			padding: 30px;
			width: 50px;
			height: 50px;
		}

		#content_box {
			decorator: horizontal-gradient(#f00 #ff0) content-box;
		}
		#padding_box {
			decorator: horizontal-gradient(#f00 #ff0) padding-box;
		}
		#auto_box {
			decorator: horizontal-gradient(#f00 #ff0);
		}
		#border_box {
			decorator: horizontal-gradient(#f00 #ff0) border-box;
		}

		body.at_decorator #content_box {
			decorator: my-gradient content-box;
		}
		body.at_decorator #padding_box {
			decorator: my-gradient padding-box;
		}
		body.at_decorator #auto_box {
			decorator: my-gradient;
		}
		body.at_decorator #border_box {
			decorator: my-gradient border-box;
		}
	</style>
</head>

<body>
	<div id="content_box"/>
	<div id="padding_box"/>
	<div id="auto_box"/>
	<div id="border_box"/>
</body>
</rml>
)";

TEST_CASE("decorator.paint-area")
{
	TestsRenderInterface* render_interface = TestsShell::GetTestsRenderInterface();
	// This test only works with the dummy renderer.
	if (!render_interface)
		return;

	Context* context = TestsShell::GetContext();

	ElementDocument* document = context->LoadDocumentFromMemory(document_decorator_rml, "assets/");
	document->Show();

	for (const bool set_at_decorator_class : {false, true})
	{
		document->SetClass("at_decorator", set_at_decorator_class);
		const byte blue = (set_at_decorator_class ? 255 : 0);

		render_interface->ExpectCompileGeometry({
			Mesh{
				Vector<Vertex>{
					{{50, 50}, {255, 0, blue, 255}, {0, 0}},
					{{100, 50}, {255, 255, blue, 255}, {0, 0}},
					{{100, 100}, {255, 255, blue, 255}, {0, 0}},
					{{50, 100}, {255, 0, blue, 255}, {0, 0}},
				},
				Vector<int>{0, 2, 1, 0, 3, 2},
			},
			Mesh{
				Vector<Vertex>{
					{{20, 20}, {255, 0, blue, 255}, {0, 0}},
					{{130, 20}, {255, 255, blue, 255}, {0, 0}},
					{{130, 130}, {255, 255, blue, 255}, {0, 0}},
					{{20, 130}, {255, 0, blue, 255}, {0, 0}},
				},
				Vector<int>{0, 2, 1, 0, 3, 2},
			},
			Mesh{
				Vector<Vertex>{
					{{20, 20}, {255, 0, blue, 255}, {0, 0}},
					{{130, 20}, {255, 255, blue, 255}, {0, 0}},
					{{130, 130}, {255, 255, blue, 255}, {0, 0}},
					{{20, 130}, {255, 0, blue, 255}, {0, 0}},
				},
				Vector<int>{0, 2, 1, 0, 3, 2},
			},
			Mesh{
				Vector<Vertex>{
					{{0, 0}, {255, 0, blue, 255}, {0, 0}},
					{{150, 0}, {255, 255, blue, 255}, {0, 0}},
					{{150, 150}, {255, 255, blue, 255}, {0, 0}},
					{{0, 150}, {255, 0, blue, 255}, {0, 0}},
				},
				Vector<int>{0, 2, 1, 0, 3, 2},
			},
		});

		context->Update();
		context->Render();
	}

	document->Close();

	TestsShell::ShutdownShell();
}
