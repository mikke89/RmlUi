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
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Types.h>

#include <doctest.h>
#include <nanobench.h>

using namespace ankerl;
using namespace Rml;

static const String rml_table_document = R"(
<rml>
<head>
    <title>Table</title>
    <link type="text/rcss" href="/../Tests/Data/style.rcss"/>
	<style>
		table {
			border-width: 20px 5px 0;
			color: #333;
			text-align: center;
		}
		table, table * {
			border-color: #666;
		}
		td {
			padding: 15px 5px;
			height: 47px;
		}
		col {
			background: #3d3;
		}
		col:first-child {
			width: 150px;
			background: #6df;
			border-right-width: 3px;
		}
		col:last-of-type {
			background: #dd3;
		}
		thead {
			color: black;
			background: #fff5;
			border-bottom: 3px #666;
		}
		tbody tr {
			border-bottom: 1px #666a;
		}
		tbody tr:last-child {
			border-bottom: 0;
		}
		tbody tr:hover {
			background: #fff5;
		}
		tfoot {
			background: #666;
			color: #ccc;
		}
		tfoot td {
			padding-top: 0px;
			padding-bottom: 0px;
			text-align: right;
			height: 20px;
		}
	</style>
</head>
<body>
</body>
</rml>
)";

static const String rml_table_element = R"(
<table>
	<col/>
	<col span="2"/>
	<col/>
	<thead>
		<tr>
			<td>A</td>
			<td colspan="2">B</td>
			<td>C</td>
		</tr>
	</thead>
	<tbody>
		<tr>
			<td>D</td>
			<td>E</td>
			<td>F</td>
			<td>G</td>
		</tr>
		<tr>
			<td>H</td>
			<td>I</td>
			<td>J</td>
			<td>K</td>
		</tr>
	</tbody>
	<tfoot>
		<tr>
			<td colspan="4">[1] Footnote</td>
		</tr>
	</tfoot>
</table>
)";


static const String rml_inlineblock_document = R"(
<rml>
<head>
    <title>Table inline-block</title>
    <link type="text/rcss" href="/../Tests/Data/style.rcss"/>
	<style>
		table {
			display: block;
			border-width: 20px 5px 0;
			color: #333;
			text-align: center;
		}
		body * {
			border-color: #666;
		}
		td {
			display: inline-block;
			box-sizing: border-box;
			padding: 15px 5px;
			width: 25%;
			height: 47px;
			background: #3d3;
		}
		td.span2 { width: 50%; }
		td.span4 { width: 100%; }
		tr {
			display: block;
		}
		td:first-child {
			background: #6df;
			border-right-width: 3px;
		}
		td:last-of-type {
			background: #dd3;
		}
		thead {
			display: block;
			color: black;
			background: #fff5;
			border-bottom: 3px #666;
		}
		tbody {
			display: block;
		}
		tbody tr {
			border-bottom: 1px #666a;
		}
		tbody tr:last-child {
			border-bottom: 0;
		}
		tbody tr:hover {
			background: #fff5;
		}
		tfoot {
			display: block;
			background: #666;
			color: #ccc;
		}
		tfoot td {
			padding-top: 0px;
			padding-bottom: 0px;
			text-align: right;
			height: 20px;
		}
	</style>
</head>

<body>
</body>
</rml>
)";

static const String rml_inline_block_element = R"(
<table>
	<thead>
		<tr>
			<td>A</td>
			<td class="span2">B</td>
			<td>C</td>
		</tr>
	</thead>
	<tbody>
		<tr>
			<td>D</td>
			<td>E</td>
			<td>F</td>
			<td>G</td>
		</tr>
		<tr>
			<td>H</td>
			<td>I</td>
			<td>J</td>
			<td>K</td>
		</tr>
	</tbody>
	<tfoot>
		<tr>
			<td style="background: transparent" class="span4">[1] Footnote</td>
		</tr>
	</tfoot>
</table>
)";


TEST_CASE("table_basic")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(rml_table_document);
	REQUIRE(document);
	document->Show();

	nanobench::Bench bench;
	bench.title("Table basic");
	bench.relative(true);

	document->SetInnerRML(rml_table_element);
	context->Update();
	context->Render();

	TestsShell::RenderLoop();
	const String msg = TestsShell::GetRenderStats();
	MESSAGE(msg);

	bench.run("Update (unmodified)", [&] {
		context->Update();
	});

	bench.run("Render", [&] {
		context->Render();
	});

	bench.run("SetInnerRML", [&] {
		document->SetInnerRML(rml_table_element);
	});

	bench.run("SetInnerRML + Update", [&] {
		document->SetInnerRML(rml_table_element);
		context->Update();
	});

	bench.run("SetInnerRML + Update + Render", [&] {
		document->SetInnerRML(rml_table_element);
		context->Update();
		context->Render();
	});

	document->Close();
}


TEST_CASE("table_inline-block")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(rml_inlineblock_document);
	REQUIRE(document);
	document->Show();

	nanobench::Bench bench;
	bench.title("Table inline-block");
	bench.relative(true);

	document->SetInnerRML(rml_inline_block_element);
	context->Update();
	context->Render();

	TestsShell::RenderLoop();
	const String msg = TestsShell::GetRenderStats();
	MESSAGE(msg);

	bench.run("Update (unmodified)", [&] {
		context->Update();
	});

	bench.run("Render", [&] {
		context->Render();
	});

	bench.run("SetInnerRML", [&] {
		document->SetInnerRML(rml_inline_block_element);
	});

	bench.run("SetInnerRML + Update", [&] {
		document->SetInnerRML(rml_inline_block_element);
		context->Update();
	});

	bench.run("SetInnerRML + Update + Render", [&] {
		document->SetInnerRML(rml_inline_block_element);
		context->Update();
		context->Render();
	});

	document->Close();
}
