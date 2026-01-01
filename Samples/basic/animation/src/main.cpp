#include <RmlUi/Core.h>
#include <RmlUi/Core/TransformPrimitive.h>
#include <RmlUi/Debugger.h>
#include <RmlUi_Backend.h>
#include <Shell.h>
#include <sstream>

class DemoWindow {
public:
	DemoWindow(const Rml::String& title, Rml::Context* context)
	{
		using namespace Rml;
		document = context->LoadDocument("basic/animation/data/animation.rml");
		if (document != nullptr)
		{
			document->GetElementById("title")->SetInnerRML(title);

			// Button fun
			{
				auto el = document->GetElementById("start_game");
				auto p1 = Transform::MakeProperty({Transforms::Rotate2D{10.f}, Transforms::TranslateX{100.f}});
				auto p2 = Transform::MakeProperty({Transforms::Scale2D{3.f}});
				el->Animate("transform", p1, 1.8f, Tween{Tween::Elastic, Tween::InOut}, -1, true);
				el->AddAnimationKey("transform", p2, 1.3f, Tween{Tween::Elastic, Tween::InOut});
			}
			{
				auto el = document->GetElementById("high_scores");
				el->Animate("margin-left", Property(0.f, Unit::PX), 0.3f, Tween{Tween::Sine, Tween::In}, 10, true, 1.f);
				el->AddAnimationKey("margin-left", Property(100.f, Unit::PX), 3.0f, Tween{Tween::Circular, Tween::Out});
			}
			{
				auto el = document->GetElementById("options");
				el->Animate("image-color", Property(Colourb(128, 255, 255, 255), Unit::COLOUR), 0.3f, Tween{}, -1, false);
				el->AddAnimationKey("image-color", Property(Colourb(128, 128, 255, 255), Unit::COLOUR), 0.3f);
				el->AddAnimationKey("image-color", Property(Colourb(0, 128, 128, 255), Unit::COLOUR), 0.3f);
				el->AddAnimationKey("image-color", Property(Colourb(64, 128, 255, 0), Unit::COLOUR), 0.9f);
				el->AddAnimationKey("image-color", Property(Colourb(255, 255, 255, 255), Unit::COLOUR), 0.3f);
			}
			{
				auto el = document->GetElementById("exit");
				PropertyDictionary pd;
				StyleSheetSpecification::ParsePropertyDeclaration(pd, "transform", "translate(200px, 200px) rotate(1215deg)");
				el->Animate("transform", *pd.GetProperty(PropertyId::Transform), 3.f, Tween{Tween::Bounce, Tween::Out}, -1);
			}

			// Transform tests
			{
				auto el = document->GetElementById("generic");
				auto p = Transform::MakeProperty(
					{Transforms::TranslateY{50, Unit::PX}, Transforms::Rotate3D{0, 0, 1, -90, Unit::DEG}, Transforms::ScaleY{0.8f}});
				el->Animate("transform", p, 1.5f, Tween{Tween::Sine, Tween::InOut}, -1, true);
			}
			{
				auto el = document->GetElementById("combine");
				auto p = Transform::MakeProperty({Transforms::Translate2D{50, 50, Unit::PX}, Transforms::Rotate2D(1215)});
				el->Animate("transform", p, 8.0f, Tween{}, -1, true);
			}
			{
				auto el = document->GetElementById("decomposition");
				auto p = Transform::MakeProperty({Transforms::TranslateY{50, Unit::PX}, Transforms::Rotate3D{0.8f, 0, 1, 110, Unit::DEG}});
				el->Animate("transform", p, 1.3f, Tween{Tween::Quadratic, Tween::InOut}, -1, true);
			}

			// Mixed units tests
			{
				auto el = document->GetElementById("abs_rel");
				el->Animate("margin-left", Property(50.f, Unit::PERCENT), 1.5f, Tween{}, -1, true);
			}
			{
				auto el = document->GetElementById("abs_rel_transform");
				auto p = Transform::MakeProperty({Transforms::TranslateX{0, Unit::PX}});
				el->Animate("transform", p, 1.5f, Tween{}, -1, true);
			}
			{
				auto el = document->GetElementById("animation_event");
				el->Animate("top", Property(Math::RandomReal(250.f), Unit::PX), 1.5f, Tween{Tween::Cubic, Tween::InOut});
				el->Animate("left", Property(Math::RandomReal(250.f), Unit::PX), 1.5f, Tween{Tween::Cubic, Tween::InOut});
			}

			document->Show();
		}
	}

	void Update(double t)
	{
		if (document)
		{
			if (t - t_prev_fade >= 1.4)
			{
				auto el = document->GetElementById("help");
				if (el->IsClassSet("fadeout"))
				{
					el->SetClass("fadeout", false);
					el->SetClass("fadein", true);
				}
				else if (el->IsClassSet("fadein"))
				{
					el->SetClass("fadein", false);
					el->SetClass("textalign", true);
				}
				else
				{
					el->SetClass("textalign", false);
					el->SetClass("fadeout", true);
				}

				t_prev_fade = t;
			}
		}
	}

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
	double t_prev_fade = 0;
};

Rml::Context* context = nullptr;
bool run_loop = true;
bool single_loop = false;
int nudge = 0;

class Event : public Rml::EventListener {
public:
	Event(const Rml::String& value) : value(value) {}

	void ProcessEvent(Rml::Event& event) override
	{
		using namespace Rml;

		if (value == "exit")
			Backend::RequestExit();

		switch (event.GetId())
		{
		case EventId::Keydown:
		{
			Rml::Input::KeyIdentifier key_identifier = (Rml::Input::KeyIdentifier)event.GetParameter<int>("key_identifier", 0);

			if (key_identifier == Rml::Input::KI_SPACE)
			{
				run_loop = !run_loop;
			}
			else if (key_identifier == Rml::Input::KI_RETURN)
			{
				run_loop = false;
				single_loop = true;
			}
			else if (key_identifier == Rml::Input::KI_OEM_PLUS)
			{
				nudge = 1;
			}
			else if (key_identifier == Rml::Input::KI_OEM_MINUS)
			{
				nudge = -1;
			}
			else if (key_identifier == Rml::Input::KI_ESCAPE)
			{
				Backend::RequestExit();
			}
			else if (key_identifier == Rml::Input::KI_LEFT)
			{
				auto el = context->GetRootElement()->GetElementById("keyevent_response");
				if (el)
					el->Animate("left", Property{-200.f, Unit::DP}, 0.5, Tween{Tween::Cubic});
			}
			else if (key_identifier == Rml::Input::KI_RIGHT)
			{
				auto el = context->GetRootElement()->GetElementById("keyevent_response");
				if (el)
					el->Animate("left", Property{200.f, Unit::DP}, 0.5, Tween{Tween::Cubic});
			}
			else if (key_identifier == Rml::Input::KI_UP)
			{
				auto el = context->GetRootElement()->GetElementById("keyevent_response");
				auto offset_right = Property{200.f, Unit::DP};
				if (el)
					el->Animate("left", Property{0.f, Unit::PX}, 0.5, Tween{Tween::Cubic}, 1, true, 0, &offset_right);
			}
			else if (key_identifier == Rml::Input::KI_DOWN)
			{
				auto el = context->GetRootElement()->GetElementById("keyevent_response");
				if (el)
					el->Animate("left", Property{0.f, Unit::PX}, 0.5, Tween{Tween::Cubic});
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
				el->Animate("top", Property(Math::RandomReal(200.f), Unit::PX), 1.2f, Tween{Tween::Cubic, Tween::InOut});
				el->Animate("left", Property(Math::RandomReal(100.f), Unit::PERCENT), 0.8f, Tween{Tween::Cubic, Tween::InOut});
			}
		}
		break;

		default: break;
		}
	}

	void OnDetach(Rml::Element* /*element*/) override { delete this; }

private:
	Rml::String value;
};

class EventInstancer : public Rml::EventListenerInstancer {
public:
	Rml::EventListener* InstanceEventListener(const Rml::String& value, Rml::Element* /*element*/) override { return new Event(value); }
};

#if defined RMLUI_PLATFORM_WIN32
	#include <RmlUi_Include_Windows.h>
int APIENTRY WinMain(HINSTANCE /*instance_handle*/, HINSTANCE /*previous_instance_handle*/, char* /*command_line*/, int /*command_show*/)
#else
int main(int /*argc*/, char** /*argv*/)
#endif
{
	const int window_width = 1700;
	const int window_height = 900;

	// Initializes the shell which provides common functionality used by the included samples.
	if (!Shell::Initialize())
		return -1;

	// Constructs the system and render interfaces, creates a window, and attaches the renderer.
	if (!Backend::Initialize("Animation Sample", window_width, window_height, true))
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
	context = Rml::CreateContext("main", Rml::Vector2i(window_width, window_height));
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

	Rml::UniquePtr<DemoWindow> window = Rml::MakeUnique<DemoWindow>("Animation sample", context);
	window->GetDocument()->AddEventListener(Rml::EventId::Keydown, new Event("hello"));
	window->GetDocument()->AddEventListener(Rml::EventId::Keyup, new Event("hello"));
	window->GetDocument()->AddEventListener(Rml::EventId::Animationend, new Event("hello"));

	bool running = true;
	while (running)
	{
		running = Backend::ProcessEvents(context, &Shell::ProcessKeyDownShortcuts);

		double t = Rml::GetSystemInterface()->GetElapsedTime();

		if (run_loop || single_loop)
		{
			window->Update(t);
			context->Update();

			Backend::BeginFrame();
			context->Render();
			Backend::PresentFrame();

			single_loop = false;
		}

		static double t_prev = 0.0f;
		float dt = float(t - t_prev);
		static int count_frames = 0;
		count_frames += 1;

		if (nudge)
		{
			t_prev = t;
			static float ff = 0.0f;
			ff += float(nudge) * 0.3f;
			auto el = window->GetDocument()->GetElementById("exit");
			el->SetProperty(Rml::PropertyId::MarginLeft, Rml::Property(ff, Rml::Unit::PX));
			float f_left = el->GetAbsoluteLeft();
			Rml::Log::Message(Rml::Log::LT_INFO, "margin-left: '%f'   abs: %f.", ff, f_left);
			nudge = 0;
		}

		if (dt > 0.2f)
		{
			t_prev = t;
			auto el = window->GetDocument()->GetElementById("fps");
			float fps = float(count_frames) / dt;
			count_frames = 0;
			el->SetInnerRML(Rml::CreateString("FPS: %f", fps));
		}
	}

	window.reset();

	// Shutdown RmlUi.
	Rml::Shutdown();
	Backend::Shutdown();
	Shell::Shutdown();

	return 0;
}
