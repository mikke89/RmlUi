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
#include <RmlUi/Core/Profiling.h>
#include <RmlUi/Core/Types.h>
#include <doctest.h>
#include <nanobench.h>

using namespace ankerl;
using namespace Rml;

static const String rml_flexbox_basic_document = R"(
<rml>
<head>
    <title>Flex 01 - Basic flexbox (slow, content-based sizing)</title>
    <link type="text/rcss" href="/../Tests/Data/style.rcss"/>
	<style>
		header, article { display: block; }
		h1 { font-size: 1.5em; }
		h2 { font-size: 1.3em; }
		
		header {
			background-color: #9777d9;
			border: 5dp #666;
		}
		h1 {
			text-align: center;
			color: white;
			line-height: 100dp;
		}
		section {
			display: flex;
			background: #666;
			border: 5dp #666;
		}
		article {
			padding: 10dp;
			margin: 0 5dp;
			background-color: #edd3c0;
		}
		h2 {
			text-align: center;
			background-color: #eb6e14;
			margin: -10dp -10dp 0;
			padding: 10dp 0; 
		}
	</style>
</head>
<body>
</body>
</rml>
)";

static const String rml_flexbox_basic_document_fast = R"(
<rml>
<head>
    <title>Flex 01 - Basic flexbox (fast, not content based)</title>
    <link type="text/rcss" href="/../Tests/Data/style.rcss"/>
	<style>
		header, article { display: block; }
		h1 { font-size: 1.5em; }
		h2 { font-size: 1.3em; }
		
		header {
			background-color: #9777d9;
			border: 5dp #666;
		}
		h1 {
			text-align: center;
			color: white;
			line-height: 100dp;
		}
		section {
			display: flex;
			background: #666;
			border: 5dp #666;
			height: 650px;
		}
		article {
			padding: 10dp;
			margin: 0 5dp;
			background-color: #edd3c0;
			flex: 1;
			box-sizing: border-box;
			height: 100%;
		}
		h2 {
			text-align: center;
			background-color: #eb6e14;
			margin: -10dp -10dp 0;
			padding: 10dp 0; 
		}
	</style>
</head>
<body>
</body>
</rml>
)";

static const String rml_flexbox_basic_document_float_reference = R"(
<rml>
<head>
    <title>Flex 01 - Basic flexbox (float comparison)</title>
    <link type="text/rcss" href="/../Tests/Data/style.rcss"/>
	<style>
		header, article { display: block; }
		h1 { font-size: 1.5em; }
		h2 { font-size: 1.3em; }
		
		header {
			background-color: #9777d9;
			border: 5dp #666;
		}
		h1 {
			text-align: center;
			color: white;
			line-height: 100dp;
		}
		section {
			display: block;
			background: #666;
			border: 5dp #666;
		}
		article {
			padding: 10dp;
			margin: 0 5dp;
			background-color: #edd3c0;
			float: left;
			width: 30%;
			box-sizing: border-box;
		}
		h2 {
			text-align: center;
			background-color: #eb6e14;
			margin: -10dp -10dp 0;
			padding: 10dp 0; 
		}
	</style>
</head>
<body>
</body>
</rml>
)";

static const String rml_flexbox_basic_body = R"(
<header>
	<h1>Header</h1>
</header>
<section>
	<article>
		<h2>First article</h2>
		<p>Etiam libero lorem, lacinia non augue lobortis, tincidunt consequat justo. Sed id enim tempor, feugiat tortor id, rhoncus enim. Quisque pretium neque eu felis tincidunt fringilla. Mauris laoreet enim neque, iaculis cursus lorem mollis sed. Nulla pretium euismod nulla sed convallis. Curabitur in tempus sem. Phasellus suscipit vitae nulla nec ultricies.</p>
	</article>
	<article>
		<h2>Second article</h2>
		<p>Ut volutpat, odio et facilisis molestie, lacus elit euismod enim, et tempor lacus sapien finibus ipsum. Aliquam erat volutpat. Nullam risus turpis, hendrerit ac fermentum in, dapibus non risus.</p>
	</article>
	<article>
		<h2>Third article</h2>
		<p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed aliquet commodo nisi, id cursus enim eleifend vitae. Praesent turpis lorem, commodo id tempus sit amet, faucibus et libero. Aliquam malesuada ultrices leo, ut molestie tortor posuere sit amet. Proin vitae tortor a sem consequat gravida. Maecenas sed egestas dolor.</p>
		<p>In gravida ligula in turpis molestie varius. Ut sed velit id tellus aliquet aliquet. Nulla et leo tellus. Ut a convallis dolor, eu rutrum enim. Nam vitae ultrices dui. Aliquam semper eros ut ultrices rutrum.</p>
	</article>
</section>
)";

static const String rml_flexbox_mixed_document = R"(
<rml>
<head>
    <title>Flex 02 - Various features</title>
    <link type="text/rcss" href="/../Tests/Data/style.rcss"/>
	<style>
        .flex-container {
            display: flex;
            margin: 10px 20px;
            background-color: #333;
            max-height: 210px;
            flex-wrap: wrap-reverse;
        }

        .flex-item {
            width: 50px;
            margin: 20px;
            background-color: #eee;
            height: 50px;
            text-align: center;
        }

        .flex-direction-row {
            flex-direction: row;
        }
        .flex-direction-row-reverse {
            flex-direction: row-reverse;
        }
        .flex-direction-column {
            flex-direction: column;
        }
        .flex-direction-column-reverse {
            flex-direction: column-reverse;
        }
        .absolute {
            margin: 0;
            position: absolute;
            right: 0;
            bottom: 10px;
        }
	</style>
</head>

<body>
</body>
</rml>
)";

static const String rml_flexbox_mixed_body = R"(
<div class="flex-container flex-direction-row" style="position: relative">
    <div class="flex-item absolute">Abs</div>
    <div class="flex-item" style="margin: 50px;">1</div>
    <div class="flex-item" style="margin-top: auto">2</div>
    <div class="flex-item" style="margin: auto">3</div>
</div>
<div class="flex-container flex-direction-row-reverse" style="height: 200px; justify-content: space-around;">
    <div class="flex-item">1</div>
    <div class="flex-item" style="margin-bottom: auto;">2</div>
    <div class="flex-item" style="margin-right: 40px;">3</div>
</div>
<div class="flex-container flex-direction-column">
    <div class="flex-item" id="test" style="margin-right: auto">1</div>
    <div class="flex-item">2</div>
    <div class="flex-item">3</div>
</div>
<div class="flex-container flex-direction-column-reverse">
    <div class="flex-item">1</div>
    <div class="flex-item">2 LONG_OVERFLOWING_WORD</div>
    <div class="flex-item">3</div>
</div>
)";

static const String rml_flexbox_scroll_document = R"(
<rml>
<head>
    <title>Flex 03 - Scrolling container</title>
    <link type="text/rcss" href="/../Tests/Data/style.rcss"/>
	<style>
		.flex {
			display: flex;
			background-color: #555;
			margin: 5dp 20dp 15dp;
			border: 2dp #333;
			justify-content: space-between;
			color: #d44fff;
		}
		.auto {
			overflow: auto;
		}
		.scroll {
			overflow: scroll;
		}
		.flex div {
			flex: 0 1 auto;
			width: 50dp;
			height: 50dp;
			margin: 20dp;
			background-color: #eee;
			line-height: 50dp;
			text-align: center;
		}
		.flex div.tall {
			height: 80dp;
			width: 15dp;
			margin: 0;
			border: 2dp #d44fff;
		}
	</style>
</head>
<body>
</body>
</rml>
)";

static const String rml_flexbox_scroll_body = R"(
overflow: scroll
<div class="flex scroll" id="scroll">
	<div>Hello<div class="tall"/></div>
	<div>big world!</div>
	<div>LOOOOOOOOOOOOOOOOOOOOONG</div>
</div>
overflow: auto
<div class="flex auto" id="auto">
	<div>Hello<div class="tall"/></div>
	<div>big world!</div>
	<div>LOOOOOOOOOOOOOOOOOOOOONG</div>
</div>
overflow: auto - only vertical overflow
<div class="flex auto" id="vertical">
	<div>Hello<div class="tall"/></div>
	<div>big world!</div>
	<div>LONG</div>
</div>
overflow: auto - only horizontal overflow
<div class="flex auto" id="horizontal">
	<div>Hello</div>
	<div>big</div>
	<div>LOOOOOOOOOOOOOOOOOOOOONG</div>
</div>
overflow: visible
<div class="flex" id="visible">
	<div>Hello<div class="tall"/></div>
	<div>big world!</div>
	<div>LOOOOOOOOOOOOOOOOOOOOONG</div>
</div>
)";

TEST_CASE("flexbox")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	{
		nanobench::Bench bench;
		bench.title("Flexbox basic layout");
		bench.relative(true);

		// Construct the flexbox layout document.
		ElementDocument* document = context->LoadDocumentFromMemory(rml_flexbox_basic_document);
		REQUIRE(document);
		document->Show();

		document->SetInnerRML(rml_flexbox_basic_body);
		context->Update();
		context->Render();

		TestsShell::RenderLoop();

		// Compare to an almost equivalent fast flexbox layout where we try to eliminate any content-based sizing. Uses the same body rml.
		ElementDocument* document_fast = context->LoadDocumentFromMemory(rml_flexbox_basic_document_fast);
		REQUIRE(document_fast);
		document_fast->Show();

		document_fast->SetInnerRML(rml_flexbox_basic_body);
		context->Update();
		context->Render();

		TestsShell::RenderLoop();

		// Finally, add a reference document based on layout with float boxes instead of flexbox. Uses the same body rml.
		ElementDocument* document_float_reference = context->LoadDocumentFromMemory(rml_flexbox_basic_document_float_reference);
		REQUIRE(document_float_reference);
		document_float_reference->Show();

		document_float_reference->SetInnerRML(rml_flexbox_basic_body);
		context->Update();
		context->Render();

		TestsShell::RenderLoop();

		bench.run("Update (unmodified)", [&] { context->Update(); });

		bench.run("Render", [&] { context->Render(); });

		bench.run("SetInnerRML", [&] { document->SetInnerRML(rml_flexbox_scroll_body); });

		bench.run("SetInnerRML + Update (float reference)", [&] {
			document_float_reference->SetInnerRML(rml_flexbox_basic_body);
			context->Update();
		});
		bench.run("SetInnerRML + Update (fast version)", [&] {
			document_fast->SetInnerRML(rml_flexbox_basic_body);
			context->Update();
		});
		bench.run("SetInnerRML + Update", [&] {
			document->SetInnerRML(rml_flexbox_basic_body);
			context->Update();
		});

		bench.run("SetInnerRML + Update + Render (float reference)", [&] {
			document_float_reference->SetInnerRML(rml_flexbox_basic_body);
			context->Update();
			context->Render();
		});
		bench.run("SetInnerRML + Update + Render (fast version)", [&] {
			document_fast->SetInnerRML(rml_flexbox_basic_body);
			context->Update();
			context->Render();
		});
		bench.run("SetInnerRML + Update + Render", [&] {
			document->SetInnerRML(rml_flexbox_basic_body);
			context->Update();
			context->Render();
		});

		document->Close();
		document_fast->Close();
		document_float_reference->Close();
	}

	{
		nanobench::Bench bench;
		bench.title("Flexbox mixed");
		bench.relative(true);

		ElementDocument* document = context->LoadDocumentFromMemory(rml_flexbox_mixed_document);
		REQUIRE(document);
		document->Show();

		document->SetInnerRML(rml_flexbox_mixed_body);
		context->Update();
		context->Render();

		TestsShell::RenderLoop();

		bench.run("Update (unmodified)", [&] { context->Update(); });

		bench.run("Render", [&] { context->Render(); });

		bench.run("SetInnerRML", [&] { document->SetInnerRML(rml_flexbox_scroll_body); });

		bench.run("SetInnerRML + Update", [&] {
			document->SetInnerRML(rml_flexbox_mixed_body);
			context->Update();
		});

		bench.run("SetInnerRML + Update + Render", [&] {
			document->SetInnerRML(rml_flexbox_mixed_body);
			context->Update();
			context->Render();
		});

		document->Close();
	}

	{
		nanobench::Bench bench;
		bench.title("Flexbox scroll");
		bench.relative(true);

		ElementDocument* document = context->LoadDocumentFromMemory(rml_flexbox_scroll_document);
		REQUIRE(document);
		document->Show();

		document->SetInnerRML(rml_flexbox_scroll_body);
		context->Update();
		context->Render();

		TestsShell::RenderLoop();

		bench.run("Update (unmodified)", [&] { context->Update(); });

		bench.run("Render", [&] { context->Render(); });

		bench.run("SetInnerRML", [&] { document->SetInnerRML(rml_flexbox_scroll_body); });

		bench.run("SetInnerRML + Update", [&] {
			document->SetInnerRML(rml_flexbox_scroll_body);
			context->Update();
		});

		bench.run("SetInnerRML + Update + Render", [&] {
			document->SetInnerRML(rml_flexbox_scroll_body);
			context->Update();
			context->Render();
		});

		document->Close();
	}
}

static const String rml_flexbox_chatbox = R"(
<rml>
<head>
	<title>Chat</title>
    <link type="text/rcss" href="/../Tests/Data/style.rcss"/>
	<style>
		body {
			font-size: 16px;
			overflow: auto;
			height: 300px;
			width: 300px;
		}
		#chat {
			display: flex;
			flex-direction: column;
			border: 2px #caa;
		}
		#chat > div {
			word-break: break-word;
			border: 2px #aac;
		}
	</style>
</head>

<body>
	<div id="chat"/>
</body>
</rml>
)";

TEST_CASE("flexbox.chat")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	nanobench::Bench bench;
	bench.title("Flexbox chat");
	bench.relative(true);
	// bench.epochs(100);

	auto MakeFlexItemsRml = [](int number_items, const String& item_text) {
		String rml;
		for (int i = 0; i < number_items; i++)
			rml += "<div>" + item_text + "</div>";
		return rml;
	};

	const String short_words =
		MakeFlexItemsRml(10, "aaaaaaaaaaaaaaa aaaaaaaaaaaaaa aaaaaaaaa aaaaaaaaaaaa aaaaaaaaaaaaa aaaaaaaaaaaaaaaaaaa aaaaaaaaaaaaaaaaaa");
	const String long_words =
		MakeFlexItemsRml(10, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");

	// The flex items will essentially be formatted four times each:
	//   - Two times during flex formatting, first to get their height, then to do their actual formatting.
	//   - Then flex formatting is itself done twice, since the body adds a scrollbar, thereby modifying the available width.
	// The long words take longer to format, since we do a naive approach to breaking up words in ElementText::GenerateLine, making us calculate
	// string widths repeatedly. Removing the 'word-break' property should make the long word-case much faster.
	ElementDocument* document = context->LoadDocumentFromMemory(rml_flexbox_chatbox);
	Element* chat = document->GetElementById("chat");
	chat->SetInnerRML(short_words + long_words);
	document->Show();
	TestsShell::RenderLoop();

	bench.run("Short words", [&] {
		chat->SetInnerRML(short_words);
		context->Update();
		context->Render();
		RMLUI_FrameMark;
	});
	bench.run("Long words", [&] {
		chat->SetInnerRML(long_words);
		context->Update();
		context->Render();
		RMLUI_FrameMark;
	});

	document->Close();
}

static const String rml_flexbox_shrink_to_fit = R"(
<rml>
<head>
    <title>Flex - Shrink-to-fit 01</title>
    <link type="text/rcss" href="/../Tests/Data/style.rcss"/>
	<style>
		body { width: 1000px; }
		.shrink-to-fit {
			float: left;
			clear: both;
			margin: 10px 0;
			border: 2px #e8e8e8;
		}
		.outer {
			border: 1px red;
			padding: 30px;
		}
		#basic .outer {
			display: flex;
		}
		#nested .outer {
			display: inline-flex;
		}
		.inner {
			border: 1px blue;
			padding: 30px;
		}
	</style>
</head>
<body>
<div id="basic" class="shrink-to-fit">
	Before
	<div class="outer">
		<div class="inner">Flex</div>
	</div>
	After
</div>
<div id="nested" class="shrink-to-fit">
	Before
	<div class="outer">
		<div class="inner">
			<div class="outer">
				<div class="inner">
					<div class="outer">
						<div class="inner">Flex</div>
					</div>
				</div>
			</div>
		</div>
	</div>
	After
</div>
</body>
</rml>
)";

TEST_CASE("flexbox.shrink-to-fit")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	nanobench::Bench bench;
	bench.title("Flexbox shrink-to-fit");
	bench.relative(true);

	ElementDocument* document = context->LoadDocumentFromMemory(rml_flexbox_shrink_to_fit);
	Element* basic = document->GetElementById("basic");
	Element* nested = document->GetElementById("nested");

	document->Show();
	TestsShell::RenderLoop();

	basic->SetProperty(PropertyId::Display, Style::Display::None);
	nested->SetProperty(PropertyId::Display, Style::Display::None);

	bench.run("Reference", [&] {
		document->SetProperty(PropertyId::Display, Style::Display::None);
		document->RemoveProperty(PropertyId::Display);
		context->Update();
		context->Render();
	});
	bench.run("Basic shrink-to-fit", [&] {
		basic->RemoveProperty(PropertyId::Display);
		nested->SetProperty(PropertyId::Display, Style::Display::None);
		context->Update();
		context->Render();
	});
	bench.run("Nested shrink-to-fit", [&] {
		basic->SetProperty(PropertyId::Display, Style::Display::None);
		nested->RemoveProperty(PropertyId::Display);
		context->Update();
		context->Render();
	});

	document->Close();
}
