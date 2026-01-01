#include "../Common/Mocks.h"
#include "../Common/TestsInterface.h"
#include "../Common/TestsShell.h"
#include "../Common/TypesToString.h"
#include "RmlUi/Core/DecorationTypes.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <doctest.h>
#include <float.h>

using namespace Rml;

static bool DictionaryApproximateMatch(const Rml::Dictionary& dict, const Rml::Dictionary& dict_expected)
{
	for (auto& pair : dict)
	{
		const String& name = pair.first;
		const Variant& value = pair.second;

		auto it = dict_expected.find(name);
		if (it == dict_expected.end())
		{
			FAIL("Unexpected key: ", name, ". Value: ", value);
			return false;
		}

		const Variant& value_expected = it->second;

		CAPTURE(name);
		REQUIRE(value.GetType() == value_expected.GetType());

		if (value.GetType() == Rml::Variant::Type::FLOAT)
		{
			REQUIRE(value.Get<float>() == doctest::Approx(value_expected.Get<float>()));
		}
		else if (value_expected.GetType() == Rml::Variant::Type::VECTOR2)
		{
			auto a = value.Get<Vector2f>();
			auto b = value_expected.Get<Vector2f>();
			REQUIRE(a.x == doctest::Approx(b.x));
			REQUIRE(a.y == doctest::Approx(b.y));
		}
		else if (value_expected.GetType() == Rml::Variant::Type::COLORSTOPLIST)
		{
			const auto& a = value.GetReference<ColorStopList>();
			const auto& b = value_expected.GetReference<ColorStopList>();
			REQUIRE(a.size() == b.size());
			for (size_t i = 0; i < Math::Min(a.size(), b.size()); i++)
			{
				REQUIRE(a[i].color == b[i].color);
				REQUIRE(a[i].position.unit == b[i].position.unit);
				REQUIRE(a[i].position.number == doctest::Approx(b[i].position.number));
			}
		}
		else
		{
			REQUIRE(value == value_expected);
		}
	}

	for (auto& pair_expected : dict_expected)
	{
		if (dict.find(pair_expected.first) == dict.end())
		{
			FAIL("Missing key: ", pair_expected.first, ". Expected value: ", pair_expected.second);
			return false;
		}
	}

	return true;
}

static const String document_decorator_rml = R"(
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

		@decorator my-gradient : horizontal-gradient {
			start-color: #f0f;
			stop-color: #fff;
		}

		div {
			border: 20px transparent;
			padding: 30px;
			width: 50px;
			height: 50px;
		}

		#content_box {
			decorator: horizontal-gradient(#f00 #ff0) content-box;
		}
		#padding_box {
			decorator: horizontal-gradient(#f00 #ff0) padding-box;
		}
		#auto_box {
			decorator: horizontal-gradient(#f00 #ff0);
		}
		#border_box {
			decorator: horizontal-gradient(#f00 #ff0) border-box;
		}

		body.at_decorator #content_box {
			decorator: my-gradient content-box;
		}
		body.at_decorator #padding_box {
			decorator: my-gradient padding-box;
		}
		body.at_decorator #auto_box {
			decorator: my-gradient;
		}
		body.at_decorator #border_box {
			decorator: my-gradient border-box;
		}
	</style>
</head>

<body>
	<div id="content_box"/>
	<div id="padding_box"/>
	<div id="auto_box"/>
	<div id="border_box"/>
</body>
</rml>
)";

TEST_CASE("decorator.paint-area")
{
	TestsRenderInterface* render_interface = TestsShell::GetTestsRenderInterface();
	// This test only works with the dummy renderer.
	if (!render_interface)
		return;

	Context* context = TestsShell::GetContext();

	ElementDocument* document = context->LoadDocumentFromMemory(document_decorator_rml, "assets/");
	document->Show();

	for (const bool set_at_decorator_class : {false, true})
	{
		document->SetClass("at_decorator", set_at_decorator_class);
		const byte blue = (set_at_decorator_class ? 255 : 0);

		render_interface->ExpectCompileGeometry({
			Mesh{
				Vector<Vertex>{
					{{50, 50}, {255, 0, blue, 255}, {0, 0}},
					{{100, 50}, {255, 255, blue, 255}, {0, 0}},
					{{100, 100}, {255, 255, blue, 255}, {0, 0}},
					{{50, 100}, {255, 0, blue, 255}, {0, 0}},
				},
				Vector<int>{0, 2, 1, 0, 3, 2},
			},
			Mesh{
				Vector<Vertex>{
					{{20, 20}, {255, 0, blue, 255}, {0, 0}},
					{{130, 20}, {255, 255, blue, 255}, {0, 0}},
					{{130, 130}, {255, 255, blue, 255}, {0, 0}},
					{{20, 130}, {255, 0, blue, 255}, {0, 0}},
				},
				Vector<int>{0, 2, 1, 0, 3, 2},
			},
			Mesh{
				Vector<Vertex>{
					{{20, 20}, {255, 0, blue, 255}, {0, 0}},
					{{130, 20}, {255, 255, blue, 255}, {0, 0}},
					{{130, 130}, {255, 255, blue, 255}, {0, 0}},
					{{20, 130}, {255, 0, blue, 255}, {0, 0}},
				},
				Vector<int>{0, 2, 1, 0, 3, 2},
			},
			Mesh{
				Vector<Vertex>{
					{{0, 0}, {255, 0, blue, 255}, {0, 0}},
					{{150, 0}, {255, 255, blue, 255}, {0, 0}},
					{{150, 150}, {255, 255, blue, 255}, {0, 0}},
					{{0, 150}, {255, 0, blue, 255}, {0, 0}},
				},
				Vector<int>{0, 2, 1, 0, 3, 2},
			},
		});

		context->Update();
		context->Render();
	}

	document->Close();

	TestsShell::ShutdownShell();
}

TEST_CASE("decorator.gradients_and_shader")
{
	namespace tl = trompeloeil;

	MockRenderInterface mockRenderInterface;

	Context* context = TestsShell::GetContext(true, &mockRenderInterface);
	REQUIRE(context);

	static const String document_gradients_rml = R"(
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

				div {
					margin: auto;
					border: 10px transparent;
					padding: 50px;
					width: 100px;
					height: 100px;
				}
			</style>
		</head>

		<body>
			<div/>
		</body>
		</rml>
	)";

	struct TestCase {
		String value;
		String expected_name;
		Dictionary expected_dictionary;
	};

	TestCase test_cases[] = {

		// -- linear-gradient --
		TestCase{
			"linear-gradient(to right, #000, #fff)",
			"linear-gradient",
			Dictionary{
				{"length", Variant(200.f)},
				{"p0", Variant(Vector2f{0.f, 100.f})},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0, Unit::NUMBER}},
						ColorStop{{255, 255, 255, 255}, {1, Unit::NUMBER}},
					})},
				{"p1", Variant(Vector2f{200.f, 100.f})},
				{"repeating", Variant(false)},
			},
		},
		TestCase{
			"repeating-linear-gradient(to right, #000, #fff)",
			"linear-gradient",
			Dictionary{
				{"length", Variant(200.f)},
				{"p0", Variant(Vector2f{0.f, 100.f})},
				{"p1", Variant(Vector2f{200.f, 100.f})},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0, Unit::NUMBER}},
						ColorStop{{255, 255, 255, 255}, {1, Unit::NUMBER}},
					})},
				{"repeating", Variant(true)},
			},
		},
		TestCase{
			"linear-gradient(to right, #000, rgba( 150, 150, 150, 255 ) 25%, #f00)",
			"linear-gradient",
			Dictionary{
				{"length", Variant(200.f)},
				{"p0", Variant(Vector2f{0.f, 100.f})},
				{"p1", Variant(Vector2f{200.f, 100.f})},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0, Unit::NUMBER}},
						ColorStop{{150, 150, 150, 255}, {0.25f, Unit::NUMBER}},
						ColorStop{{255, 0, 0, 255}, {1, Unit::NUMBER}},
					})},
				{"repeating", Variant(false)},
			},
		},
		TestCase{
			"linear-gradient(to right, #000, #fff 50px)",
			"linear-gradient",
			Dictionary{
				{"length", Variant(200.f)},
				{"p0", Variant(Vector2f{0.f, 100.f})},
				{"p1", Variant(Vector2f{200.f, 100.f})},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0, Unit::NUMBER}},
						ColorStop{{255, 255, 255, 255}, {0.25f, Unit::NUMBER}},
					})},
				{"repeating", Variant(false)},
			},
		},
		TestCase{
			"linear-gradient(to right, #000, #f00 100px 75%, #fff)",
			"linear-gradient",
			Dictionary{
				{"length", Variant(200.f)},
				{"p0", Variant(Vector2f{0.f, 100.f})},
				{"p1", Variant(Vector2f{200.f, 100.f})},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0, Unit::NUMBER}},
						ColorStop{{255, 0, 0, 255}, {0.5f, Unit::NUMBER}},
						ColorStop{{255, 0, 0, 255}, {0.75f, Unit::NUMBER}},
						ColorStop{{255, 255, 255, 255}, {1, Unit::NUMBER}},
					})},
				{"repeating", Variant(false)},
			},
		},
		TestCase{
			"linear-gradient(to right, #000, #fff) content-box",
			"linear-gradient",
			Dictionary{
				{"length", Variant(100.f)},
				{"p0", Variant(Vector2f{0.f, 50.f})},
				{"p1", Variant(Vector2f{100.f, 50.f})},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0, Unit::NUMBER}},
						ColorStop{{255, 255, 255, 255}, {1, Unit::NUMBER}},
					})},
				{"repeating", Variant(false)},
			},
		},
		TestCase{
			"linear-gradient(0deg, #000, #fff)",
			"linear-gradient",
			Dictionary{
				{"length", Variant(200.f)},
				{"p0", Variant(Vector2f{100.f, 200.f})},
				{"p1", Variant(Vector2f{100.f, 0.f})},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0, Unit::NUMBER}},
						ColorStop{{255, 255, 255, 255}, {1, Unit::NUMBER}},
					})},
				{"repeating", Variant(false)},
			},
		},
		TestCase{
			"linear-gradient(#000, #fff)",
			"linear-gradient",
			Dictionary{
				{"length", Variant(200.f)},
				{"p0", Variant(Vector2f{100.f, 0.f})},
				{"p1", Variant(Vector2f{100.f, 200.f})},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0, Unit::NUMBER}},
						ColorStop{{255, 255, 255, 255}, {1, Unit::NUMBER}},
					})},
				{"repeating", Variant(false)},
			},
		},
		TestCase{
			"linear-gradient(to top right, #000, #fff)",
			"linear-gradient",
			Dictionary{
				{"length", Variant(282.843f)},
				{"p0", Variant(Vector2f{0.f, 200.f})},
				{"p1", Variant(Vector2f{200.f, 0.f})},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0, Unit::NUMBER}},
						ColorStop{{255, 255, 255, 255}, {1, Unit::NUMBER}},
					})},
				{"repeating", Variant(false)},
			},
		},

		// -- radial-gradient --
		TestCase{
			"radial-gradient(#000, #fff)",
			"radial-gradient",
			Dictionary{
				{"center", Variant(Vector2f{100.f, 100.f})},
				{"radius", Variant(Vector2f{Math::SquareRoot(2.f) * 100.f})},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0, Unit::NUMBER}},
						ColorStop{{255, 255, 255, 255}, {1, Unit::NUMBER}},
					})},
				{"repeating", Variant(false)},
			},
		},
		TestCase{
			"repeating-radial-gradient(#000, #fff)",
			"radial-gradient",
			Dictionary{
				{"center", Variant(Vector2f{100.f, 100.f})},
				{"radius", Variant(Vector2f{Math::SquareRoot(2.f) * 100.f})},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0, Unit::NUMBER}},
						ColorStop{{255, 255, 255, 255}, {1, Unit::NUMBER}},
					})},
				{"repeating", Variant(true)},
			},
		},
		TestCase{
			"radial-gradient(closest-side, #000, #fff)",
			"radial-gradient",
			Dictionary{
				{"center", Variant(Vector2f{100.f, 100.f})},
				{"radius", Variant(Vector2f{100.f})},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0, Unit::NUMBER}},
						ColorStop{{255, 255, 255, 255}, {1, Unit::NUMBER}},
					})},
				{"repeating", Variant(false)},
			},
		},
		TestCase{
			"radial-gradient(closest-side, #000 25px 50%, #fff)",
			"radial-gradient",
			Dictionary{
				{"center", Variant(Vector2f{100.f, 100.f})},
				{"radius", Variant(Vector2f{100.f})},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0.25f, Unit::NUMBER}},
						ColorStop{{0, 0, 0, 255}, {0.5f, Unit::NUMBER}},
						ColorStop{{255, 255, 255, 255}, {1, Unit::NUMBER}},
					})},
				{"repeating", Variant(false)},
			},
		},
		TestCase{
			"radial-gradient(circle closest-side at 75% 50%, #000, #fff)",
			"radial-gradient",
			Dictionary{
				{"center", Variant(Vector2f{150.f, 100.f})},
				{"radius", Variant(Vector2f{50.f})},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0, Unit::NUMBER}},
						ColorStop{{255, 255, 255, 255}, {1, Unit::NUMBER}},
					})},
				{"repeating", Variant(false)},
			},
		},
		TestCase{
			"radial-gradient(ellipse closest-side at 75% 50%, #000, #fff)",
			"radial-gradient",
			Dictionary{
				{"center", Variant(Vector2f{150.f, 100.f})},
				{"radius", Variant(Vector2f{50.f, 100.f})},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0, Unit::NUMBER}},
						ColorStop{{255, 255, 255, 255}, {1, Unit::NUMBER}},
					})},
				{"repeating", Variant(false)},
			},
		},
		TestCase{
			"radial-gradient(circle farthest-side at 150px 100px, #000, #fff)",
			"radial-gradient",
			Dictionary{
				{"center", Variant(Vector2f{150.f, 100.f})},
				{"radius", Variant(Vector2f{150.f})},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0, Unit::NUMBER}},
						ColorStop{{255, 255, 255, 255}, {1, Unit::NUMBER}},
					})},
				{"repeating", Variant(false)},
			},
		},
		TestCase{
			"radial-gradient(farthest-side at 150px 100px, #000, #fff)",
			"radial-gradient",
			Dictionary{
				{"center", Variant(Vector2f{150.f, 100.f})},
				{"radius", Variant(Vector2f{150.f, 100.f})},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0, Unit::NUMBER}},
						ColorStop{{255, 255, 255, 255}, {1, Unit::NUMBER}},
					})},
				{"repeating", Variant(false)},
			},
		},
		TestCase{
			"radial-gradient(farthest-corner at right, #000, #fff)",
			"radial-gradient",
			Dictionary{
				{"center", Variant(Vector2f{200.f, 100.f})},
				{"radius", Variant(Math::SquareRoot(2.f) * Vector2f{200.f, 100.f})},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0, Unit::NUMBER}},
						ColorStop{{255, 255, 255, 255}, {1, Unit::NUMBER}},
					})},
				{"repeating", Variant(false)},
			},
		},
		TestCase{
			"radial-gradient(50px at top right, #000, #fff)",
			"radial-gradient",
			Dictionary{
				{"center", Variant(Vector2f{200.f, 0.f})},
				{"radius", Variant(Vector2f{50.f})},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0, Unit::NUMBER}},
						ColorStop{{255, 255, 255, 255}, {1, Unit::NUMBER}},
					})},
				{"repeating", Variant(false)},
			},
		},
		TestCase{
			"radial-gradient(50% 25% at bottom left, #000, #fff)",
			"radial-gradient",
			Dictionary{
				{"center", Variant(Vector2f{0.f, 200.f})},
				{"radius", Variant(Vector2f{100.f, 50.f})},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0, Unit::NUMBER}},
						ColorStop{{255, 255, 255, 255}, {1, Unit::NUMBER}},
					})},
				{"repeating", Variant(false)},
			},
		},

		// -- conic-gradient --
		TestCase{
			"conic-gradient(#000, #fff)",
			"conic-gradient",
			Dictionary{
				{"center", Variant(Vector2f{100.f, 100.f})},
				{"angle", Variant(0.f)},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0, Unit::NUMBER}},
						ColorStop{{255, 255, 255, 255}, {1, Unit::NUMBER}},
					})},
				{"repeating", Variant(false)},
			},
		},
		TestCase{
			"repeating-conic-gradient(#000, #fff)",
			"conic-gradient",
			Dictionary{
				{"center", Variant(Vector2f{100.f, 100.f})},
				{"angle", Variant(0.f)},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0, Unit::NUMBER}},
						ColorStop{{255, 255, 255, 255}, {1, Unit::NUMBER}},
					})},
				{"repeating", Variant(true)},
			},
		},
		TestCase{
			"conic-gradient(#000 50%, #fff)",
			"conic-gradient",
			Dictionary{
				{"center", Variant(Vector2f{100.f, 100.f})},
				{"angle", Variant(0.f)},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0.5f, Unit::NUMBER}},
						ColorStop{{255, 255, 255, 255}, {1, Unit::NUMBER}},
					})},
				{"repeating", Variant(false)},
			},
		},
		TestCase{
			"conic-gradient(#000 50% 270deg, #fff)",
			"conic-gradient",
			Dictionary{
				{"center", Variant(Vector2f{100.f, 100.f})},
				{"angle", Variant(0.f)},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0.5f, Unit::NUMBER}},
						ColorStop{{0, 0, 0, 255}, {0.75f, Unit::NUMBER}},
						ColorStop{{255, 255, 255, 255}, {1, Unit::NUMBER}},
					})},
				{"repeating", Variant(false)},
			},
		},
		TestCase{
			"conic-gradient(from 90deg, #000, #fff)",
			"conic-gradient",
			Dictionary{
				{"center", Variant(Vector2f{100.f, 100.f})},
				{"angle", Variant(1.5708f)},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0, Unit::NUMBER}},
						ColorStop{{255, 255, 255, 255}, {1, Unit::NUMBER}},
					})},
				{"repeating", Variant(false)},
			},
		},
		TestCase{
			"conic-gradient(from 90deg at bottom right, #000, #fff)",
			"conic-gradient",
			Dictionary{
				{"center", Variant(Vector2f{200.f, 200.f})},
				{"angle", Variant(1.5708f)},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0, Unit::NUMBER}},
						ColorStop{{255, 255, 255, 255}, {1, Unit::NUMBER}},
					})},
				{"repeating", Variant(false)},
			},
		},
		TestCase{
			"conic-gradient(at 180px 25%, #000, #fff)",
			"conic-gradient",
			Dictionary{
				{"center", Variant(Vector2f{180.f, 50.f})},
				{"angle", Variant(0.f)},
				{"color_stop_list",
					Variant(ColorStopList{
						ColorStop{{0, 0, 0, 255}, {0, Unit::NUMBER}},
						ColorStop{{255, 255, 255, 255}, {1, Unit::NUMBER}},
					})},
				{"repeating", Variant(false)},
			},
		},

		// -- shader --
		TestCase{
			"shader(cake)",
			"shader",
			Dictionary{
				{"value", Variant("cake")},
				{"dimensions", Variant(Vector2f{200, 200})},
			},
		},
		TestCase{
			"shader(animated_radar) content-box",
			"shader",
			Dictionary{
				{"value", Variant("animated_radar")},
				{"dimensions", Variant(Vector2f{100, 100})},
			},
		},
		TestCase{
			"shader(\"taco party\") border-box",
			"shader",
			Dictionary{
				{"value", Variant("taco party")},
				{"dimensions", Variant(Vector2f{220, 220})},
			},
		},
	};

	ElementDocument* document = context->LoadDocumentFromMemory(document_gradients_rml);
	REQUIRE(document);
	auto div = document->GetChild(0);
	REQUIRE(div);

	document->Show();

	CompiledShaderHandle compiled_shader_handle = {1};
	CompiledGeometryHandle compiled_geometry_handle = {1001};

	for (const TestCase& test_case : test_cases)
	{
		compiled_shader_handle += 1;
		compiled_geometry_handle += 1;
		CAPTURE(test_case.value);

		REQUIRE_CALL(mockRenderInterface, CompileGeometry(tl::_, tl::_)).RETURN(compiled_geometry_handle);
		REQUIRE_CALL(mockRenderInterface, ReleaseGeometry(compiled_geometry_handle));

		REQUIRE_CALL(mockRenderInterface, CompileShader(test_case.expected_name, tl::_))
			.WITH(DictionaryApproximateMatch(_2, test_case.expected_dictionary))
			.RETURN(compiled_shader_handle);
		REQUIRE_CALL(mockRenderInterface, RenderShader(compiled_shader_handle, compiled_geometry_handle, tl::_, TextureHandle{}));
		REQUIRE_CALL(mockRenderInterface, ReleaseShader(compiled_shader_handle));

		div->SetProperty("decorator", test_case.value);
		TestsShell::RenderLoop();

		div->SetProperty("decorator", "none");
		context->Update();
		context->Render();
	}

	document->Close();

	TestsShell::ShutdownShell();
}
