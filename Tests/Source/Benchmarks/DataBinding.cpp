#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <doctest.h>
#include <nanobench.h>

using namespace Rml;
using namespace ankerl;

static const String document_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<link type="text/template" href="/assets/window.rml"/>
	<style>
		body.window
		{
			left: 50px;
			right: 50px;
			top: 30px;
			bottom: 30px;
			max-width: -1px;
			max-height: -1px;
		}
		div#content
		{
			text-align: left;
			padding: 50px;
			box-sizing: border-box;
		}
	</style>
</head>

<body template="window">
<div data-model="basics">
<h1>Globals</h1>
<p id="i">0</p>

<p id="i0">{{ i0 }}</p>
<p id="i1">{{ i1 }}</p>
<p id="i2">{{ i2 }}</p>
<p id="i3">{{ i3 }}</p>

<h1>Basic</h1>
<p id="b_a">{{ basic.a }}</p>
<p id="b_b">{{ basic.b }}</p>
<p id="b_c">{{ basic.c.val }}</p>

<h1>Arrays</h1>
<p id="array"><span>10 </span><span>20 </span><span>30 </span></p>
<p><span data-for="arrays.a">{{ it }} </span></p>
<p><span data-for="arrays.b">{{ it }} </span></p>
<p><span data-for="arrays.c">{{ it.val }} </span></p>
<p><span data-for="arrays.d">{{ 'a: ' + it.a + ', b: ' + it.b + ', c: ' + it.c.val + ' :: ' }}</span></p>
</div>
</body>
</rml>
)";

struct StringWrap {
	StringWrap(String val = "wrap_default") : val(val) {}
	String val;
};

struct Globals {
	int i0 = 0;
	int* i1 = new int(1);
	UniquePtr<int> i2 = MakeUnique<int>(2);
	SharedPtr<int> i3 = MakeShared<int>(3);
} globals;

struct Basic {
	int a = 1;
	int* b = new int(2);
	StringWrap* c = new StringWrap("basic.c");
};

struct Arrays {
	Vector<int> a = {10, 11, 12};
	Vector<int*> b = {new int(20), new int(21), new int(22)};
	Vector<StringWrap> c = {StringWrap("c1"), StringWrap("c2"), StringWrap("c3")};
	Vector<Basic> d = {Basic{10}, Basic{20}, Basic{30}};
};

static UniquePtr<Basic> basic;
static UniquePtr<Arrays> arrays;

static DataModelHandle InitializeDataBindings(Context* context)
{
	Rml::DataModelConstructor constructor = context->CreateDataModel("basics");
	if (!constructor)
		return DataModelHandle();

	if (auto handle = constructor.RegisterStruct<StringWrap>())
		handle.RegisterMember("val", &StringWrap::val);

	constructor.Bind("i0", &globals.i0);
	constructor.Bind("i1", &globals.i1);
	constructor.Bind("i2", &globals.i2);
	constructor.Bind("i3", &globals.i3);

	if (auto handle = constructor.RegisterStruct<Basic>())
	{
		handle.RegisterMember("a", &Basic::a);
		handle.RegisterMember("b", &Basic::b);
		handle.RegisterMember("c", &Basic::c);
	}
	basic = MakeUnique<Basic>();
	constructor.Bind("basic", basic.get());

	constructor.RegisterArray<decltype(Arrays::a)>();
	constructor.RegisterArray<decltype(Arrays::b)>();
	constructor.RegisterArray<decltype(Arrays::c)>();
	constructor.RegisterArray<decltype(Arrays::d)>();

	if (auto handle = constructor.RegisterStruct<Arrays>())
	{
		handle.RegisterMember("a", &Arrays::a);
		handle.RegisterMember("b", &Arrays::b);
		handle.RegisterMember("c", &Arrays::c);
		handle.RegisterMember("d", &Arrays::d);
	}
	arrays = MakeUnique<Arrays>();
	constructor.Bind("arrays", arrays.get());

	DataModelHandle model_handle = constructor.GetModelHandle();

	return model_handle;
}

TEST_CASE("data_binding")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	DataModelHandle model_handle = InitializeDataBindings(context);

	model_handle.DirtyAllVariables();

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	REQUIRE(document);
	document->Show();

	context->Update();
	context->Render();

	SUBCASE("dirty")
	{
		nanobench::Bench bench;
		bench.title("Data bindings: Dirty variables");
		bench.relative(true);

		bench.run("Reference (Update)", [&] { context->Update(); });
		bench.run("Dirty one variable", [&] {
			model_handle.DirtyVariable("i0");
			context->Update();
		});
		bench.run("Dirty big variable", [&] {
			model_handle.DirtyVariable("arrays");
			context->Update();
		});
		bench.run("Dirty all variables", [&] {
			model_handle.DirtyAllVariables();
			context->Update();
		});
	}

	SUBCASE("update")
	{
		Element* element_i = document->GetElementById("i");
		Element* element_array = document->GetElementById("array");

		nanobench::Rng rng;
		nanobench::Bench bench;
		bench.title("Data bindings: Update");
		bench.relative(true);

		bench.run("Reference (Integer)", [&] {
			element_i->SetInnerRML(Rml::ToString(rng.bounded(1000)));
			context->Update();
		});

		bench.run("Integer", [&] {
			globals.i0 = rng.bounded(1000);
			model_handle.DirtyVariable("i0");
			context->Update();
		});

		bench.run("Basic", [&] {
			basic->a = rng.bounded(2000);
			*basic->b = rng.bounded(3000);
			basic->c->val = String("abc") + String(5, char('a' + rng.bounded('z' - 'a')));
			model_handle.DirtyVariable("basic");
			context->Update();
		});

		bench.run("Reference (Arrays)", [&] {
			element_array->SetInnerRML(
				Rml::CreateString("<span>%d </span><span>%d </span><span>%d </span>", rng.bounded(5000), rng.bounded(5000), rng.bounded(5000)));
			context->Update();
		});

		bench.run("Arrays", [&] {
			for (auto& v : arrays->a)
				v = rng.bounded(5000);
			model_handle.DirtyVariable("arrays");
			context->Update();
		});
	}

	TestsShell::RenderLoop();

	document->Close();

	TestsShell::ShutdownShell();
}
