#include "../Common/TestsShell.h"
#include <RmlUi/Core/ComputedValues.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/ElementText.h>
#include <RmlUi/Core/Factory.h>
#include <doctest.h>

using namespace Rml;

static const String document_xml_tags_in_css = R"(
<rml>
    <head>
        <style>
            body {
                /* <body> */
                width: 200px;
                height: 200px;
                background-color: #00ff00;
            }
        </style>
    </head>
    <body>
    </body>
</rml>
)";

static const String document_escaping = R"(
<rml>
    <head>
	<style>
	p {
		font-family: LatoLatin;
	}
	</style>
    </head>
    <body>
	<p id="p">&#x20AC;&#8364;</p>
    </body>
</rml>
)";

static const String document_escaping_tags = R"(
<rml>
    <head>
	<style>
	* {
		font-family: LatoLatin;
	}
	</style>
    </head>
    <body>&lt;p&gt;&amp;lt;span/&amp;gt;&lt;/p&gt;</body>
</rml>
)";

TEST_CASE("XMLParser")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	// Style nodes should accept XML reserved characters, see https://github.com/mikke89/RmlUi/issues/341

	ElementDocument* document = context->LoadDocumentFromMemory(document_xml_tags_in_css);
	REQUIRE(document);
	document->Show();

	TestsShell::RenderLoop();

	const Colourb background = document->GetComputedValues().background_color();
	CHECK(background.red == 0);
	CHECK(background.green == 0xff);
	CHECK(background.blue == 0);
	CHECK(background.alpha == 0xff);

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("XMLParser.escaping")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_escaping);
	REQUIRE(document);
	document->Show();

	TestsShell::RenderLoop();

	auto element = document->GetElementById("p");
	REQUIRE(element);

	CHECK(element->GetInnerRML() == "\xe2\x82\xac\xe2\x82\xac");

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("XMLParser.escaping_tags")
{
	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	ElementDocument* document = context->LoadDocumentFromMemory(document_escaping_tags);
	REQUIRE(document);
	document->Show();

	TestsShell::RenderLoop();

	CHECK(document->GetNumChildren() == 1);
	CHECK(document->GetFirstChild()->GetTagName() == "#text");
	// Text-access should yield decoded value, while RML-access should yield encoded value
	CHECK(static_cast<ElementText*>(document->GetFirstChild())->GetText() == "<p>&lt;span/&gt;</p>");
	CHECK(document->GetInnerRML() == "&lt;p&gt;&amp;lt;span/&amp;gt;&lt;/p&gt;");

	document->Close();
	TestsShell::ShutdownShell();
}

TEST_CASE("XMLParser.comments_and_cdata")
{
	const String document_source_pre = R"(
	<rml>
	    <head>
		<style>
		body {
			font-family: LatoLatin;
		}
		</style>
	    </head>
	    <body>)";
	const String document_source_post = R"(</body></rml>)";

	struct TestCase {
		String rml;
		String expected_parsed_rml;
	};

	const TestCase tests[] = {
		{"<!-- <xyz> -->", ""},
		{"<!--<xyz>-->", ""},
		{"<!-- <xyz> ->-->", ""},
		{"<!-- <xyz> -- >-->", ""},
		{"<!-- <xyz> --->", ""},
		{"<!-- <xyz> ---->", ""},
		{"<!--- <xyz> ---->", ""},
		{"<!-- <p> --><p>hello</p><!-- </p> -->", "<p>hello</p>"},
		{"<![CDATA[hello]]>", "hello"},
		{"<![CDATA[hello]]]>", "hello]"},
		{"<![CDATA[hello]]world]]>", "hello]]world"},
		{"<![CDATA[\"hello\"]]>", "&quot;hello&quot;"},
		{"<![CDATA[<p>world</p>]]>", "<p>world</p>"},
	};

	Context* context = TestsShell::GetContext();
	REQUIRE(context);

	for (const TestCase& test : tests)
	{
		ElementDocument* document = context->LoadDocumentFromMemory(document_source_pre + test.rml + document_source_post);
		REQUIRE(document);

		CHECK(document->GetInnerRML() == test.expected_parsed_rml);

		document->Close();
		context->Update();
	}
	TestsShell::ShutdownShell();
}
