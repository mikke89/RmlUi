#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <doctest.h>

using namespace Rml;

static const String document_wrapped_image_rml = R"(
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
	</style>
</head>

<body>
<div>
	<img src="/assets/high_scores_alien_1.tga"/>
</div>
</body>
</rml>
)";

TEST_CASE("elementimage.dp_ratio")
{
	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocumentFromMemory(document_wrapped_image_rml, "assets/");

	Element* div = document->GetChild(0);
	Element* img = div->GetChild(0);

	document->Show();
	const float img_width = img->GetClientWidth();

	SUBCASE("basic")
	{
		context->SetDensityIndependentPixelRatio(2.0f);
		context->Update();
		CHECK(img->GetClientWidth() == 2.f * img_width);
	}

	SUBCASE("clone")
	{
		context->SetDensityIndependentPixelRatio(2.0f);
		auto clone_ptr = div->Clone();

		context->SetDensityIndependentPixelRatio(4.f);
		Element* clone_div = document->AppendChild(std::move(clone_ptr));
		Element* clone_img = clone_div->GetChild(0);
		context->Update();

		CHECK(clone_img->GetClientWidth() == 4.f * img_width);
	}

	SUBCASE("create_image")
	{
		context->SetDensityIndependentPixelRatio(2.0f);
		ElementPtr new_img_ptr = document->CreateElement("img");
		new_img_ptr->SetAttribute("src", "high_scores_alien_1.tga");

		context->SetDensityIndependentPixelRatio(4.0f);
		Element* new_img = document->AppendChild(std::move(new_img_ptr));
		context->Update();

		CHECK(new_img->GetClientWidth() == 4.f * img_width);
	}

	TestsShell::RenderLoop();

	document->Close();
	TestsShell::ShutdownShell();
}

static const String document_wrapped_image_rml_ratio_test = R"(
<rml>
<head>
	<title>Test</title>
	<style>
		.width-50 { width: 50px; }
		.height-50 { height: 50px; }
		.height-100 { height: 100px; }
	</style>
</head>
<body>
	<img src="/test_not_loaded/test_512_256.tga" id="test1" alt="Original size" />
	<img src="/test_not_loaded/test_512_256.tga" id="test2" height="50" alt="Set height to 50, width auto, attribute set" />
	<img src="/test_not_loaded/test_512_256.tga" id="test3" class="height-50" alt="Set height to 50, width auto, css set" />
	<img src="/test_not_loaded/test_512_256.tga" id="test4" width="50" alt="Set width to 50, height auto, attribute set" />
	<img src="/test_not_loaded/test_512_256.tga" id="test5" class="width-50" alt="Set width to 50, height auto, css set" />
	<img src="/test_not_loaded/test_512_256.tga" id="test6" width="50" height="100" alt="Set width to 50 and height to 100, attribute set" />
	<img src="/test_not_loaded/test_512_256.tga" id="test7" class="height-100 width-50" alt="Set width to 50 and height to 100, css set" />
</body>
</rml>
)";

TEST_CASE("elementimage.preserve_ratio")
{
	Context* context = TestsShell::GetContext();
	ElementDocument* document = context->LoadDocumentFromMemory(document_wrapped_image_rml_ratio_test, "assets/");
	document->Show();

	// Texture size is hardcoded in test render interface to 512x256
	{
		INFO("original image size");
		Element* img_test1 = document->GetElementById("test1");
		CHECK(img_test1->GetClientWidth() == 512);
		CHECK(img_test1->GetClientHeight() == 256);
	}

	{
		INFO("height only - attribute set");
		Element* img_test2 = document->GetElementById("test2");
		CHECK(img_test2->GetClientWidth() == 100);
		CHECK(img_test2->GetClientHeight() == 50);
	}

	{
		INFO("height only - css set");
		Element* img_test3 = document->GetElementById("test3");
		CHECK(img_test3->GetClientWidth() == 100);
		CHECK(img_test3->GetClientHeight() == 50);
	}

	{
		INFO("width only - attribute set");
		Element* img_test4 = document->GetElementById("test4");
		CHECK(img_test4->GetClientWidth() == 50);
		CHECK(img_test4->GetClientHeight() == 25);
	}

	{
		INFO("width only - css set");
		Element* img_test5 = document->GetElementById("test5");
		CHECK(img_test5->GetClientWidth() == 50);
		CHECK(img_test5->GetClientHeight() == 25);
	}

	{
		INFO("height and width - attribute set");
		Element* img_test6 = document->GetElementById("test6");
		CHECK(img_test6->GetClientWidth() == 50);
		CHECK(img_test6->GetClientHeight() == 100);
	}

	{
		INFO("height and width - css set");
		Element* img_test7 = document->GetElementById("test7");
		CHECK(img_test7->GetClientWidth() == 50);
		CHECK(img_test7->GetClientHeight() == 100);
	}

	TestsShell::RenderLoop();

	document->Close();
	TestsShell::ShutdownShell();
}
