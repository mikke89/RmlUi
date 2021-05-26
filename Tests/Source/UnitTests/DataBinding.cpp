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
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <doctest.h>
#include <map>

using namespace Rml;

namespace {


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

<h1>Basic</h1>
<p>{{ basic.a }}</p>
<p>{{ basic.b }}</p>
<p>{{ basic.c }}</p>
<p>{{ basic.d }}</p>
<p>{{ basic.e }}</p>
<p>{{ basic.f }}</p>

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

</div>
</body>
</rml>	
)";

struct StringWrap
{
	StringWrap(String val = "wrap_default") : val(val) {}
	String val;
};

struct Globals
{
	int i0 = 0;
	int* i1 = new int(1);
	UniquePtr<int> i2 = MakeUnique<int>(2);
	SharedPtr<int> i3 = MakeShared<int>(3);

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

struct Basic
{
	int a = 1;
	int* b = new int(2);

	int GetC() {
		static int v = 5;
		return v;
	}
	int& GetD() {
		static int v = 5;
		return v;
	}
	int* GetE() {
		static int v = 6;
		return &v;
	}
	UniquePtr<int> GetF() {
		return MakeUnique<int>(7);
	}

	// Invalid: const member
	const int x0 = 2;
	// Invalid: const pointer
	const int* x1 = new int(3);
	// Invalid: const qualified member function
	int GetX2() const {
		return 4;
	}
	// Invalid: const reference return
	const int& GetX3() {
		static int g = 7;
		return g;
	}
	// Invalid: const pointer return
	const int* GetX4() {
		static int h = 8;
		return &h;
	}
	// Invalid: Illegal signature
	int GetX5(int) {
		return 9;
	}
};

struct Wrapped
{
	StringWrap a = { "a" };
	StringWrap* b = new StringWrap("b");
	UniquePtr<StringWrap> c = MakeUnique<StringWrap>("c");

	StringWrap& GetD() {
		static StringWrap v = { "e" };
		return v;
	}
	StringWrap* GetE() {
		static StringWrap v = { "f" };
		return &v;
	}
	
	// Invalid: const pointer
	const StringWrap* x0 = new StringWrap("x0");
	// Invalid (run-time): Returning non-scalar variable by value.
	StringWrap GetX1() {
		return { "x1" };
	}
	// Invalid (run-time): Returning non-scalar variable by value.
	UniquePtr<StringWrap> GetX2() {
		return MakeUnique<StringWrap>("x2");
	}
};

using StringWrapPtr = UniquePtr<StringWrap>;

struct Pointed
{
	StringWrapPtr a = MakeUnique<StringWrap>("a");

	StringWrapPtr& GetB() {
		static StringWrapPtr v = MakeUnique<StringWrap>("b");
		return v;
	}
	StringWrapPtr* GetC() {
		static StringWrapPtr v = MakeUnique<StringWrap>("c");
		return &v;
	}
	
	// Invalid: We disallow recursive pointer types (pointer to pointer)
	StringWrapPtr* x0 = new StringWrapPtr(new StringWrap("x0"));

	// Invalid (run-time error): Only scalar data members can be returned by value
	StringWrapPtr GetX1() {
		return MakeUnique<StringWrap>("x1");
	}

};

struct Arrays
{
	Vector<int> a = { 10, 11, 12 };
	Vector<int*> b = { new int(20), new int(21), new int(22) };
	Vector<StringWrap> c = { StringWrap("c1"), StringWrap("c2"), StringWrap("c3") };
	Vector<StringWrap*> d = { new StringWrap("d1"), new StringWrap("d2"), new StringWrap("d3") };
	Vector<StringWrapPtr> e;
	
	// Invalid: const pointer
	Vector<const int*> x0 = { new int(30), new int(31), new int(32) };
	// Invalid: const pointer
	Vector<UniquePtr<const StringWrap>> x1;
	
	Arrays() {
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

	if (auto handle = constructor.RegisterStruct<StringWrap>())
	{
		handle.RegisterMember("val", &StringWrap::val);
	}

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

		// Invalid: Each of the following should give a compile-time failure.
		//constructor.Bind("x0", &globals.x0);
		//constructor.Bind("x1", &globals.x1);
		//constructor.Bind("x2", &globals.x2);
		//constructor.Bind("x3", &globals.x3);
		//constructor.Bind("x4", &globals.x4);
	}

	if (auto handle = constructor.RegisterStruct<Basic>())
	{
		handle.RegisterMember("a", &Basic::a);
		handle.RegisterMember("b", &Basic::b);
		handle.RegisterMember("c", &Basic::GetC);
		handle.RegisterMember("d", &Basic::GetD);
		handle.RegisterMember("e", &Basic::GetE);
		handle.RegisterMember("f", &Basic::GetF);

		//handle.RegisterMember("x0", &Basic::x0);
		//handle.RegisterMember("x1", &Basic::x1);
		//handle.RegisterMember("x2", &Basic::GetX2);
		//handle.RegisterMember("x3", &Basic::GetX3);
		//handle.RegisterMember("x4", &Basic::GetX4);
		//handle.RegisterMember("x5", &Basic::GetX5);
	}
	constructor.Bind("basic", new Basic);
	
	if (auto handle = constructor.RegisterStruct<Wrapped>())
	{
		handle.RegisterMember("a", &Wrapped::a);
		handle.RegisterMember("b", &Wrapped::b);
		handle.RegisterMember("c", &Wrapped::c);
		handle.RegisterMember("d", &Wrapped::GetD);
		handle.RegisterMember("e", &Wrapped::GetE);

		//handle.RegisterMember("x0", &Wrapped::x0);
		//handle.RegisterMember("x1", &Wrapped::GetX1);
		//handle.RegisterMember("x2", &Wrapped::GetX2);
	}
	constructor.Bind("wrapped", new Wrapped);
	
	if (auto handle = constructor.RegisterStruct<Pointed>())
	{
		handle.RegisterMember("a", &Pointed::a);
		handle.RegisterMember("b", &Pointed::GetB);
		handle.RegisterMember("c", &Pointed::GetC);

		//handle.RegisterMember("x0", &Pointed::x0);
		//handle.RegisterMember("x1", &Pointed::GetX1);
	}
	constructor.Bind("pointed", new Pointed);

	constructor.RegisterArray<decltype(Arrays::a)>();
	constructor.RegisterArray<decltype(Arrays::b)>();
	constructor.RegisterArray<decltype(Arrays::c)>();
	constructor.RegisterArray<decltype(Arrays::d)>();
	constructor.RegisterArray<decltype(Arrays::e)>();

	//constructor.RegisterArray<decltype(Arrays::x0)>();
	//constructor.RegisterArray<decltype(Arrays::x1)>();

	if (auto handle = constructor.RegisterStruct<Arrays>())
	{
		handle.RegisterMember("a", &Arrays::a);
		handle.RegisterMember("b", &Arrays::b);
		handle.RegisterMember("c", &Arrays::c);
		handle.RegisterMember("d", &Arrays::d);
		handle.RegisterMember("e", &Arrays::e);

		//handle.RegisterMember("x0", &Arrays::x0);
		//handle.RegisterMember("x1", &Arrays::x1);
	}
	constructor.Bind("arrays", new Arrays);
	
	model_handle = constructor.GetModelHandle();

	return true;
}

} // Anonymous namespace


TEST_CASE("databinding")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	REQUIRE(InitializeDataBindings(context));

	ElementDocument* document = context->LoadDocumentFromMemory(document_rml);
	REQUIRE(document);
	document->Show();

	context->Update();
	context->Render();

	TestsShell::RenderLoop();

	document->Close();

	TestsShell::ShutdownShell();
}

TEST_CASE("databinding.inside_string")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	REQUIRE(InitializeDataBindings(context));

	ElementDocument* document = context->LoadDocumentFromMemory(inside_string_rml);
	REQUIRE(document);
	document->Show();

	context->Update();
	context->Render();

	TestsShell::RenderLoop();

	CHECK(document->QuerySelector("p:nth-child(4)")->GetInnerRML() == "before i{{test}}23 test");
	CHECK(document->QuerySelector("p:nth-child(5)")->GetInnerRML() == "a i b j c");

	document->Close();

	TestsShell::ShutdownShell();
}