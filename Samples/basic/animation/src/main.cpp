/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2018 Michael R. P. Ragazzon
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
#include <RmlUi/Core/TransformPrimitive.h>

#include <sstream>


class DemoWindow
{
public:
	DemoWindow(const Rml::Core::String &title, const Rml::Core::Vector2f &position, Rml::Core::Context *context)
	{
		using namespace Rml::Core;
		document = context->LoadDocument("basic/animation/data/animation.rml");
		if (document != nullptr)
		{
			{
				document->GetElementById("title")->SetInnerRML(title);
				document->SetProperty(PropertyId::Left, Property(position.x, Property::PX));
				document->SetProperty(PropertyId::Top, Property(position.y, Property::PX));
				//document->Animate("opacity", Property(0.0f, Property::NUMBER), 2.0f, Tween{ Tween::Quadratic, Tween::InOut }, -1, true, 1.0f);
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
				el->Animate("transform", *pd.GetProperty(PropertyId::Transform), 3.f, Tween{ Tween::Bounce, Tween::Out }, -1);
			}

			// Transform tests
			{
				auto el = document->GetElementById("generic");
				auto p = Transform::MakeProperty({ Transforms::TranslateY{50, Property::PX}, Transforms::RotateZ{-90, Property::DEG}, Transforms::ScaleY{0.8f} });
				el->Animate("transform", p, 1.5f, Tween{Tween::Sine, Tween::InOut}, -1, true);
			}
			{
				auto el = document->GetElementById("combine");
				auto p = Transform::MakeProperty({ Transforms::Translate2D{50, 50, Property::PX}, Transforms::Rotate2D(1215) });
				el->Animate("transform", p, 8.0f, Tween{}, -1, true);
			}
			{
				auto el = document->GetElementById("decomposition");
				auto p = Transform::MakeProperty({ Transforms::TranslateY{50, Property::PX}, Transforms::Rotate3D{0.8f, 0, 1, 110, Property::DEG} });
				el->Animate("transform", p, 1.3f, Tween{ Tween::Quadratic, Tween::InOut }, -1, true);
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

	~DemoWindow()
	{
		if (document)
		{
			document->Close();
		}
	}

	Rml::Core::ElementDocument * GetDocument() {
		return document;
	}

private:
	Rml::Core::ElementDocument *document;
};


Rml::Core::Context* context = nullptr;
ShellRenderInterfaceExtensions *shell_renderer;
DemoWindow* window = nullptr;

bool run_loop = true;
bool single_loop = false;
int nudge = 0;

void GameLoop()
{
	if(run_loop || single_loop)
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

	if(nudge)
	{
		t_prev = t;
		static float ff = 0.0f;
		ff += float(nudge)*0.3f;
		auto el = window->GetDocument()->GetElementById("exit");
		auto f = el->GetProperty<float>("margin-left");
		el->SetProperty(Rml::Core::PropertyId::MarginLeft, Rml::Core::Property(ff, Rml::Core::Property::PX));
		float f_left = el->GetAbsoluteLeft();
		Rml::Core::Log::Message(Rml::Core::Log::LT_INFO, "margin-left: '%f'   abs: %f.", ff, f_left);
		nudge = 0;
	}

	if (window && dt > 0.2f)
	{
		t_prev = t;
		auto el = window->GetDocument()->GetElementById("fps");
		float fps = float(count_frames) / dt;
		count_frames = 0;
		el->SetInnerRML(Rml::Core::CreateString( 20, "FPS: %f", fps ));
	}
}



class Event : public Rml::Core::EventListener
{
public:
	Event(const Rml::Core::String& value) : value(value) {}

	void ProcessEvent(Rml::Core::Event& event) override
	{
		using namespace Rml::Core;

		if(value == "exit")
			Shell::RequestExit();

		switch (event.GetId())
		{
		case EventId::Keydown:
		{
			Rml::Core::Input::KeyIdentifier key_identifier = (Rml::Core::Input::KeyIdentifier) event.GetParameter< int >("key_identifier", 0);

			if (key_identifier == Rml::Core::Input::KI_SPACE)
			{
				run_loop = !run_loop;
			}
			else if (key_identifier == Rml::Core::Input::KI_RETURN)
			{
				run_loop = false;
				single_loop = true;
			}
			else if (key_identifier == Rml::Core::Input::KI_OEM_PLUS)
			{
				nudge = 1;
			}
			else if (key_identifier == Rml::Core::Input::KI_OEM_MINUS)
			{
				nudge = -1;
			}
			else if (key_identifier == Rml::Core::Input::KI_ESCAPE)
			{
				Shell::RequestExit();
			}
			else if (key_identifier == Rml::Core::Input::KI_F8)
			{
				Rml::Debugger::SetVisible(!Rml::Debugger::IsVisible());
			}
			else if (key_identifier == Rml::Core::Input::KI_LEFT)
			{
				auto el = context->GetRootElement()->GetElementById("keyevent_response");
				if (el) el->Animate("left", Property{ -200.f, Property::PX }, 0.5, Tween{ Tween::Cubic });
			}
			else if (key_identifier == Rml::Core::Input::KI_RIGHT)
			{
				auto el = context->GetRootElement()->GetElementById("keyevent_response");
				if (el) el->Animate("left", Property{ 200.f, Property::PX }, 0.5, Tween{ Tween::Cubic });
			}
			else if (key_identifier == Rml::Core::Input::KI_UP)
			{
				auto el = context->GetRootElement()->GetElementById("keyevent_response");
				auto offset_right = Property{ 200.f, Property::PX };
				if (el) el->Animate("left", Property{ 0.f, Property::PX }, 0.5, Tween{ Tween::Cubic }, 1, true, 0, &offset_right);
			}
			else if (key_identifier == Rml::Core::Input::KI_DOWN)
			{
				auto el = context->GetRootElement()->GetElementById("keyevent_response");
				if (el) el->Animate("left", Property{ 0.f, Property::PX }, 0.5, Tween{ Tween::Cubic });
			}
		}
		break;

		case EventId::Click:
		{
			auto el = event.GetTargetElement();
			if (el->GetId() == "transition_class")
			{
				el->SetClass("move_me", !el->IsClassSet("move_me"));
			}
		}
		break;

		case EventId::Animationend:
		{
			auto el = event.GetTargetElement();
			if (el->GetId() == "animation_event")
			{
				el->Animate("top", Property(Math::RandomReal(200.f), Property::PX), 1.2f, Tween{ Tween::Cubic, Tween::InOut });
				el->Animate("left", Property(Math::RandomReal(100.f), Property::PERCENT), 0.8f, Tween{ Tween::Cubic, Tween::InOut });
			}
		}
		break;

		default:
			break;
		}
	}

	void OnDetach(Rml::Core::Element* element) override { delete this; }

private:
	Rml::Core::String value;
};


class EventInstancer : public Rml::Core::EventListenerInstancer
{
public:

	/// Instances a new event handle for Invaders.
	Rml::Core::EventListener* InstanceEventListener(const Rml::Core::String& value, Rml::Core::Element* element) override
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
		!Shell::OpenWindow("Animation Sample", shell_renderer, width, height, true))
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

	EventInstancer event_listener_instancer;
	Rml::Core::Factory::RegisterEventListenerInstancer(&event_listener_instancer);

	Shell::LoadFonts("assets/");

	window = new DemoWindow("Animation sample", Rml::Core::Vector2f(81, 100), context);
	window->GetDocument()->AddEventListener(Rml::Core::EventId::Keydown, new Event("hello"));
	window->GetDocument()->AddEventListener(Rml::Core::EventId::Keyup, new Event("hello"));
	window->GetDocument()->AddEventListener(Rml::Core::EventId::Animationend, new Event("hello"));


	Shell::EventLoop(GameLoop);

	delete window;

	// Shutdown RmlUi.
	Rml::Core::Shutdown();

	Shell::CloseWindow();
	Shell::Shutdown();

	return 0;
}
