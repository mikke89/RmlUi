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
//  - Update transform animations / resolve keys again when parent box size changes.
//  - RCSS support? Both @keyframes and transition, maybe.
//  - Profiling
//  - [offtopic] Improve performance of transform parser (hashtable)
//  - [offtopic] Use double for absolute time, get and cache time for each render/update loop

class DemoWindow
{
public:
	DemoWindow(const Rocket::Core::String &title, const Rocket::Core::Vector2f &position, Rocket::Core::Context *context)
	{
		using namespace Rocket::Core;
		document = context->LoadDocument("basic/animation/data/animation.rml");
		if (document != NULL)
		{
			{
				//document->GetElementById("title")->SetInnerRML(title);
				document->SetProperty("left", Property(position.x, Property::PX));
				document->SetProperty("top", Property(position.y, Property::PX));
				//document->Animate("opacity", Property(1.0f, Property::NUMBER), 1.5f, Tween{Tween::Quadratic, Tween::Out}, 1, true, 0.0f);
			}

			// Button fun
			{
				auto el = document->GetElementById("start_game");
				auto p1 = Transform::MakeProperty({ Transforms::Rotate2D{10.f}, Transforms::TranslateX{100.f} });
				auto p2 = Transform::MakeProperty({ Transforms::Scale2D{3.f} });
				el->Animate("transform", p1, 1.8f, Tween{ Tween::Elastic, Tween::InOut }, -1, true);
				el->AddAnimationKey("transform", p2, 1.3f, Tween{ Tween::Elastic, Tween::InOut });
			}
			{
				auto el = document->GetElementById("high_scores");
				el->Animate("margin-left", Property(0.f, Property::PX), 0.3f, Tween{ Tween::Sine, Tween::In }, 10, true, 1.f);
				el->AddAnimationKey("margin-left", Property(100.f, Property::PX), 3.0f, Tween{ Tween::Circular, Tween::Out });
			}
			{
				auto el = document->GetElementById("options");
				el->Animate("image-color", Property(Colourb(128, 255, 255, 255), Property::COLOUR), 0.3f, Tween{}, -1, false);
				el->AddAnimationKey("image-color", Property(Colourb(128, 128, 255, 255), Property::COLOUR), 0.3f);
				el->AddAnimationKey("image-color", Property(Colourb(0, 128, 128, 255), Property::COLOUR), 0.3f);
				el->AddAnimationKey("image-color", Property(Colourb(64, 128, 255, 0), Property::COLOUR), 0.9f);
				el->AddAnimationKey("image-color", Property(Colourb(255, 255, 255, 255), Property::COLOUR), 0.3f);
			}
			{
				auto el = document->GetElementById("help");
				el->Animate("margin-left", Property(100.f, Property::PX), 1.0f, Tween{ Tween::Quadratic, Tween::InOut }, -1, true);
			}
			{
				auto el = document->GetElementById("exit");
				PropertyDictionary pd;
				StyleSheetSpecification::ParsePropertyDeclaration(pd, "transform", "translate(200px, 200px) rotate(1215deg)");
				el->Animate("transform", *pd.GetProperty("transform"), 3.f, Tween{ Tween::Bounce, Tween::Out }, -1);
			}

			// Transform tests
			{
				auto el = document->GetElementById("generic");
				auto p = Transform::MakeProperty({ Transforms::TranslateY{50, Property::PX}, Transforms::Rotate3D{0.8f, 0, 1, 110, Property::DEG}});
				el->Animate("transform", p, 1.3f, Tween{Tween::Quadratic, Tween::InOut}, -1, true);
			}
			{
				auto el = document->GetElementById("combine");
				auto p = Transform::MakeProperty({ Transforms::Translate2D{50, 50, Property::PX}, Transforms::Rotate2D(1215) });
				el->Animate("transform", p, 8.0f, Tween{}, -1, true);
			}
			{
				auto el = document->GetElementById("decomposition");
				auto p = Transform::MakeProperty({ Transforms::Translate2D{50, 50, Property::PX}, Transforms::Rotate2D(1215) });
				el->Animate("transform", p, 8.0f, Tween{}, -1, true);
			}

			// Mixed units tests
			{
				auto el = document->GetElementById("abs_rel");
				el->Animate("margin-left", Property(50.f, Property::PERCENT), 1.5f, Tween{}, -1, true);
			}
			{
				auto el = document->GetElementById("abs_rel_transform");
				auto p = Transform::MakeProperty({ Transforms::TranslateX{0, Property::PX} });
				el->Animate("transform", p, 1.5f, Tween{}, -1, true);
			}
			{
				auto el = document->GetElementById("animation_event");
				el->Animate("top", Property(Math::RandomReal(250.f), Property::PX), 1.5f, Tween{ Tween::Cubic, Tween::InOut });
				el->Animate("left", Property(Math::RandomReal(250.f), Property::PX), 1.5f, Tween{ Tween::Cubic, Tween::InOut });
			}

			document->Show();
		}
	}

	void performance_test()
	{
		/*
		  FPS values
		  Original: 18.5
		  Without property counter: 22.0
		  With std::string: 23.0
		  robin_hood unordered_flat_map: 24.0
		  Avoid dirtying em's: 27.5
		
		*/
		
		if (!document)
			return;

		auto el = document->GetElementById("performance");

		Rocket::Core::String rml;

		for (int i = 0; i < 50; i++)
		{
			int index = rand() % 1000;
			int route = rand() % 50;
			int max = (rand() % 40) + 10;
			int value = rand() % max;
			Rocket::Core::String rml_row = Rocket::Core::CreateString(1000, R"(
			<div class="row">
				<div class="col col1"><button class="expand" index="%d">+</button>&nbsp;<a>Route %d</a></div>
				<div class="col col23"><input type="range" class="assign_range" min="0" max="%d" value="%d"/></div>
				<div class="col col4">Assigned</div>
				<div class="inrow unmark_collapse">
					<div class="col col123 assign_text">Assign to route</div>
					<div class="col col4">
						<button class="vehicle_depot_assign_confirm" quantity="0">Confirm</button>
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

		if (el)
			el->SetInnerRML(rml);
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
int nudge = 0;

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

	static double t_prev = 0.0f;
	double t = Shell::GetElapsedTime();
	float dt = float(t - t_prev);
	static int count_frames = 0;
	count_frames += 1;
	//t_prev = t;
	//if(dt > 1.0f)
	if(nudge)
	{
		t_prev = t;
		static float ff = 0.0f;
		ff += float(nudge)*0.3f;
		auto el = window->GetDocument()->GetElementById("exit");
		auto f = el->GetProperty<float>("margin-left");
		el->SetProperty("margin-left", Rocket::Core::Property(ff, Rocket::Core::Property::PX));
		float f_left = el->GetAbsoluteLeft();
		Rocket::Core::Log::Message(Rocket::Core::Log::LT_INFO, "margin-left: '%f'   abs: %f.", ff, f_left);
		nudge = 0;
	}

	if (window && dt > 0.2f)
	{
		t_prev = t;
		auto el = window->GetDocument()->GetElementById("fps");
		float fps = float(count_frames) / dt;
		count_frames = 0;
		el->SetInnerRML(Rocket::Core::CreateString( 20, "FPS: %f", fps ));
	}

	window->performance_test();
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
		using namespace Rocket::Core;

		if(value == "exit")
			Shell::RequestExit();

		if (event == "keydown")
		{
			bool key_down = event == "keydown";
			Rocket::Core::Input::KeyIdentifier key_identifier = (Rocket::Core::Input::KeyIdentifier) event.GetParameter< int >("key_identifier", 0);

			if (key_identifier == Rocket::Core::Input::KI_SPACE)
			{
				pause_loop = !pause_loop;
			}
			else if (key_identifier == Rocket::Core::Input::KI_RETURN)
			{
				pause_loop = true;
				single_loop = true;
			}
			else if (key_identifier == Rocket::Core::Input::KI_OEM_PLUS)
			{
				nudge = 1;
			}
			else if (key_identifier == Rocket::Core::Input::KI_OEM_MINUS)
			{
				nudge = -1;
			}
			else if (key_identifier == Rocket::Core::Input::KI_ESCAPE)
			{
				Shell::RequestExit();
			}
			else if (key_identifier == Rocket::Core::Input::KI_F8)
			{
				Rocket::Debugger::SetVisible(!Rocket::Debugger::IsVisible());
			}
			else if (key_identifier == Rocket::Core::Input::KI_LEFT)
			{
				auto el = context->GetRootElement()->GetElementById("keyevent_response");
				if (el) el->Animate("left", Property{ -200.f, Property::PX }, 0.5, Tween{ Tween::Cubic });
			}
			else if (key_identifier == Rocket::Core::Input::KI_RIGHT)
			{
				auto el = context->GetRootElement()->GetElementById("keyevent_response");
				if (el) el->Animate("left", Property{ 200.f, Property::PX }, 0.5, Tween{ Tween::Cubic });
			}
			else if (key_identifier == Rocket::Core::Input::KI_UP)
			{
				auto el = context->GetRootElement()->GetElementById("keyevent_response");
				auto offset_right = Property{ 200.f, Property::PX };
				if (el) el->Animate("left", Property{ 0.f, Property::PX }, 0.5, Tween{ Tween::Cubic }, 1, true, 0, &offset_right);
			}
			else if (key_identifier == Rocket::Core::Input::KI_DOWN)
			{
				auto el = context->GetRootElement()->GetElementById("keyevent_response");
				if (el) el->Animate("left", Property{ 0.f, Property::PX }, 0.5, Tween{ Tween::Cubic });
			}
		}
		if (event == "click")
		{
			auto el = event.GetTargetElement();
			if (el->GetId() == "transition_class")
			{
				el->SetClass("move_me", !el->IsClassSet("move_me"));
			}
		}
		if (event == "animationend")
		{
			auto el = event.GetTargetElement();
			if (el->GetId() == "animation_event")
			{
				el->Animate("top", Property(Math::RandomReal(200.f), Property::PX), 1.2f, Tween{ Tween::Cubic, Tween::InOut });
				el->Animate("left", Property(Math::RandomReal(100.f), Property::PERCENT), 0.8f, Tween{ Tween::Cubic, Tween::InOut });
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

	const int width = 1800;
	const int height = 1000;

	ShellRenderInterfaceOpenGL opengl_renderer;
	shell_renderer = &opengl_renderer;

	// Generic OS initialisation, creates a window and attaches OpenGL.
	if (!Shell::Initialise("../../Samples/") ||
		!Shell::OpenWindow("Animation Sample", shell_renderer, width, height, true))
	{
		Shell::Shutdown();
		return -1;
	}

	// Rocket initialisation.
	Rocket::Core::SetRenderInterface(&opengl_renderer);
	opengl_renderer.SetViewport(width, height);

	ShellSystemInterface system_interface;
	Rocket::Core::SetSystemInterface(&system_interface);

	Rocket::Core::Initialise();

	// Create the main Rocket context and set it on the shell's input layer.
	context = Rocket::Core::CreateContext("main", Rocket::Core::Vector2i(width, height));
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

	window = new DemoWindow("Animation sample", Rocket::Core::Vector2f(81, 100), context);
	window->GetDocument()->AddEventListener("keydown", new Event("hello"));
	window->GetDocument()->AddEventListener("keyup", new Event("hello"));
	window->GetDocument()->AddEventListener("animationend", new Event("hello"));


	Shell::EventLoop(GameLoop);

	delete window;

	// Shutdown Rocket.
	context->RemoveReference();
	Rocket::Core::Shutdown();

	Shell::CloseWindow();
	Shell::Shutdown();

	return 0;
}
