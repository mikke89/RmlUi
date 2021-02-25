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

#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include <Input.h>
#include <Shell.h>
#include <ShellRenderInterfaceOpenGL.h>


class DemoWindow
{
public:
	DemoWindow(const Rml::String &title, Rml::Context *context)
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
			Rml::String rml_row = Rml::CreateString(1000, R"(
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
				index, 
				route,
				max,
				value
			);
			rml += rml_row;
		}

		if (auto el = document->GetElementById("performance"))
			el->SetInnerRML(rml);
	}

	class SimpleEventListener : public Rml::EventListener {
	public:
		void ProcessEvent(Rml::Event& event) override {
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

	Rml::ElementDocument * GetDocument() {
		return document;
	}

private:
	Rml::ElementDocument *document;
};


Rml::Context* context = nullptr;
ShellRenderInterfaceExtensions *shell_renderer;
DemoWindow* window = nullptr;

bool run_loop = true;
bool single_loop = true;
bool run_update = true;
bool single_update = true;

void GameLoop()
{
	if (run_update || single_update)
	{
		single_update = false;

		window->performance_test();
	}

	if (run_loop || single_loop)
	{
		single_loop = false;
	
		context->Update();

		shell_renderer->PrepareRenderBuffer();
		context->Render();
		shell_renderer->PresentRenderBuffer();
	}

	static constexpr int buffer_size = 200;
	static float fps_buffer[buffer_size] = {};
	static int buffer_index = 0;

	static double t_prev = 0.0f;
	double t = Shell::GetElapsedTime();
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
		el->SetInnerRML(Rml::CreateString(20, "FPS: %f", fps_mean));
	}
}



class Event : public Rml::EventListener
{
public:
	Event(const Rml::String& value) : value(value) {}

	void ProcessEvent(Rml::Event& event) override
	{
		using namespace Rml;

		if(value == "exit")
			Shell::RequestExit();

		if (event == "keydown")
		{
			auto key_identifier = (Rml::Input::KeyIdentifier)event.GetParameter< int >("key_identifier", 0);

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
				Shell::RequestExit();
			}
		}
	}

	void OnDetach(Rml::Element* /*element*/) override { delete this; }

private:
	Rml::String value;
};


class EventInstancer : public Rml::EventListenerInstancer
{
public:

	/// Instances a new event handle for Invaders.
	Rml::EventListener* InstanceEventListener(const Rml::String& value, Rml::Element* /*element*/) override
	{
		return new Event(value);
	}
};


#if defined RMLUI_PLATFORM_WIN32
#include <windows.h>
int APIENTRY WinMain(HINSTANCE RMLUI_UNUSED_PARAMETER(instance_handle), HINSTANCE RMLUI_UNUSED_PARAMETER(previous_instance_handle), char* RMLUI_UNUSED_PARAMETER(command_line), int RMLUI_UNUSED_PARAMETER(command_show))
#else
int main(int RMLUI_UNUSED_PARAMETER(argc), char** RMLUI_UNUSED_PARAMETER(argv))
#endif
{
#ifdef RMLUI_PLATFORM_WIN32
	RMLUI_UNUSED(instance_handle);
	RMLUI_UNUSED(previous_instance_handle);
	RMLUI_UNUSED(command_line);
	RMLUI_UNUSED(command_show);
#else
	RMLUI_UNUSED(argc);
	RMLUI_UNUSED(argv);
#endif

	const int width = 1800;
	const int height = 1000;

	ShellRenderInterfaceOpenGL opengl_renderer;
	shell_renderer = &opengl_renderer;

	// Generic OS initialisation, creates a window and attaches OpenGL.
	if (!Shell::Initialise() ||
		!Shell::OpenWindow("Benchmark Sample", shell_renderer, width, height, true))
	{
		Shell::Shutdown();
		return -1;
	}

	// RmlUi initialisation.
	Rml::SetRenderInterface(&opengl_renderer);
	opengl_renderer.SetViewport(width, height);

	ShellSystemInterface system_interface;
	Rml::SetSystemInterface(&system_interface);

	Rml::Initialise();

	// Create the main RmlUi context and set it on the shell's input layer.
	context = Rml::CreateContext("main", Rml::Vector2i(width, height));
	if (context == nullptr)
	{
		Rml::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	Rml::Debugger::Initialise(context);
	Input::SetContext(context);
	Shell::SetContext(context);

	EventInstancer event_listener_instancer;
	Rml::Factory::RegisterEventListenerInstancer(&event_listener_instancer);

	Shell::LoadFonts("assets/");

	window = new DemoWindow("Benchmark sample", context);
	window->GetDocument()->AddEventListener(Rml::EventId::Keydown, new Event("hello"));
	window->GetDocument()->AddEventListener(Rml::EventId::Keyup, new Event("hello"));
	window->GetDocument()->AddEventListener(Rml::EventId::Animationend, new Event("hello"));


	Shell::EventLoop(GameLoop);

	delete window;

	// Shutdown RmlUi.
	Rml::Shutdown();

	Shell::CloseWindow();
	Shell::Shutdown();

	return 0;
}
