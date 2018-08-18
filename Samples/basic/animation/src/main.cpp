/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2018 Michael Ragazzon
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

#include <Rocket/Core.h>
#include <Rocket/Controls.h>
#include <Rocket/Debugger.h>
#include <Input.h>
#include <Shell.h>
#include <Rocket/Core/TransformPrimitive.h>

#include <cmath>
#include <sstream>


// Animations TODO:
//  - Jittering on slow margin-left updates
//  - Proper interpolation of full transform matrices (split into translate/rotate/skew/scale).
//  - Support interpolation of primitive derivatives without going to full matrices.
//  - Better error reporting when submitting invalid animations, check validity on add. Remove animation if invalid.
//  - Tweening
//  - RCSS support? Both @keyframes and transition, maybe.
//  - Profiling
//  - [offtopic] Improve performance of transform parser (hashtable)

class DemoWindow
{
public:
	DemoWindow(const Rocket::Core::String &title, const Rocket::Core::Vector2f &position, Rocket::Core::Context *context)
	{
		using namespace Rocket::Core;
		document = context->LoadDocument("basic/animation/data/animation.rml");
		if (document != NULL)
		{
			document->GetElementById("title")->SetInnerRML(title);
			document->SetProperty("left", Property(position.x, Property::PX));
			document->SetProperty("top", Property(position.y, Property::PX));
			document->Animate("opacity", Property(0.6f, Property::NUMBER), 0.5f, -1, true);

			auto el = document->GetElementById("high_score");
			el->Animate("margin-left", Property(0.f, Property::PX), 0.3f, 10, true);
			el->Animate("margin-left", Property(100.f, Property::PX), 0.6f);

			el = document->GetElementById("exit");
			el->Animate("margin-left", Property(100.f, Property::PX), 8.0f, -1, true);

			el = document->GetElementById("start_game");
			auto t0 = TransformRef{ new Transform };
			auto v0 = Transforms::NumericValue(100.f, Property::PERCENT);
			t0->AddPrimitive({ Transforms::Rotate2D{ &v0 } });
			//auto t1 = TransformRef{ new Transform };
			//auto v1 = Transforms::NumericValue(370.f, Property::DEG);
			//t1->AddPrimitive({ Transforms::Rotate2D{ &v1 } });
			PropertyDictionary pd;
			StyleSheetSpecification::ParsePropertyDeclaration(pd, "transform", "rotate(200%)");
			auto p = pd.GetProperty("transform");
			el->Animate("transform", *p, 1.3f, -1, true);
			//el->Animate("transform", Property(t0, Property::TRANSFORM), 1.3f, -1, true);

			el = document->GetElementById("help");
			el->Animate("image-color", Property(Colourb(128, 255, 255, 255), Property::COLOUR), 0.3f, -1, false);
			el->Animate("image-color", Property(Colourb(128, 128, 255, 255), Property::COLOUR), 0.3f);
			el->Animate("image-color", Property(Colourb(0, 128, 128, 255), Property::COLOUR), 0.3f);
			el->Animate("image-color", Property(Colourb(64, 128, 255, 0), Property::COLOUR), 0.9f);
			el->Animate("image-color", Property(Colourb(255, 255, 255, 255), Property::COLOUR), 0.3f);

			document->Show();
		}
	}

	~DemoWindow()
	{
		if (document)
		{
			document->RemoveReference();
			document->Close();
		}
	}

	Rocket::Core::ElementDocument * GetDocument() {
		return document;
	}

private:
	Rocket::Core::ElementDocument *document;
};


Rocket::Core::Context* context = NULL;
ShellRenderInterfaceExtensions *shell_renderer;
DemoWindow* window = NULL;

bool pause_loop = false;
bool single_loop = false;

void GameLoop()
{
	if(!pause_loop || single_loop)
	{
		context->Update();

		shell_renderer->PrepareRenderBuffer();
		context->Render();
		shell_renderer->PresentRenderBuffer();

		single_loop = false;
	}

	//auto el = window->GetDocument()->GetElementById("exit");
	//auto f = el->GetProperty<int>("margin-left");
	//static int f_prev = 0.0f;
	//int df = f - f_prev;
	//f_prev = f;
	//if(df != 0)
	//	Rocket::Core::Log::Message(Rocket::Core::Log::LT_INFO, "Animation f = %d,  df = %d", f, df);
}



class Event : public Rocket::Core::EventListener
{
public:
	Event(const Rocket::Core::String& value) : value(value) {}

	void ProcessEvent(Rocket::Core::Event& event) override
	{
		if(value == "exit")
			Shell::RequestExit();

		if (event == "keydown" ||
			event == "keyup")
		{
			bool key_down = event == "keydown";
			Rocket::Core::Input::KeyIdentifier key_identifier = (Rocket::Core::Input::KeyIdentifier) event.GetParameter< int >("key_identifier", 0);

			if (key_identifier == Rocket::Core::Input::KI_SPACE && key_down)
			{
				pause_loop = !pause_loop;
			}
			else if (key_identifier == Rocket::Core::Input::KI_RETURN && key_down)
			{
				pause_loop = true;
				single_loop = true;
			}
			else if (key_identifier == Rocket::Core::Input::KI_ESCAPE)
			{
				Shell::RequestExit();
			}
		}
	}

	void OnDetach(Rocket::Core::Element* element) override { delete this; }

private:
	Rocket::Core::String value;
};


class EventInstancer : public Rocket::Core::EventListenerInstancer
{
public:

	/// Instances a new event handle for Invaders.
	Rocket::Core::EventListener* InstanceEventListener(const Rocket::Core::String& value, Rocket::Core::Element* element) override
	{
		return new Event(value);
	}

	/// Destroys the instancer.
	void Release() override { delete this; }
};




#if defined ROCKET_PLATFORM_WIN32
#include <windows.h>
int APIENTRY WinMain(HINSTANCE ROCKET_UNUSED_PARAMETER(instance_handle), HINSTANCE ROCKET_UNUSED_PARAMETER(previous_instance_handle), char* ROCKET_UNUSED_PARAMETER(command_line), int ROCKET_UNUSED_PARAMETER(command_show))
#else
int main(int ROCKET_UNUSED_PARAMETER(argc), char** ROCKET_UNUSED_PARAMETER(argv))
#endif
{
#ifdef ROCKET_PLATFORM_WIN32
	ROCKET_UNUSED(instance_handle);
	ROCKET_UNUSED(previous_instance_handle);
	ROCKET_UNUSED(command_line);
	ROCKET_UNUSED(command_show);
#else
	ROCKET_UNUSED(argc);
	ROCKET_UNUSED(argv);
#endif

	ShellRenderInterfaceOpenGL opengl_renderer;
	shell_renderer = &opengl_renderer;

	// Generic OS initialisation, creates a window and attaches OpenGL.
	if (!Shell::Initialise("../../Samples/") ||
		!Shell::OpenWindow("Animation Sample", shell_renderer, 1024, 768, true))
	{
		Shell::Shutdown();
		return -1;
	}

	// Rocket initialisation.
	Rocket::Core::SetRenderInterface(&opengl_renderer);
	opengl_renderer.SetViewport(1024,768);

	ShellSystemInterface system_interface;
	Rocket::Core::SetSystemInterface(&system_interface);

	Rocket::Core::Initialise();

	// Create the main Rocket context and set it on the shell's input layer.
	context = Rocket::Core::CreateContext("main", Rocket::Core::Vector2i(1024, 768));
	if (context == NULL)
	{
		Rocket::Core::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	Rocket::Controls::Initialise();
	Rocket::Debugger::Initialise(context);
	Input::SetContext(context);
	shell_renderer->SetContext(context);


	EventInstancer* event_instancer = new EventInstancer();
	Rocket::Core::Factory::RegisterEventListenerInstancer(event_instancer);
	event_instancer->RemoveReference();

	Shell::LoadFonts("assets/");

	window = new DemoWindow("Animation sample", Rocket::Core::Vector2f(81, 200), context);
	window->GetDocument()->AddEventListener("keydown", new Event("hello"));
	window->GetDocument()->AddEventListener("keyup", new Event("hello"));


	Shell::EventLoop(GameLoop);

	delete window;

	// Shutdown Rocket.
	context->RemoveReference();
	Rocket::Core::Shutdown();

	Shell::CloseWindow();
	Shell::Shutdown();

	return 0;
}
