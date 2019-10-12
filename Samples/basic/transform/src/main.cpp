/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2014 Markus Schöngart
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
#include <RmlUi/Controls.h>
#include <RmlUi/Debugger.h>
#include <Input.h>
#include <Shell.h>
#include <ShellRenderInterfaceOpenGL.h>

#include <cmath>
#include <sstream>

static bool run_rotate = true;

class DemoWindow : public Rml::Core::EventListener
{
public:
	DemoWindow(const Rml::Core::String &title, const Rml::Core::Vector2f &position, Rml::Core::Context *context)
	{
		document = context->LoadDocument("basic/transform/data/transform.rml");
		if (document)
		{
			document->GetElementById("title")->SetInnerRML(title);
			document->SetProperty(Rml::Core::PropertyId::Left, Rml::Core::Property(position.x, Rml::Core::Property::PX));
			document->SetProperty(Rml::Core::PropertyId::Top, Rml::Core::Property(position.y, Rml::Core::Property::PX));
			document->Show();
		}
	}

	~DemoWindow()
	{
		if (document)
			document->Close();
	}

	void SetPerspective(float distance)
	{
		perspective = distance;

		if (document && perspective > 0)
		{
			std::stringstream s;
			s << "perspective(" << perspective << "px) ";
			document->SetProperty("transform", s.str().c_str());
		}
	}

	void SetRotation(float degrees)
	{
		if(document)
		{
			std::stringstream s;
			if (perspective > 0)
				s << "perspective(" << perspective << "px) ";
			s << "rotate3d(0.0, 1.0, 0.0, " << degrees << "deg)";
			document->SetProperty("transform", s.str().c_str());
		}
	}

	void ProcessEvent(Rml::Core::Event& ev) override
	{
		if (ev == Rml::Core::EventId::Keydown)
		{
			Rml::Core::Input::KeyIdentifier key_identifier = (Rml::Core::Input::KeyIdentifier) ev.GetParameter< int >("key_identifier", 0);

			if (key_identifier == Rml::Core::Input::KI_SPACE)
			{
				run_rotate = !run_rotate;
			}
			else if (key_identifier == Rml::Core::Input::KI_ESCAPE)
			{
				Shell::RequestExit();
			}
		}
	}

private:
	float perspective = 0;
	Rml::Core::ElementDocument *document;
};

Rml::Core::Context* context = nullptr;
ShellRenderInterfaceExtensions* shell_renderer;
DemoWindow* window_1 = nullptr;
DemoWindow* window_2 = nullptr;

void GameLoop()
{
	context->Update();

	shell_renderer->PrepareRenderBuffer();
	context->Render();
	shell_renderer->PresentRenderBuffer();

	double t = Rml::Core::GetSystemInterface()->GetElapsedTime();
	static double t_prev = t;
	double dt = t - t_prev;
	t_prev = t;

	if(run_rotate)
	{
		static float deg = 0;
		deg = (float)std::fmod(deg + dt * 50.0, 360.0);
		if (window_1)
			window_1->SetRotation(deg);
		if (window_2)
			window_2->SetRotation(deg);
	}
}

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

	constexpr int width = 1600;
	constexpr int height = 950;

	ShellRenderInterfaceOpenGL opengl_renderer;
	shell_renderer = &opengl_renderer;

	// Generic OS initialisation, creates a window and attaches OpenGL.
	if (!Shell::Initialise() ||
		!Shell::OpenWindow("Transform Sample", shell_renderer, width, height, true))
	{
		Shell::Shutdown();
		return -1;
	}

	// RmlUi initialisation.
	Rml::Core::SetRenderInterface(&opengl_renderer);
	opengl_renderer.SetViewport(width, height);

	ShellSystemInterface system_interface;
	Rml::Core::SetSystemInterface(&system_interface);

	Rml::Core::Initialise();

	// Create the main RmlUi context and set it on the shell's input layer.
	context = Rml::Core::CreateContext("main", Rml::Core::Vector2i(width, height));
	if (context == nullptr)
	{
		Rml::Core::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	Rml::Controls::Initialise();
	Rml::Debugger::Initialise(context);
	Input::SetContext(context);
	shell_renderer->SetContext(context);

	Shell::LoadFonts("assets/");

	window_1 = new DemoWindow("Orthographic transform", Rml::Core::Vector2f(120, 180), context);
	if (window_1)
	{
		context->GetRootElement()->AddEventListener(Rml::Core::EventId::Keydown, window_1);
	}
	window_2 = new DemoWindow("Perspective transform", Rml::Core::Vector2f(900, 180), context);
	if (window_2)
	{
		window_2->SetPerspective(800);
	}

	Shell::EventLoop(GameLoop);

	delete window_1;
	delete window_2;

	// Shutdown RmlUi.
	Rml::Core::Shutdown();

	Shell::CloseWindow();
	Shell::Shutdown();

	return 0;
}
