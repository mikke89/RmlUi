#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Elements/ElementFormControlSelect.h>
#include <RmlUi/Core/EventListener.h>
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
			)",
			"", "A"},

		{
			R"(
			<select>
				<option value="a">A</option>
				<option>B</option>
				<option>C</option>
				<option>D</option>
			</select>
			)",
			"a", "A"},

		{
			R"(
			<select value="z">
				<option>A</option>
				<option>B</option>
				<option>C</option>
				<option>D</option>
			</select>
			)",
			"", "A"},

		{
			R"(
			<select value="c">
				<option value="a">A</option>
				<option value="b">B</option>
				<option value="c">C</option>
				<option value="d">D</option>
			</select>
			)",
			"c", "C"},

		{
			R"(
			<select value="c">
				<option value="a">A</option>
				<option value="b">B</option>
				<option value="c">C</option>
				<option value="d" selected>D</option>
			</select>
			)",
			"d", "D"},

		{
			R"(
			<select value="c">
				<option value="a" selected>A</option>
				<option value="b">B</option>
				<option value="c">C</option>
				<option value="d">D</option>
			</select>
			)",
			"a", "A"},

		{
			R"(
			<select value="c">
				<option value="a" selected>A</option>
				<option value="b">B</option>
				<option value="c">C</option>
				<option value="d" selected>D</option>
			</select>
			)",
			"d", "D"},

		{
			R"(
			<select>
				<option value="a">A</option>
				<option value="b">B</option>
				<option value="c" selected>C</option>
				<option value="d">D</option>
			</select>
			)",
			"c", "C"},
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

TEST_CASE("form.select.data_binding")
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
	Vector<String> values = {"a", "b", "c", "d"};

	Test tests[] = {
		{
			R"(
			<select data-value="selected_index">
				<option value="0">A</option>
				<option value="1">B</option>
				<option value="2">C</option>
				<option value="3">D</option>
			</select>
			)",
			"2", "C"},

		{
			R"(
			<select data-value="selected_value">
				<option value="a">A</option>
				<option value="b">B</option>
				<option value="c">C</option>
				<option value="d">D</option>
			</select>
			)",
			"b", "B"},

		{
			R"(
			<select>
				<option value="a" data-attrif-selected="selected_value == 'a'">A</option>
				<option value="b" data-attrif-selected="selected_value == 'b'">B</option>
				<option value="c" data-attrif-selected="selected_value == 'c'">C</option>
				<option value="d" data-attrif-selected="selected_value == 'd'">D</option>
			</select>
			)",
			"b", "B"},

		{
			R"(
			<select>
				 <option data-for="s : subjects" data-attr-value="s" data-attrif-selected="s == selected_value">{{ s | to_upper }}</option>
			</select>
			)",
			"b", "B"},

		{
			R"(
			<select>
				 <option data-for="s : subjects" data-attr-value="it_index" data-attrif-selected="it_index == selected_index">{{ s | to_upper }}</option>
			</select>
			)",
			"2", "C"},

		{
			R"(
			<select data-value="selected_value">
				 <option data-for="s : subjects" data-attr-value="s">{{ s | to_upper }}</option>
			</select>
			)",
			"b", "B"},

		{
			R"(
			<select data-value="selected_index">
				 <option data-for="s : subjects" data-attr-value="it_index">{{ s | to_upper }}</option>
			</select>
			)",
			"2", "C"},

		{
			R"(
			<select data-value="selected_index">
				 <option data-for="s : subjects" data-attr-value="it_index"><p data-rml="s | to_upper"></p></option>
			</select>
			)",
			"2", "<p data-rml=\"s | to_upper\">C</p>"},
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
	Vector<String> values = {"a", "b", "c", "d"};

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
			// TestsShell::RenderLoop();

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
			// TestsShell::RenderLoop();

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

static const String event_doc_rml = R"(
<rml>
<head>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<link type="text/rcss" href="/assets/invader.rcss"/>
</head>
<body>
    <select id="sel">
        <option selected>Cube</option>
        <option>Cone</option>
        <option value="c">Cylinder</option>
        <option value="d">Sphere</option>
    </select>
</body>
</rml>
)";

static void ClickAt(Context* context, int x, int y)
{
	context->Update();
	context->Render();
	context->ProcessMouseMove(x, y, 0);
	context->Update();
	context->Render();
	context->ProcessMouseButtonDown(0, 0);
	context->ProcessMouseButtonUp(0, 0);
	context->Update();
	context->Render();
}

TEST_CASE("form.select.event.change")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(event_doc_rml);
	REQUIRE(document);

	document->Show();

	struct SelectionEventListener : public Rml::EventListener {
		void ProcessEvent(Rml::Event& ev) override
		{
			num_events_processed += 1;
			value = ev.GetParameter<Rml::String>("value", "*empty*");
		}
		int num_events_processed = 0;
		String value;
	};
	auto listener = MakeUnique<SelectionEventListener>();
	document->GetElementById("sel")->AddEventListener(Rml::EventId::Change, listener.get());

	ClickAt(context, 100, 20); // Open select
	ClickAt(context, 100, 40); // Click option 'Cube'
	CHECK(listener->num_events_processed == 0);

	ClickAt(context, 100, 20); // Open select
	ClickAt(context, 100, 63); // Click option 'Cone'
	CHECK(listener->num_events_processed == 1);
	CHECK(listener->value == "");

	ClickAt(context, 100, 20); // Open select again
	ClickAt(context, 100, 85); // Click option 'Cylinder'
	CHECK(listener->num_events_processed == 2);
	CHECK(listener->value == "c");

	ClickAt(context, 100, 20);             // Open select again
	context->ProcessMouseMove(100, 85, 0); // Hover option 'Cylinder'
	context->Update();
	context->Render();
	context->ProcessKeyDown(Rml::Input::KI_DOWN, 0);
	context->Update();
	context->Render();
	CHECK(listener->num_events_processed == 3);
	CHECK(listener->value == "d");
	context->ProcessKeyDown(Rml::Input::KI_DOWN, 0);
	context->Update();
	context->Render();
	CHECK(listener->num_events_processed == 3);

	document->Close();

	TestsShell::ShutdownShell();
}
