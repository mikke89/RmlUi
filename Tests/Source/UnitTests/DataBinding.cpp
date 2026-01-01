#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <cmath>
#include <doctest.h>

using namespace Rml;

namespace {

static const String data_binding_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/template" href="/assets/window.rml"/>
	<style>
		body.window
		{
			left: 50px;
			right: 50px;
			top: 30px;
			bottom: 30px;
			max-width: none;
			max-height: none;
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

<input type="text" data-value="i0"/>

<h1>Globals</h1>
<p>{{ i0 }}</p>
<p>{{ i1 }}</p>
<p>{{ i2 }}</p>
<p>{{ i3 }}</p>

<p>{{ s0 }}</p>
<p>{{ s1 }}</p>
<p>{{ s2.val }}</p>
<p>{{ s3.val }}</p>
<p>{{ s4.val }}</p>
<p>{{ s5.val }}</p>
<p id="simple">{{ simple }}</p>
<p id="simple_custom">{{ simple_custom }}</p>
<p id="scoped">{{ scoped }}</p>
<p id="scoped_custom">{{ scoped_custom }}</p>

<h1>Basic</h1>
<p>{{ basic.a }}</p>
<p>{{ basic.b }}</p>
<p>{{ basic.c }}</p>
<p>{{ basic.d }}</p>
<p>{{ basic.e }}</p>
<p>{{ basic.f }}</p>
<p id="simple" data-event-click="basic.simple = 2">{{ basic.simple }}</p>
<p>{{ basic.scoped }}</p>

<h1>Wrapped</h1>
<p>{{ wrapped.a.val }}</p>
<p>{{ wrapped.b.val }}</p>
<p>{{ wrapped.c.val }}</p>
<p>{{ wrapped.d.val }}</p>
<p>{{ wrapped.e.val }}</p>

<h1>Pointed</h1>
<p>{{ pointed.a.val }}</p>
<p>{{ pointed.b.val }}</p>
<p>{{ pointed.c.val }}</p>

<h1>Arrays</h1>
<p><span data-for="arrays.a">{{ it }} </span></p>
<p><span data-for="arrays.b">{{ it }} </span></p>
<p><span data-for="arrays.c">{{ it.val }} </span></p>
<p><span data-for="arrays.d">{{ it.val }} </span></p>
<p><span data-for="arrays.e">{{ it.val }} </span></p>

</div>
</body>
</rml>
)";

static const String inside_string_rml = R"(
<rml>
<head>
	<title>Test</title>
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
	</style>
</head>

<body template="window">
<div data-model="basics">

<p>{{ i0 }}</p>
<p>{{ 'i0' }}</p>
<p>{{ 'i{}23' }}</p>
<p>before {{ 'i{{test}}23' }} test</p>
<p>a {{ 'i' }} b {{ 'j' }} c</p>
<p>{{i0}}</p>
<p>{{ 'i{}' }}</p>

</div>
</body>
</rml>
)";

static const String aliasing_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<link type="text/rcss" href="/assets/invader.rcss"/>
	<link type="text/template" href="/../Tests/Data/UnitTests/data-title.rml"/>
	<style>
		body {
			width: 600px;
			height: 400px;
			background: #ccc;
			color: #333;
		}
		.title-wrapper { border: 1dp red; }
		.icon { width: 64dp; height: 64dp; display: inline-block; }
		.icon[icon="a"] { decorator: image("/assets/high_scores_alien_1.tga"); }
		.icon[icon="b"] { decorator: image("/assets/high_scores_alien_2.tga"); }
	</style>
</head>

<body data-model="basics">
<p>{{ i0 }}</p>
<p data-alias-differentname="i0">{{ differentname }}</p>
<div data-alias-title="s0" data-alias-icon="wrapped.a.val" id="w1">
	<template src="data-title"/>
</div>
<div data-alias-title="s1" data-alias-icon="wrapped.b.val" id="w2">
	<template src="data-title"/>
</div>
</body>
</rml>
)";

static const String dynamic_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<link type="text/rcss" href="/assets/invader.rcss"/>
	<link type="text/template" href="/../Tests/Data/UnitTests/data-title.rml"/>
</head>

<body data-model="basics">
<p>{{ arrays.a[0] }}</p>
<p>{{ arrays.a[i0] }}</p>
<p>{{ arrays.b[i1] }}</p>
<p>{{ arrays.c[arrays.b[i1] - 19].val }}</p>
<p>{{ arrays.c[sqrt(arrays.b[i1] - 12) - 1].val }}</p>
</body>
</rml>
)";

struct StringWrap {
	StringWrap(String val = "wrap_default") : val(val) {}
	String val;
};

enum SimpleEnum { Simple_Zero = 0, Simple_One, Simple_Two };

enum SimpleEnumCustom { Simple_Zero_Custom, Simple_One_Custom, Simple_Two_Custom };

enum class ScopedEnum : uint64_t { Zero = 0, One, Two };

enum class ScopedEnumCustom { Zero, One, Two };

struct Globals {
	int i0 = 0;
	int* i1 = new int(1);
	UniquePtr<int> i2 = MakeUnique<int>(2);
	SharedPtr<int> i3 = MakeShared<int>(3);

	SimpleEnum simple = Simple_One;
	SimpleEnumCustom simple_custom = Simple_One_Custom;
	ScopedEnum scoped = ScopedEnum::One;
	ScopedEnumCustom scoped_custom = ScopedEnumCustom::One;

	String s0 = "s0";
	String* s1 = new String("s1");
	StringWrap s2 = StringWrap("s2");
	StringWrap* s3 = new StringWrap("s3");
	UniquePtr<StringWrap> s4 = MakeUnique<StringWrap>("s4");
	SharedPtr<StringWrap> s5 = MakeShared<StringWrap>("s5");

	// Invalid
	const int x0 = 100;                                            // Invalid: const variable
	const int* x1 = new int(101);                                  // Invalid: const pointer
	UniquePtr<const int> x2 = MakeUnique<int>(102);                // Invalid: const pointer
	const StringWrap* x3 = new StringWrap("x2");                   // Invalid: const pointer
	UniquePtr<const StringWrap> x4 = MakeUnique<StringWrap>("x3"); // Invalid: const pointer
} globals;

struct Basic {
	int a = 1;
	int* b = new int(2);

	SimpleEnum simple = Simple_One;
	ScopedEnum scoped = ScopedEnum::One;

	int GetC()
	{
		static int v = 5;
		return v;
	}
	int& GetD()
	{
		static int v = 5;
		return v;
	}
	int* GetE()
	{
		static int v = 6;
		return &v;
	}
	UniquePtr<int> GetF() { return MakeUnique<int>(7); }

	// Invalid: const member
	const int x0 = 2;
	// Invalid: const pointer
	const int* x1 = new int(3);
	// Invalid: const qualified member function
	int GetX2() const { return 4; }
	// Invalid: const reference return
	const int& GetX3()
	{
		static int g = 7;
		return g;
	}
	// Invalid: const pointer return
	const int* GetX4()
	{
		static int h = 8;
		return &h;
	}
	// Invalid: Illegal signature
	int GetX5(int) { return 9; }
};

struct Wrapped {
	StringWrap a = {"a"};
	StringWrap* b = new StringWrap("b");
	UniquePtr<StringWrap> c = MakeUnique<StringWrap>("c");

	StringWrap& GetD()
	{
		static StringWrap v = {"e"};
		return v;
	}
	StringWrap* GetE()
	{
		static StringWrap v = {"f"};
		return &v;
	}

	// Invalid: const pointer
	const StringWrap* x0 = new StringWrap("x0");
	// Invalid (run-time): Returning non-scalar variable by value.
	StringWrap GetX1() { return {"x1"}; }
	// Invalid (run-time): Returning non-scalar variable by value.
	UniquePtr<StringWrap> GetX2() { return MakeUnique<StringWrap>("x2"); }
};

using StringWrapPtr = UniquePtr<StringWrap>;

struct Pointed {
	StringWrapPtr a = MakeUnique<StringWrap>("a");

	StringWrapPtr& GetB()
	{
		static StringWrapPtr v = MakeUnique<StringWrap>("b");
		return v;
	}
	StringWrapPtr* GetC()
	{
		static StringWrapPtr v = MakeUnique<StringWrap>("c");
		return &v;
	}

	// Invalid: We disallow recursive pointer types (pointer to pointer)
	StringWrapPtr* x0 = new StringWrapPtr(new StringWrap("x0"));

	// Invalid (run-time error): Only scalar data members can be returned by value
	StringWrapPtr GetX1() { return MakeUnique<StringWrap>("x1"); }
};

struct Arrays {
	Vector<int> a = {10, 11, 12};
	Vector<int*> b = {new int(20), new int(21), new int(22)};
	Vector<StringWrap> c = {StringWrap("c1"), StringWrap("c2"), StringWrap("c3")};
	Vector<StringWrap*> d = {new StringWrap("d1"), new StringWrap("d2"), new StringWrap("d3")};
	Vector<StringWrapPtr> e;

	// Invalid: const pointer
	Vector<const int*> x0 = {new int(30), new int(31), new int(32)};
	// Invalid: const pointer
	Vector<UniquePtr<const StringWrap>> x1;

	Arrays()
	{
		e.emplace_back(MakeUnique<StringWrap>("e1"));
		e.emplace_back(MakeUnique<StringWrap>("e2"));
		e.emplace_back(MakeUnique<StringWrap>("e3"));
		x1.emplace_back(MakeUnique<StringWrap>("x1_1"));
		x1.emplace_back(MakeUnique<StringWrap>("x1_2"));
		x1.emplace_back(MakeUnique<StringWrap>("x1_3"));
	}
};

DataModelHandle model_handle;

bool InitializeDataBindings(Context* context)
{
	Rml::DataModelConstructor constructor = context->CreateDataModel("basics");
	if (!constructor)
		return false;

	constructor.RegisterTransformFunc("sqrt", [](const VariantList& params) {
		if (params.empty())
			return Variant();
		return Variant(std::sqrt(params[0].Get<int>()));
	});

	if (auto handle = constructor.RegisterStruct<StringWrap>())
	{
		handle.RegisterMember("val", &StringWrap::val);
	}

	constructor.RegisterScalar<SimpleEnumCustom>(
		[](const SimpleEnumCustom& value, Rml::Variant& variant) {
			if (value == Simple_Zero_Custom)
			{
				variant = "Zero";
			}
			else if (value == Simple_One_Custom)
			{
				variant = "One";
			}
			else if (value == Simple_Two_Custom)
			{
				variant = "Two";
			}
			else
			{
				Rml::Log::Message(Rml::Log::LT_ERROR, "Invalid value for SimpleEnumCustom type.");
			}
		},
		[](SimpleEnumCustom& value, const Rml::Variant& variant) {
			Rml::String str = variant.Get<Rml::String>();
			if (str == "Zero")
			{
				value = Simple_Zero_Custom;
			}
			else if (str == "One")
			{
				value = Simple_One_Custom;
			}
			else if (str == "Two")
			{
				value = Simple_Two_Custom;
			}
			else
			{
				Rml::Log::Message(Rml::Log::LT_ERROR, "Can't convert '%s' to SimpleEnumCustom.", str.c_str());
			}
		});

	constructor.RegisterScalar<ScopedEnumCustom>(
		[](const ScopedEnumCustom& value, Rml::Variant& variant) {
			if (value == ScopedEnumCustom::Zero)
			{
				variant = "Zero";
			}
			else if (value == ScopedEnumCustom::One)
			{
				variant = "One";
			}
			else if (value == ScopedEnumCustom::Two)
			{
				variant = "Two";
			}
			else
			{
				Rml::Log::Message(Rml::Log::LT_ERROR, "Invalid value for ScopedEnumCustom type.");
			}
		},
		[](ScopedEnumCustom& value, const Rml::Variant& variant) {
			Rml::String str = variant.Get<Rml::String>();
			if (str == "Zero")
			{
				value = ScopedEnumCustom::Zero;
			}
			else if (str == "One")
			{
				value = ScopedEnumCustom::One;
			}
			else if (str == "Two")
			{
				value = ScopedEnumCustom::Two;
			}
			else
			{
				Rml::Log::Message(Rml::Log::LT_ERROR, "Can't convert '%s' to ScopedEnumCustom.", str.c_str());
			}
		});

	{
		// Globals
		constructor.Bind("i0", &globals.i0);
		constructor.Bind("i1", &globals.i1);
		constructor.Bind("i2", &globals.i2);
		constructor.Bind("i3", &globals.i3);

		constructor.Bind("s0", &globals.s0);
		constructor.Bind("s1", &globals.s1);
		constructor.Bind("s2", &globals.s2);
		constructor.Bind("s3", &globals.s3);
		constructor.Bind("s4", &globals.s4);
		constructor.Bind("s5", &globals.s5);

		constructor.Bind("simple", &globals.simple);
		constructor.Bind("simple_custom", &globals.simple_custom);
		constructor.Bind("scoped", &globals.scoped);
		constructor.Bind("scoped_custom", &globals.scoped_custom);
		// Invalid: Each of the following should give a compile-time failure.
		// constructor.Bind("x0", &globals.x0);
		// constructor.Bind("x1", &globals.x1);
		// constructor.Bind("x2", &globals.x2);
		// constructor.Bind("x3", &globals.x3);
		// constructor.Bind("x4", &globals.x4);
	}

	if (auto handle = constructor.RegisterStruct<Basic>())
	{
		handle.RegisterMember("a", &Basic::a);
		handle.RegisterMember("b", &Basic::b);
		handle.RegisterMember("c", &Basic::GetC);
		handle.RegisterMember("d", &Basic::GetD);
		handle.RegisterMember("e", &Basic::GetE);
		handle.RegisterMember("f", &Basic::GetF);
		handle.RegisterMember("simple", &Basic::simple);
		handle.RegisterMember("scoped", &Basic::scoped);

		// handle.RegisterMember("x0", &Basic::x0);
		// handle.RegisterMember("x1", &Basic::x1);
		// handle.RegisterMember("x2", &Basic::GetX2);
		// handle.RegisterMember("x3", &Basic::GetX3);
		// handle.RegisterMember("x4", &Basic::GetX4);
		// handle.RegisterMember("x5", &Basic::GetX5);
	}
	constructor.Bind("basic", new Basic);

	if (auto handle = constructor.RegisterStruct<Wrapped>())
	{
		handle.RegisterMember("a", &Wrapped::a);
		handle.RegisterMember("b", &Wrapped::b);
		handle.RegisterMember("c", &Wrapped::c);
		handle.RegisterMember("d", &Wrapped::GetD);
		handle.RegisterMember("e", &Wrapped::GetE);

		// handle.RegisterMember("x0", &Wrapped::x0);
		// handle.RegisterMember("x1", &Wrapped::GetX1);
		// handle.RegisterMember("x2", &Wrapped::GetX2);
	}
	constructor.Bind("wrapped", new Wrapped);

	if (auto handle = constructor.RegisterStruct<Pointed>())
	{
		handle.RegisterMember("a", &Pointed::a);
		handle.RegisterMember("b", &Pointed::GetB);
		handle.RegisterMember("c", &Pointed::GetC);

		// handle.RegisterMember("x0", &Pointed::x0);
		// handle.RegisterMember("x1", &Pointed::GetX1);
	}
	constructor.Bind("pointed", new Pointed);

	constructor.RegisterArray<decltype(Arrays::a)>();
	constructor.RegisterArray<decltype(Arrays::b)>();
	constructor.RegisterArray<decltype(Arrays::c)>();
	constructor.RegisterArray<decltype(Arrays::d)>();
	constructor.RegisterArray<decltype(Arrays::e)>();

	// constructor.RegisterArray<decltype(Arrays::x0)>();
	// constructor.RegisterArray<decltype(Arrays::x1)>();

	if (auto handle = constructor.RegisterStruct<Arrays>())
	{
		handle.RegisterMember("a", &Arrays::a);
		handle.RegisterMember("b", &Arrays::b);
		handle.RegisterMember("c", &Arrays::c);
		handle.RegisterMember("d", &Arrays::d);
		handle.RegisterMember("e", &Arrays::e);

		// handle.RegisterMember("x0", &Arrays::x0);
		// handle.RegisterMember("x1", &Arrays::x1);
	}
	constructor.Bind("arrays", new Arrays);

	model_handle = constructor.GetModelHandle();

	return true;
}

} // Anonymous namespace

TEST_CASE("data_binding")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	REQUIRE(InitializeDataBindings(context));

	ElementDocument* document = context->LoadDocumentFromMemory(data_binding_rml);
	REQUIRE(document);
	document->Show();

	TestsShell::RenderLoop();

	Element* element = document->GetElementById("simple");
	CHECK(element->GetInnerRML() == "1");

	element = document->GetElementById("simple_custom");
	CHECK(element->GetInnerRML() == "One");

	element = document->GetElementById("scoped");
	CHECK(element->GetInnerRML() == "1");

	element = document->GetElementById("scoped_custom");
	CHECK(element->GetInnerRML() == "One");

	document->Close();

	TestsShell::ShutdownShell();
}

TEST_CASE("data_binding.inside_string")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	REQUIRE(InitializeDataBindings(context));

	ElementDocument* document = context->LoadDocumentFromMemory(inside_string_rml);
	REQUIRE(document);
	document->Show();

	TestsShell::RenderLoop();

	CHECK(document->QuerySelector("p:nth-child(4)")->GetInnerRML() == "before i{{test}}23 test");
	CHECK(document->QuerySelector("p:nth-child(5)")->GetInnerRML() == "a i b j c");

	document->Close();

	TestsShell::ShutdownShell();
}
TEST_CASE("data_binding.aliasing")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	REQUIRE(InitializeDataBindings(context));

	ElementDocument* document = context->LoadDocumentFromMemory(aliasing_rml);
	REQUIRE(document);
	document->Show();

	TestsShell::RenderLoop();

	CHECK(document->QuerySelector("p:nth-child(1)")->GetInnerRML() == document->QuerySelector("p:nth-child(2)")->GetInnerRML());
	CHECK(document->QuerySelector("#w1 .title")->GetInnerRML() == "s0");
	CHECK(document->QuerySelector("#w1 .icon")->GetAttribute("icon", String()) == "a");
	CHECK(document->QuerySelector("#w2 .title")->GetInnerRML() == "s1");
	CHECK(document->QuerySelector("#w2 .icon")->GetAttribute("icon", String()) == "b");

	document->Close();

	TestsShell::ShutdownShell();
}

TEST_CASE("data_binding.dynamic_variables")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	REQUIRE(InitializeDataBindings(context));

	ElementDocument* document = context->LoadDocumentFromMemory(dynamic_rml);
	REQUIRE(document);
	document->Show();

	TestsShell::RenderLoop();

	CHECK(document->QuerySelector("p:nth-child(1)")->GetInnerRML() == "10");
	CHECK(document->QuerySelector("p:nth-child(2)")->GetInnerRML() == "10");
	CHECK(document->QuerySelector("p:nth-child(3)")->GetInnerRML() == "21");
	CHECK(document->QuerySelector("p:nth-child(4)")->GetInnerRML() == "c3");
	CHECK(document->QuerySelector("p:nth-child(5)")->GetInnerRML() == "c3");

	*globals.i1 = 0;
	context->GetDataModel("basics").GetModelHandle().DirtyVariable("i1");
	TestsShell::RenderLoop();

	CHECK(document->QuerySelector("p:nth-child(3)")->GetInnerRML() == "20");
	CHECK(document->QuerySelector("p:nth-child(4)")->GetInnerRML() == "c2");

	document->Close();
	*globals.i1 = 1;

	TestsShell::ShutdownShell();
}

static const String set_enum_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/template" href="/assets/window.rml"/>
	<style>
		body.window {
			width: 500px;
			height: 400px;
		}
	</style>
</head>
<body template="window">
<div data-model="basics">
<p id="simple" data-event-click="simple = 2">{{ simple }}</p>
</div>
</body>
</rml>
)";

TEST_CASE("data_binding.set_enum")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	globals.simple = Simple_One;

	REQUIRE(InitializeDataBindings(context));

	ElementDocument* document = context->LoadDocumentFromMemory(set_enum_rml);
	REQUIRE(document);
	document->Show();

	TestsShell::RenderLoop();

	Element* element = document->GetElementById("simple");
	CHECK(element->GetInnerRML() == "1");

	element->DispatchEvent(EventId::Click, Dictionary());
	TestsShell::RenderLoop();

	CHECK(globals.simple == Simple_Two);
	CHECK(element->GetInnerRML() == "2");

	document->Close();
	TestsShell::ShutdownShell();
}

static const String data_model_on_body_rml = R"(
<rml>
<head>
	<title>Test</title>
	<link type="text/rcss" href="/assets/rml.rcss"/>
	<link type="text/rcss" href="/assets/invader.rcss"/>
	<link type="text/template" href="/assets/window.rml"/>
	<style>
		body {
			width: 500px;
			height: 400px;
			background: #ccc;
			color: #333;
		}
	</style>
</head>
<body BODY_ATTRIBUTE>
	<div DIV_ATTRIBUTE>
		<div id="simple">{{ simple }}</div>
		<div id="s2_val">{{ s2.val }}</div>
		<div id="array_size">{{ arrays.a.size }}</div>
		<div id="array_empty" data-attr-empty="arrays.a.size == 0"></div>
		<div data-for="value, i : arrays.a">{{ i }}: {{ value }}</div>
	</div>
</body>
</rml>
)";

TEST_CASE("data_binding.data_model_on_body")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	REQUIRE(InitializeDataBindings(context));

	const String data_model_attribute = R"( data-model="basics")";
	const String template_attribute = R"( template="window")";

	String document_rml;
	SUBCASE("data_model_on_div")
	{
		document_rml = StringUtilities::Replace(data_model_on_body_rml, "BODY_ATTRIBUTE", "");
		document_rml = StringUtilities::Replace(document_rml, "DIV_ATTRIBUTE", data_model_attribute);
	}
	SUBCASE("data_model_on_body")
	{
		document_rml = StringUtilities::Replace(data_model_on_body_rml, "BODY_ATTRIBUTE", data_model_attribute);
		document_rml = StringUtilities::Replace(document_rml, "DIV_ATTRIBUTE", "");
	}
	SUBCASE("data_model_on_body_with_template")
	{
		document_rml = StringUtilities::Replace(data_model_on_body_rml, "BODY_ATTRIBUTE", data_model_attribute + template_attribute);
		document_rml = StringUtilities::Replace(document_rml, "DIV_ATTRIBUTE", "");
	}

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	REQUIRE(document);
	document->Show();

	TestsShell::RenderLoop();

	CHECK(document->GetElementById("simple")->GetInnerRML() == Rml::ToString(int(globals.simple)));
	CHECK(document->GetElementById("s2_val")->GetInnerRML() == globals.s2.val);

	const Vector<int> array = Arrays{}.a;
	CHECK(document->GetElementById("array_size")->GetInnerRML() == Rml::ToString(array.size()));
	CHECK(document->GetElementById("array_empty")->GetAttribute<bool>("empty", true) == array.empty());

	Element* element = document->GetElementById("array_empty")->GetNextSibling();
	size_t i = 0;
	for (; i < array.size() && element; ++i)
	{
		CHECK(element->GetInnerRML() == Rml::CreateString("%zu: %d", i, array[i]));
		element = element->GetNextSibling();
	}
	CHECK(i == array.size());

	document->Close();
	TestsShell::ShutdownShell();
}
