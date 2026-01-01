#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include <RmlUi_Backend.h>
#include <Shell.h>

struct SVGToogleStruct {
	Rml::String svg_data = R"(<circle cx="25" cy="25" r="20" stroke="black" stroke-width="3" fill="red" />)";
	Rml::String line_color = "black";
	Rml::String fill_color = "red";
	Rml::Element* svg_element;
	bool toggle_state = false;
	void Toggle(Rml::DataModelHandle model, Rml::Event& /*event*/, const Rml::VariantList& /*args*/)
	{
		toggle_state = !toggle_state;

		// This example uses 3 methods of setting inline SVG data
		// - Used to set the svg data via data-rml with the main svg data in the data-rml attribute but concatenating colour variables from the model
		line_color = toggle_state ? "yellow" : "black";
		fill_color = toggle_state ? "green" : "red";

		// - Used to set the svg data via data-rml with all the svg data contained within a model property
		svg_data = R"(<circle cx="25" cy="25" r="20" stroke=")" + line_color + R"(" stroke-width="3" fill=")" + fill_color + R"(" />)";

		// - Using SetInnerRML directly on an SVG element to change the SVG data
		if (svg_element)
			svg_element->SetInnerRML(svg_data);

		model.DirtyVariable("svg_data");
		model.DirtyVariable("line_color");
		model.DirtyVariable("fill_color");
	}
} toggle_model;

#if defined RMLUI_PLATFORM_WIN32
	#include <RmlUi_Include_Windows.h>
int APIENTRY WinMain(HINSTANCE /*instance_handle*/, HINSTANCE /*previous_instance_handle*/, char* /*command_line*/, int /*command_show*/)
#else
int main(int /*argc*/, char** /*argv*/)
#endif
{
	int window_width = 1024;
	int window_height = 768;

	// Initializes the shell which provides common functionality used by the included samples.
	if (!Shell::Initialize())
		return -1;

	// Constructs the system and render interfaces, creates a window, and attaches the renderer.
	if (!Backend::Initialize("SVG sample", window_width, window_height, true))
	{
		Shell::Shutdown();
		return -1;
	}

	// Install the custom interfaces constructed by the backend before initializing RmlUi.
	Rml::SetSystemInterface(Backend::GetSystemInterface());
	Rml::SetRenderInterface(Backend::GetRenderInterface());

	// RmlUi initialisation.
	Rml::Initialise();

	// Create the main RmlUi context.
	Rml::Context* context = Rml::CreateContext("main", Rml::Vector2i(window_width, window_height));
	if (context == nullptr)
	{
		Rml::Shutdown();
		Backend::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	Rml::String svg_data;
	Rml::DataModelConstructor dm_con = context->CreateDataModel("svg_test_model");
	if (auto model_handle = dm_con.RegisterStruct<SVGToogleStruct>())
	{
		model_handle.RegisterMember("name", &SVGToogleStruct::svg_data);
		model_handle.RegisterMember("line_color", &SVGToogleStruct::line_color);
		model_handle.RegisterMember("fill_color", &SVGToogleStruct::fill_color);
		model_handle.RegisterMember("sprite", &SVGToogleStruct::toggle_state);
	}
	dm_con.Bind("svg_data", &toggle_model.svg_data);
	dm_con.Bind("line_color", &toggle_model.line_color);
	dm_con.Bind("fill_color", &toggle_model.fill_color);
	dm_con.BindEventCallback("toggle_svg", &SVGToogleStruct::Toggle, &toggle_model);

	Rml::Debugger::Initialise(context);
	Shell::LoadFonts();

	// Load and show the documents.
	std::vector<std::string> rml_docs = {"basic/svg/data/svg_element.rml", "basic/svg/data/svg_decorator.rml", "basic/svg/data/svg_inline.rml"};
	for (const auto& rml_doc : rml_docs)
	{
		if (Rml::ElementDocument* document = context->LoadDocument(rml_doc))
		{
			document->Show();
			document->GetElementById("title")->SetInnerRML(document->GetTitle());
			if (Rml::Element* svg_element = document->GetElementById("svg_1"))
				toggle_model.svg_element = svg_element;
		}
	}

	bool running = true;
	while (running)
	{
		running = Backend::ProcessEvents(context, &Shell::ProcessKeyDownShortcuts, true);

		context->Update();

		Backend::BeginFrame();
		context->Render();
		Backend::PresentFrame();
	}

	// Shutdown RmlUi.
	Rml::Shutdown();

	Backend::Shutdown();
	Shell::Shutdown();

	return 0;
}
