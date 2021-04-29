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
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Elements/ElementFormControlSelect.h>
#include <doctest.h>

using namespace Rml;

static const String basic_doc_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<link type="text/rcss" href="/assets/invader.rcss"/>
</head>
<body/>
</rml>
)";

static const String data_model_doc_rml_pre = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/template" href="/assets/window.rml"/>
	<style>
		body.window
		{
			top: 100px;
			left: 200px;
			width: 600px;
			height: 450px;
		}
	</style>
</head>
<body template="window">
<div data-model="select-test" id="select_wrap">
)";
static const String data_model_doc_rml_post = R"(
</div>
</body>
</rml>
)";


static String GetSelectValueRml(ElementFormControlSelect* select_element)
{
	for (int child_index = 0; child_index < select_element->GetNumChildren(true); child_index++)
	{
		Element* child = select_element->GetChild(child_index);
		if (child->GetTagName() == "selectvalue")
			return child->GetInnerRML();
	}

	return String();
}


TEST_CASE("form.select.value")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	struct Test {
		const String rml;
		const String expected_value;
		const String expected_value_rml;
	};

	Test tests[] = {
		{
			R"(
			<select>
				<option>A</option>
				<option>B</option>
				<option>C</option>
				<option>D</option>
			</select>
			)", "", "A"
		},

		{
			R"(
			<select>
				<option value="a">A</option>
				<option>B</option>
				<option>C</option>
				<option>D</option>
			</select>
			)", "a", "A"
		},

		{
			R"(
			<select value="z">
				<option>A</option>
				<option>B</option>
				<option>C</option>
				<option>D</option>
			</select>
			)", "", "A"
		},

		{
			R"(
			<select value="c">
				<option value="a">A</option>
				<option value="b">B</option>
				<option value="c">C</option>
				<option value="d">D</option>
			</select>
			)", "c", "C"
		},

		{
			R"(
			<select value="c">
				<option value="a">A</option>
				<option value="b">B</option>
				<option value="c">C</option>
				<option value="d" selected>D</option>
			</select>
			)", "d", "D"
		},

		{
			R"(
			<select value="c">
				<option value="a" selected>A</option>
				<option value="b">B</option>
				<option value="c">C</option>
				<option value="d">D</option>
			</select>
			)", "a", "A"
		},

		{
			R"(
			<select value="c">
				<option value="a" selected>A</option>
				<option value="b">B</option>
				<option value="c">C</option>
				<option value="d" selected>D</option>
			</select>
			)", "d", "D"
		},

		{
			R"(
			<select>
				<option value="a">A</option>
				<option value="b">B</option>
				<option value="c" selected>C</option>
				<option value="d">D</option>
			</select>
			)", "c", "C"
		},
	};

	ElementDocument* document = context->LoadDocumentFromMemory(basic_doc_rml);
	REQUIRE(document);

	document->Show();
	context->Update();
	context->Render();

	int i = 0;
	for (const Test& test : tests)
	{
		document->SetInnerRML(test.rml);
		context->Update();

		REQUIRE(document->GetNumChildren() == 1);

		ElementFormControlSelect* select_element = rmlui_dynamic_cast<ElementFormControlSelect*>(document->GetFirstChild());
		REQUIRE(select_element);

		const String value = select_element->GetValue();
		const String selectvalue_rml = GetSelectValueRml(select_element);

		CHECK_MESSAGE(value == test.expected_value, "Document " << i);
		CHECK_MESSAGE(selectvalue_rml == test.expected_value_rml, "Document " << i);

		i++;
	}

	document->Close();
	TestsShell::ShutdownShell();
}


TEST_CASE("form.select.databinding")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	struct Test {
		const String rml;
		const String expected_value;
		const String expected_value_rml;
	};

	int selected_index = 2;
	String selected_value = "b";
	Vector<String> values = { "a", "b", "c", "d" };

	Test tests[] = {
		{
			R"(
			<select data-value="selected_index">
				<option value="0">A</option>
				<option value="1">B</option>
				<option value="2">C</option>
				<option value="3">D</option>
			</select>
			)", "2", "C"
		},

		{
			R"(
			<select data-value="selected_value">
				<option value="a">A</option>
				<option value="b">B</option>
				<option value="c">C</option>
				<option value="d">D</option>
			</select>
			)", "b", "B"
		},

		{
			R"(
			<select>
				<option value="a" data-attrif-selected="selected_value == 'a'">A</option>
				<option value="b" data-attrif-selected="selected_value == 'b'">B</option>
				<option value="c" data-attrif-selected="selected_value == 'c'">C</option>
				<option value="d" data-attrif-selected="selected_value == 'd'">D</option>
			</select>
			)", "b", "B"
		},

		{
			R"(
			<select>
				 <option data-for="s : subjects" data-attr-value="s" data-attrif-selected="s == selected_value">{{ s | to_upper }}</option>
			</select>
			)", "b", "B"
		},

		{
			R"(
			<select>
				 <option data-for="s : subjects" data-attr-value="it_index" data-attrif-selected="it_index == selected_index">{{ s | to_upper }}</option>
			</select>
			)", "2", "C"
		},

		{
			R"(
			<select data-value="selected_value">
				 <option data-for="s : subjects" data-attr-value="s">{{ s | to_upper }}</option>
			</select>
			)", "b", "B"
		},

		{
			R"(
			<select data-value="selected_index">
				 <option data-for="s : subjects" data-attr-value="it_index">{{ s | to_upper }}</option>
			</select>
			)", "2", "C"
		},
	};

	DataModelConstructor constructor = context->CreateDataModel("select-test");
	constructor.RegisterArray<Vector<String>>();

	constructor.Bind("selected_index", &selected_index);
	constructor.Bind("selected_value", &selected_value);
	constructor.Bind("subjects", &values);

	int i = 0;
	for (const Test& test : tests)
	{
		const String document_rml = data_model_doc_rml_pre + test.rml + data_model_doc_rml_post;

		ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
		REQUIRE(document);

		document->Show();

		TestsShell::RenderLoop();

		Element* wrapper_element = document->GetElementById("select_wrap");
		REQUIRE(wrapper_element);
		REQUIRE(wrapper_element->GetNumChildren() == 1);

		ElementFormControlSelect* select_element = rmlui_dynamic_cast<ElementFormControlSelect*>(wrapper_element->GetFirstChild());
		REQUIRE(select_element);

		const String value = select_element->GetValue();
		const String selectvalue_rml = GetSelectValueRml(select_element);

		CHECK_MESSAGE(value == test.expected_value, "Document " << i);
		CHECK_MESSAGE(selectvalue_rml == test.expected_value_rml, "Document " << i);

		i++;
		document->Close();
	}

	TestsShell::ShutdownShell();
}


TEST_CASE("form.select.data-for")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	struct Test {
		const String rml;
		const String expected_value;
		const String expected_value_rml;
	};

	String selected_value = "b";
	Vector<String> values = { "a", "b", "c", "d" };

	DataModelConstructor constructor = context->CreateDataModel("select-test");
	constructor.RegisterArray<Vector<String>>();

	constructor.Bind("selected_value", &selected_value);
	constructor.Bind("subjects", &values);

	DataModelHandle handle = constructor.GetModelHandle();

	{

		const String select_rml = R"(
			<select data-value="selected_value">
				 <option data-for="s : subjects" data-attr-value="s">{{ s | to_upper }}</option>
			</select>
		)";

		const String document_rml = data_model_doc_rml_pre + select_rml + data_model_doc_rml_post;

		ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
		REQUIRE(document);

		document->Show();

		Element* wrapper_element = document->GetElementById("select_wrap");
		REQUIRE(wrapper_element);
		REQUIRE(wrapper_element->GetNumChildren() == 1);

		ElementFormControlSelect* select_element = rmlui_dynamic_cast<ElementFormControlSelect*>(wrapper_element->GetFirstChild());
		REQUIRE(select_element);

		{
			//TestsShell::RenderLoop();

			const String value = select_element->GetValue();
			const int selected_index = select_element->GetSelection();
			const String selectvalue_rml = GetSelectValueRml(select_element);

			CHECK(value == "b");
			CHECK(selected_index == 1);
			CHECK(selectvalue_rml == "B");
		}

		{
			selected_value = "d";
			handle.DirtyVariable("selected_value");
			context->Update();
			//TestsShell::RenderLoop();

			const String value = select_element->GetValue();
			const int selected_index = select_element->GetSelection();
			const String selectvalue_rml = GetSelectValueRml(select_element);

			CHECK(value == "d");
			CHECK(selected_index == 3);
			CHECK(selectvalue_rml == "D");
		}

		{
			select_element->SetValue("a");
			context->Update();
			TestsShell::RenderLoop();

			const String value = select_element->GetValue();
			const int selected_index = select_element->GetSelection();
			const String selectvalue_rml = GetSelectValueRml(select_element);

			CHECK(selected_value == "a");
			CHECK(value == "a");
			CHECK(selected_index == 0);
			CHECK(selectvalue_rml == "A");
		}

		{
			select_element->SetSelection(1);
			context->Update();
			TestsShell::RenderLoop();

			const String value = select_element->GetValue();
			const int selected_index = select_element->GetSelection();
			const String selectvalue_rml = GetSelectValueRml(select_element);

			CHECK(selected_value == "b");
			CHECK(value == "b");
			CHECK(selected_index == 1);
			CHECK(selectvalue_rml == "B");
		}

#if 0
		// These are not supported now, we may want to add support later.

		{
			// Values: a c b d
			std::swap(values[1], values[2]);
			handle.DirtyVariable("subjects");
			context->Update();
			TestsShell::RenderLoop();

			const String value = select_element->GetValue();
			const int selected_index = select_element->GetSelection();
			const String selectvalue_rml = GetSelectValueRml(select_element);

			CHECK(value == "b");
			CHECK(selected_index == 2);
			CHECK(selectvalue_rml == "B");
		}

		{
			// Values: c b d
			values.erase(values.begin());
			handle.DirtyVariable("subjects");
			context->Update();
			TestsShell::RenderLoop();

			const String value = select_element->GetValue();
			const int selected_index = select_element->GetSelection();
			const String selectvalue_rml = GetSelectValueRml(select_element);

			CHECK(value == "b");
			CHECK(selected_index == 1);
			CHECK(selectvalue_rml == "B");
		}
#endif

		document->Close();
	}

	TestsShell::ShutdownShell();
}
