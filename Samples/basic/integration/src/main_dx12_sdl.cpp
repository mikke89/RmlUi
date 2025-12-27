#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include <RmlUi_Backend.h>
#include <Shell.h>

#if defined RMLUI_PLATFORM_WIN32
	#include <RmlUi_Include_Windows.h>
int APIENTRY WinMain(HINSTANCE /*instance_handle*/, HINSTANCE /*previous_instance_handle*/, char* /*command_line*/, int /*command_show*/)
#else
int main(int /*argc*/, char** /*argv*/)
#endif
{
	const int window_width = 1024;
	const int window_height = 768;

	// Initializes the shell which provides common functionality used by the included samples.
	if (!Shell::Initialize())
		return -1;

	// Constructs the system and render interfaces, creates a window, and attaches the renderer.
	if (!Backend::Initialize("Effects Sample", window_width, window_height, true))
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
	if (!context)
	{
		Rml::Shutdown();
		Backend::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	Rml::Debugger::Initialise(context);
	Shell::LoadFonts();

	static constexpr float perspective_max = 3000.f;

	struct EffectsData {
		bool show_menu = false;
		Rml::String submenu = "filter";

		struct Filter {
			float opacity = 1.0f;
			float sepia = 0.0f;
			float grayscale = 0.0f;
			float saturate = 1.0f;
			float brightness = 1.0f;
			float contrast = 1.0f;
			float hue_rotate = 0.0f;
			float invert = 0.0f;
			float blur = 0.0f;
			bool drop_shadow = false;
		} filter;

		struct Transform {
			float scale = 1.0f;
			Rml::Vector3f rotate;
			float perspective = perspective_max;
			Rml::Vector2f perspective_origin = Rml::Vector2f(50.f);
			bool transform_all = false;
		} transform;
	} data;

	if (Rml::DataModelConstructor constructor = context->CreateDataModel("effects"))
	{
		constructor.Bind("show_menu", &data.show_menu);
		constructor.Bind("submenu", &data.submenu);

		constructor.Bind("opacity", &data.filter.opacity);
		constructor.Bind("sepia", &data.filter.sepia);
		constructor.Bind("grayscale", &data.filter.grayscale);
		constructor.Bind("saturate", &data.filter.saturate);
		constructor.Bind("brightness", &data.filter.brightness);
		constructor.Bind("contrast", &data.filter.contrast);
		constructor.Bind("hue_rotate", &data.filter.hue_rotate);
		constructor.Bind("invert", &data.filter.invert);
		constructor.Bind("blur", &data.filter.blur);
		constructor.Bind("drop_shadow", &data.filter.drop_shadow);

		constructor.Bind("scale", &data.transform.scale);
		constructor.Bind("rotate_x", &data.transform.rotate.x);
		constructor.Bind("rotate_y", &data.transform.rotate.y);
		constructor.Bind("rotate_z", &data.transform.rotate.z);
		constructor.Bind("perspective", &data.transform.perspective);
		constructor.Bind("perspective_origin_x", &data.transform.perspective_origin.x);
		constructor.Bind("perspective_origin_y", &data.transform.perspective_origin.y);
		constructor.Bind("transform_all", &data.transform.transform_all);

		constructor.BindEventCallback("reset", [&data](Rml::DataModelHandle handle, Rml::Event& /*ev*/, const Rml::VariantList& /*arguments*/) {
			if (data.submenu == "transform")
				data.transform = EffectsData::Transform{};
			else if (data.submenu == "filter")
				data.filter = EffectsData::Filter{};
			handle.DirtyAllVariables();
		});
	}

	if (Rml::ElementDocument* document = context->LoadDocument("basic/effects/data/effects.rml"))
		document->Show();

	bool running = true;
	while (running)
	{
		running = Backend::ProcessEvents(context, &Shell::ProcessKeyDownShortcuts, false);

		context->Update();

		Backend::BeginFrame();
		context->Render();
		Backend::PresentFrame();
	}

	Rml::Shutdown();

	Backend::Shutdown();
	Shell::Shutdown();

	return 0;
}
