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

<p>{{ test }}</p>
<p>{{ str1.val }}</p>
<p>{{ str2.val }}</p>
<p>{{ str3.val }}</p>
<p>{{ test2.val }}</p>
	
<h1>Basic</h1>
<p>{{ basic.a }}</p>
<p>{{ basic.b }}</p>
<p>{{ basic.c }}</p>
<p>{{ basic.d }}</p>
<p>{{ basic.e }}</p>
<p>{{ basic.f }}</p>
<p>{{ basic.g }}</p>
<p>{{ basic.h }}</p>
	
<h1>Wrapped</h1>
<p>{{ wrapped.a.val }}</p>
<p>{{ wrapped.b.val }}</p>
<p>{{ wrapped.c.val }}</p>
<p>{{ wrapped.d.val }}</p>
<p>{{ wrapped.e.val }}</p>
<p>{{ wrapped.f.val }}</p>
<p>{{ wrapped.g.val }}</p>
<p>{{ wrapped.h.val }}</p>
	
<h1>Pointed</h1>
<p>{{ pointed.a.val }}</p>
<p>{{ pointed.e.val }}</p>
<p>{{ pointed.f.val }}</p>
<p>{{ pointed.g.val }}</p>
<p>{{ pointed.h.val }}</p>
	
<h1>ConstPointed</h1>
<p>{{ const_pointed.a.val }}</p>
<p>{{ const_pointed.e.val }}</p>
<p>{{ const_pointed.f.val }}</p>
<p>{{ const_pointed.g.val }}</p>
<p>{{ const_pointed.h.val }}</p>
	
<h1>Arrays</h1>
<p><span data-for="arrays.a">{{ it }} </span></p>
<p><span data-for="arrays.b">{{ it }} </span></p>
<p><span data-for="arrays.c">{{ it }} </span></p>
<p><span data-for="arrays.d">{{ it.val }} </span></p>
<p><span data-for="arrays.e">{{ it.val }} </span></p>
<p><span data-for="arrays.f">{{ it.val }} </span></p>
<p><span data-for="arrays.g">{{ it.val }} </span></p>

</div>
</body>
</rml>
)";

struct StringWrap
{
	StringWrap(String val = "wrap_default") : val(val) {}
	String val;
};

StringWrap gStr0 = StringWrap("obj");
StringWrap* gStr1 = new StringWrap("raw");
UniquePtr<StringWrap> gStr2 = MakeUnique<StringWrap>("unique");
SharedPtr<StringWrap> gStr3 = MakeShared<StringWrap>("shared");
const StringWrap* gStr4 = new StringWrap("const raw");
const int* const_ptr_test = new int(5);

struct Basic
{
	int a = 1;
	int* b = new int(2);
	const int* c = new int(3);

	int GetD() {
		return 4;
	}
	int& GetE() {
		static int e = 5;
		return e;
	}
	int* GetF() {
		static int f = 6;
		return &f;
	}
	const int& GetG() {
		static int g = 7;
		return g;
	}
	const int* GetH() {
		static int h = 8;
		return &h;
	}

};

struct Wrapped
{
	StringWrap a = { "a" };
	StringWrap* b = new StringWrap("b");
	const StringWrap* c = new StringWrap("c");

	// Illegal: Must return by reference, or return scalar value.
	StringWrap GetD() {
		return { "d" };
	}
	StringWrap& GetE() {
		static StringWrap e = { "e" };
		return e;
	}
	StringWrap* GetF() {
		static StringWrap f = { "f" };
		return &f;
	}
	const StringWrap& GetG() {
		static StringWrap g = { "g" };
		return g;
	}
	const StringWrap* GetH() {
		static StringWrap h = { "h" };
		return &h;
	}
};

using StringWrapPtr = UniquePtr<StringWrap>;

struct Pointed
{
	StringWrapPtr a = MakeUnique<StringWrap>("a");

	// We disallow recursive pointer types (pointer to pointer)
	// Invalid: 
	StringWrapPtr* b = new StringWrapPtr(new StringWrap("b"));
	const StringWrapPtr* c = new StringWrapPtr(new StringWrap("c"));

	StringWrapPtr GetD() {
		return MakeUnique<StringWrap>("d");
	}
	// -- End Invalid

	StringWrapPtr& GetE() {
		static StringWrapPtr e = MakeUnique<StringWrap>("e");
		return e;
	}
	StringWrapPtr* GetF() {
		static StringWrapPtr f = MakeUnique<StringWrap>("f");
		return &f;
	}
	const StringWrapPtr& GetG() {
		static StringWrapPtr g = MakeUnique<StringWrap>("g");
		return g;
	}
	const StringWrapPtr* GetH() {
		static StringWrapPtr h = MakeUnique<StringWrap>("h");
		return &h;
	}
};

using StringWrapConstPtr = UniquePtr<const StringWrap>;

struct ConstPointed
{
	StringWrapConstPtr a = MakeUnique<StringWrap>("a");

	// We disallow recursive pointer types (pointer to pointer)
	// -- Invalid
	StringWrapConstPtr* b = new StringWrapConstPtr(new StringWrap("b"));
	const StringWrapConstPtr* c = new StringWrapConstPtr(new StringWrap("c"));

	StringWrapConstPtr GetD() {
		return MakeUnique<StringWrap>("d");
	}
	// -- End Invalid

	StringWrapConstPtr& GetE() {
		static StringWrapConstPtr e = MakeUnique<StringWrap>("e");
		return e;
	}
	StringWrapConstPtr* GetF() {
		static StringWrapConstPtr f = MakeUnique<StringWrap>("f");
		return &f;
	}
	const StringWrapConstPtr& GetG() {
		static StringWrapConstPtr g = MakeUnique<StringWrap>("g");
		return g;
	}
	const StringWrapConstPtr* GetH() {
		static StringWrapConstPtr h = MakeUnique<StringWrap>("h");
		return &h;
	}
};

struct Arrays {
	Vector<int> a = { 10, 11, 12 };
	Vector<int*> b = { new int(20), new int(21), new int(22) };
	Vector<const int*> c = { new int(30), new int(31), new int(32) };
	Vector<StringWrap> d = { StringWrap("d1"), StringWrap("d2"), StringWrap("d3") };
	Vector<StringWrap*> e = { new StringWrap("e1"), new StringWrap("e2"), new StringWrap("e3") };
	Vector<StringWrapPtr> f;
	Vector<StringWrapConstPtr> g;
	Arrays() {
		f.emplace_back(MakeUnique<StringWrap>("f1"));
		f.emplace_back(MakeUnique<StringWrap>("f2"));
		f.emplace_back(MakeUnique<StringWrap>("f3"));
		g.emplace_back(MakeUnique<StringWrap>("g1"));
		g.emplace_back(MakeUnique<StringWrap>("g2"));
		g.emplace_back(MakeUnique<StringWrap>("g3"));
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

	constructor.Bind("test", &const_ptr_test);
	constructor.Bind("test2", &gStr4);

	constructor.Bind("str0", &gStr0);
	constructor.Bind("str1", &gStr1);
	constructor.Bind("str2", &gStr2);
	constructor.Bind("str3", &gStr3);

	if (auto handle = constructor.RegisterStruct<Basic>())
	{
		handle.RegisterMember("a", &Basic::a);
		handle.RegisterMember("b", &Basic::b);
		handle.RegisterMember("c", &Basic::c);
		handle.RegisterMemberScalar("d", &Basic::GetD);
		handle.RegisterMember("e", &Basic::GetE);
		handle.RegisterMember("f", &Basic::GetF);
		handle.RegisterMember("g", &Basic::GetG);
		handle.RegisterMember("h", &Basic::GetH);
	}
	constructor.Bind("basic", new Basic);
	
	if (auto handle = constructor.RegisterStruct<Wrapped>())
	{
		handle.RegisterMember("a", &Wrapped::a);
		handle.RegisterMember("b", &Wrapped::b);
		handle.RegisterMember("c", &Wrapped::c);
		//handle.RegisterMemberScalar("d", &Wrapped::GetD);
		handle.RegisterMember("e", &Wrapped::GetE);
		handle.RegisterMember("f", &Wrapped::GetF);
		handle.RegisterMember("g", &Wrapped::GetG);
		handle.RegisterMember("h", &Wrapped::GetH);
	}
	constructor.Bind("wrapped", new Wrapped);
	
	if (auto handle = constructor.RegisterStruct<Pointed>())
	{
		handle.RegisterMember("a", &Pointed::a);
		//handle.RegisterMember("b", &Pointed::b);
		//handle.RegisterMember("c", &Pointed::c);
		//handle.RegisterMember("d", &Pointed::GetD);
		handle.RegisterMember("e", &Pointed::GetE);
		handle.RegisterMember("f", &Pointed::GetF);
		handle.RegisterMember("g", &Pointed::GetG);
		handle.RegisterMember("h", &Pointed::GetH);
	}
	constructor.Bind("pointed", new Pointed);

	
	if (auto handle = constructor.RegisterStruct<ConstPointed>())
	{
		handle.RegisterMember("a", &ConstPointed::a);
		//handle.RegisterMember("b", &ConstPointed::b);
		//handle.RegisterMember("c", &ConstPointed::c);
		//handle.RegisterMemberGetter("d", &ConstPointed::GetD);
		handle.RegisterMember("e", &ConstPointed::GetE);
		handle.RegisterMember("f", &ConstPointed::GetF);
		handle.RegisterMember("g", &ConstPointed::GetG);
		handle.RegisterMember("h", &ConstPointed::GetH);
	}
	constructor.Bind("const_pointed", new ConstPointed);


	constructor.RegisterArray<decltype(Arrays::a)>();
	constructor.RegisterArray<decltype(Arrays::b)>();
	constructor.RegisterArray<decltype(Arrays::c)>();
	constructor.RegisterArray<decltype(Arrays::d)>();
	constructor.RegisterArray<decltype(Arrays::e)>();
	constructor.RegisterArray<decltype(Arrays::f)>();
	constructor.RegisterArray<decltype(Arrays::g)>();


	if (auto handle = constructor.RegisterStruct<Arrays>())
	{
		handle.RegisterMember("a", &Arrays::a);
		handle.RegisterMember("b", &Arrays::b);
		handle.RegisterMember("c", &Arrays::c);
		handle.RegisterMember("d", &Arrays::d);
		handle.RegisterMember("e", &Arrays::e);
		handle.RegisterMember("f", &Arrays::f);
		handle.RegisterMember("g", &Arrays::g);
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
