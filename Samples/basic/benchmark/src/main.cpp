#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include <RmlUi_Backend.h>
#include <Shell.h>

class DemoWindow {
public:
	DemoWindow(const Rml::String& title, Rml::Context* context)
	{
		using namespace Rml;
		document = context->LoadDocument("basic/benchmark/data/benchmark.rml");
		if (document != nullptr)
		{
			{
				document->GetElementById("title")->SetInnerRML(title);
			}

			document->Show();
		}
	}

	void performance_test()
	{
		RMLUI_ZoneScoped;

		if (!document)
			return;

		Rml::String rml;

		for (int i = 0; i < 50; i++)
		{
			int index = rand() % 1000;
			int route = rand() % 50;
			int max = (rand() % 40) + 10;
			int value = rand() % max;
			Rml::String rml_row = Rml::CreateString(R"(
			<div class="row">
				<div class="col col1"><button class="expand" index="%d">+</button>&nbsp;<a>Route %d</a></div>
				<div class="col col23"><input type="range" class="assign_range" min="0" max="%d" value="%d"/></div>
				<div class="col col4">Assigned</div>
				<select>
					<option>Red</option><option>Blue</option><option selected>Green</option><option style="background-color: yellow;">Yellow</option>
				</select>
				<div class="inrow unmark_collapse">
					<div class="col col123 assign_text">Assign to route</div>
					<div class="col col4">
						<input type="submit" class="vehicle_depot_assign_confirm" quantity="0">Confirm</input>
					</div>
				</div>
			</div>)",
				index, route, max, value);
			rml += rml_row;
		}

		if (auto el = document->GetElementById("performance"))
			el->SetInnerRML(rml);
	}

	class SimpleEventListener : public Rml::EventListener {
	public:
		void ProcessEvent(Rml::Event& event) override
		{
			static int i = 0;
			event.GetTargetElement()->SetProperty("background-color", i++ % 2 == 0 ? "green" : "orange");
		}
	} simple_event_listener;

	~DemoWindow()
	{
		if (document)
		{
			document->Close();
		}
	}

	Rml::ElementDocument* GetDocument() { return document; }

private:
	Rml::ElementDocument* document;
};

bool run_loop = true;
bool single_loop = true;
bool run_update = true;
bool single_update = true;

class Event : public Rml::EventListener {
public:
	Event(const Rml::String& value) : value(value) {}

	void ProcessEvent(Rml::Event& event) override
	{
		using namespace Rml;

		if (value == "exit")
			Backend::RequestExit();

		if (event == "keydown")
		{
			auto key_identifier = (Rml::Input::KeyIdentifier)event.GetParameter<int>("key_identifier", 0);

			if (key_identifier == Rml::Input::KI_SPACE)
			{
				run_loop = !run_loop;
			}
			else if (key_identifier == Rml::Input::KI_DOWN)
			{
				run_loop = false;
				single_loop = true;
			}
			else if (key_identifier == Rml::Input::KI_RIGHT)
			{
				run_update = false;
				single_update = true;
			}
			else if (key_identifier == Rml::Input::KI_RETURN)
			{
				run_update = !run_update;
			}
			else if (key_identifier == Rml::Input::KI_ESCAPE)
			{
				Backend::RequestExit();
			}
		}
	}

	void OnDetach(Rml::Element* /*element*/) override { delete this; }

private:
	Rml::String value;
};

class EventInstancer : public Rml::EventListenerInstancer {
public:
	/// Instances a new event handle for Invaders.
	Rml::EventListener* InstanceEventListener(const Rml::String& value, Rml::Element* /*element*/) override { return new Event(value); }
};

#if defined RMLUI_PLATFORM_WIN32
	#include <RmlUi_Include_Windows.h>
int APIENTRY WinMain(HINSTANCE /*instance_handle*/, HINSTANCE /*previous_instance_handle*/, char* /*command_line*/, int /*command_show*/)
#else
int main(int /*argc*/, char** /*argv*/)
#endif
{
	const int window_width = 1800;
	const int window_height = 1000;

	// Initializes the shell which provides common functionality used by the included samples.
	if (!Shell::Initialize())
		return -1;

	// Constructs the system and render interfaces, creates a window, and attaches the renderer.
	if (!Backend::Initialize("Benchmark Sample", window_width, window_height, true))
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

	EventInstancer event_listener_instancer;
	Rml::Factory::RegisterEventListenerInstancer(&event_listener_instancer);

	Shell::LoadFonts();

	Rml::UniquePtr<DemoWindow> window = Rml::MakeUnique<DemoWindow>("Benchmark sample", context);
	window->GetDocument()->AddEventListener(Rml::EventId::Keydown, new Event("hello"));
	window->GetDocument()->AddEventListener(Rml::EventId::Keyup, new Event("hello"));
	window->GetDocument()->AddEventListener(Rml::EventId::Animationend, new Event("hello"));

	bool running = true;
	while (running)
	{
		running = Backend::ProcessEvents(context, &Shell::ProcessKeyDownShortcuts);

		if (run_update || single_update)
		{
			single_update = false;

			window->performance_test();
		}

		if (run_loop || single_loop)
		{
			single_loop = false;

			context->Update();

			Backend::BeginFrame();
			context->Render();
			Backend::PresentFrame();
		}

		static constexpr int buffer_size = 200;
		static float fps_buffer[buffer_size] = {};
		static int buffer_index = 0;

		static double t_prev = 0.0f;
		double t = Rml::GetSystemInterface()->GetElapsedTime();
		float dt = float(t - t_prev);
		t_prev = t;
		static int count_frames = 0;
		count_frames += 1;

		float fps = 1.0f / dt;
		fps_buffer[buffer_index] = fps;
		buffer_index = ((buffer_index + 1) % buffer_size);

		if (window && count_frames > buffer_size / 8)
		{
			float fps_mean = 0;
			for (int i = 0; i < buffer_size; i++)
				fps_mean += fps_buffer[(buffer_index + i) % buffer_size];
			fps_mean = fps_mean / (float)buffer_size;

			auto el = window->GetDocument()->GetElementById("fps");
			count_frames = 0;
			el->SetInnerRML(Rml::CreateString("FPS: %f", fps_mean));
		}
	}

	window.reset();

	// Shutdown RmlUi.
	Rml::Shutdown();

	Backend::Shutdown();
	Shell::Shutdown();

	return 0;
}
