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
#include <RmlUi/Debugger.h>
#include <RmlUi_Backend.h>
#include <Shell.h>
#include <cmath>
#include <sstream>

static bool run_rotate = true;

class DemoWindow : public Rml::EventListener
{
public:
	DemoWindow(const Rml::String &title, const Rml::Vector2f &position, Rml::Context *context)
	{
		document = context->LoadDocument("basic/transform/data/transform.rml");
		if (document)
		{
			document->GetElementById("title")->SetInnerRML(title);
			document->SetProperty(Rml::PropertyId::Left, Rml::Property(position.x, Rml::Property::DP));
			document->SetProperty(Rml::PropertyId::Top, Rml::Property(position.y, Rml::Property::DP));
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
			s << "perspective(" << perspective << "dp) ";
			document->SetProperty("transform", s.str().c_str());
		}
	}

	void SetRotation(float degrees)
	{
		if(document)
		{
			std::stringstream s;
			if (perspective > 0)
				s << "perspective(" << perspective << "dp) ";
			s << "rotate3d(0.0, 1.0, 0.0, " << degrees << "deg)";
			document->SetProperty("transform", s.str().c_str());
		}
	}

	void ProcessEvent(Rml::Event& ev) override
	{
		if (ev == Rml::EventId::Keydown)
		{
			Rml::Input::KeyIdentifier key_identifier = (Rml::Input::KeyIdentifier) ev.GetParameter< int >("key_identifier", 0);

			if (key_identifier == Rml::Input::KI_SPACE)
			{
				run_rotate = !run_rotate;
			}
			else if (key_identifier == Rml::Input::KI_ESCAPE)
			{
				Backend::RequestExit();
			}
		}
	}

private:
	float perspective = 0;
	Rml::ElementDocument *document;
};

#if defined RMLUI_PLATFORM_WIN32
	#include <RmlUi_Include_Windows.h>
int APIENTRY WinMain(HINSTANCE /*instance_handle*/, HINSTANCE /*previous_instance_handle*/, char* /*command_line*/, int /*command_show*/)
#else
int main(int /*argc*/, char** /*argv*/)
#endif
{
	const int window_width = 1600;
	const int window_height = 950;

	// Initializes the shell which provides common functionality used by the included samples.
	if (!Shell::Initialize())
		return -1;

	// Constructs the system and render interfaces, creates a window, and attaches the renderer.
	if (!Backend::Initialize("Transform Sample", window_width, window_height, true))
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

	Rml::UniquePtr<DemoWindow> window_1 = Rml::MakeUnique<DemoWindow>("Orthographic transform", Rml::Vector2f(120, 180), context);
	if (window_1)
		context->GetRootElement()->AddEventListener(Rml::EventId::Keydown, window_1.get());

	Rml::UniquePtr<DemoWindow> window_2 = Rml::MakeUnique<DemoWindow>("Perspective transform", Rml::Vector2f(900, 180), context);
	if (window_2)
		window_2->SetPerspective(800);

	bool running = true;
	while (running)
	{
		running = Backend::ProcessEvents(context, &Shell::ProcessKeyDownShortcuts);

		context->Update();

		Backend::BeginFrame();
		context->Render();
		Backend::PresentFrame();

		double t = Rml::GetSystemInterface()->GetElapsedTime();
		static double t_prev = t;
		double dt = t - t_prev;
		t_prev = t;

		if (run_rotate)
		{
			static float deg = 0;
			deg = (float)std::fmod(deg + dt * 50.0, 360.0);
			if (window_1)
				window_1->SetRotation(deg);
			if (window_2)
				window_2->SetRotation(deg);
		}
	}

	if (window_1)
		context->GetRootElement()->RemoveEventListener(Rml::EventId::Keydown, window_1.get());

	window_1.reset();
	window_2.reset();

	// Shutdown RmlUi.
	Rml::Shutdown();
	
	Backend::Shutdown();
	Shell::Shutdown();

	return 0;
}
