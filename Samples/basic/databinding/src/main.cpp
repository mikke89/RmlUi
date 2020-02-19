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
#include <numeric>


class DemoWindow : public Rml::Core::EventListener
{
public:
	DemoWindow(const Rml::Core::String &title, const Rml::Core::Vector2f &position, Rml::Core::Context *context)
	{
		using namespace Rml::Core;
		document = context->LoadDocument("basic/databinding/data/databinding.rml");
		if (document)
		{
			document->GetElementById("title")->SetInnerRML(title);
			document->SetProperty(PropertyId::Left, Property(position.x, Property::PX));
			document->SetProperty(PropertyId::Top, Property(position.y, Property::PX));

			document->Show();
		}
	}

	void Update() 
	{

	}

	void Shutdown() 
	{
		if (document)
		{
			document->Close();
			document = nullptr;
		}
	}

	void ProcessEvent(Rml::Core::Event& event) override
	{
		using namespace Rml::Core;

		switch (event.GetId())
		{
		case EventId::Keydown:
		{
			Rml::Core::Input::KeyIdentifier key_identifier = (Rml::Core::Input::KeyIdentifier) event.GetParameter< int >("key_identifier", 0);
			bool ctrl_key = event.GetParameter< bool >("ctrl_key", false);

			if (key_identifier == Rml::Core::Input::KI_ESCAPE)
			{
				Shell::RequestExit();
			}
			else if (key_identifier == Rml::Core::Input::KI_F8)
			{
				Rml::Debugger::SetVisible(!Rml::Debugger::IsVisible());
			}
		}
		break;

		default:
			break;
		}
	}

	Rml::Core::ElementDocument * GetDocument() {
		return document;
	}


private:
	Rml::Core::ElementDocument *document = nullptr;
};



struct Invader {
	Rml::Core::String name;
	Rml::Core::String sprite;
	Rml::Core::Colourb color{ 255, 255, 255 };
	std::vector<int> damage;
	float danger_rating = 50;

	void GetColor(Rml::Core::Variant& variant) {
		variant = "rgba(" + Rml::Core::ToString(color) + ')';
	}
	void SetColor(const Rml::Core::Variant& variant) {
		using namespace Rml::Core;
		String str = variant.Get<String>();
		if (str.size() > 6)
			str = str.substr(5, str.size() - 6);
		color = Rml::Core::FromString<Colourb>(variant.Get<String>());
	}
};


struct MyData {
	Rml::Core::String hello_world = "Hello World!";
	Rml::Core::String mouse_detector = "Mouse-move <em>Detector</em>.";

	int rating = 99;

	Invader delightful_invader{ "Delightful invader", "icon-invader" };

	std::vector<Invader> invaders = {
		Invader{"Angry invader", "icon-invader", {255, 40, 30}, {3, 6, 7}},
		Invader{"Harmless invader", "icon-flag", {20, 40, 255}, {5, 0}},
		Invader{"Hero", "icon-game", {255, 255, 30}, {10, 11, 12, 13, 14}},
	};

	std::vector<int> indices = { 1, 2, 3, 4, 5 };

	std::vector<Rml::Core::Vector2f> positions;

	void AddMousePos(Rml::Core::DataModelHandle model_handle, Rml::Core::Event& ev, const Rml::Core::VariantList& arguments)
	{
		positions.emplace_back(ev.GetParameter("mouse_x", 0.f), ev.GetParameter("mouse_y", 0.f));
		model_handle.DirtyVariable("positions");
	}

} my_data;


void ClearPositions(Rml::Core::DataModelHandle model_handle, Rml::Core::Event& ev, const Rml::Core::VariantList& arguments)
{
	my_data.positions.clear();
	model_handle.DirtyVariable("positions");
}

void HasGoodRating(Rml::Core::Variant& variant) {
	variant = int(my_data.rating > 50);
}

struct InvaderModelData {
	double time_last_invader_spawn = 0;
	double time_last_weapons_launched = 0;

	float incoming_invaders_rate = 10; // Per minute

	std::vector<Invader> invaders = {
		Invader{"Angry invader", "icon-invader", {255, 40, 30}, {3, 6, 7}, 80}
	};

	void LaunchWeapons(Rml::Core::DataModelHandle model_handle, Rml::Core::Event& /*ev*/, const Rml::Core::VariantList& /*arguments*/)
	{
		invaders.clear();
		model_handle.DirtyVariable("invaders");
	}

} invaders_data;


Rml::Core::DataModelHandle invaders_model, my_model;


bool SetupDataBinding(Rml::Core::Context* context)
{
	// The invaders model
	{
		Rml::Core::DataModelConstructor constructor = context->CreateDataModel("invaders");
		if (!constructor)
			return false;

		constructor.Bind("incoming_invaders_rate", &invaders_data.incoming_invaders_rate);

		constructor.RegisterArray<std::vector<int>>();

		if (auto invader_handle = constructor.RegisterStruct<Invader>())
		{
			invader_handle.RegisterMember("name", &Invader::name);
			invader_handle.RegisterMember("sprite", &Invader::sprite);
			invader_handle.RegisterMember("damage", &Invader::damage);
			invader_handle.RegisterMember("danger_rating", &Invader::danger_rating);
			invader_handle.RegisterMemberFunc("color", &Invader::GetColor, &Invader::SetColor);
		}

		constructor.RegisterArray<std::vector<Invader>>();
		constructor.Bind("invaders", &invaders_data.invaders);

		constructor.BindEventCallback("launch_weapons", &InvaderModelData::LaunchWeapons, &invaders_data);

		invaders_model = constructor.GetModelHandle();
	}


	// The main model
	{
		Rml::Core::DataModelConstructor constructor = context->CreateDataModel("my_model");
		if (!constructor)
			return false;

		constructor.Bind("hello_world", &my_data.hello_world);
		constructor.Bind("mouse_detector", &my_data.mouse_detector);
		constructor.Bind("rating", &my_data.rating);
		constructor.BindFunc("good_rating", &HasGoodRating);
		constructor.BindFunc("great_rating", [](Rml::Core::Variant& variant) {
			variant = int(my_data.rating > 80);
			});

		constructor.Bind("delightful_invader", &my_data.delightful_invader);

		//constructor.RegisterArray<std::vector<Invader>>();

		constructor.Bind("indices", &my_data.indices);
		constructor.Bind("invaders", &my_data.invaders);

		if (auto vec2_handle = constructor.RegisterStruct<Rml::Core::Vector2f>())
		{
			vec2_handle.RegisterMember("x", &Rml::Core::Vector2f::x);
			vec2_handle.RegisterMember("y", &Rml::Core::Vector2f::y);
		}

		constructor.RegisterArray<std::vector<Rml::Core::Vector2f>>();
		constructor.Bind("positions", &my_data.positions);

		constructor.BindEventCallback("clear_positions", &ClearPositions);
		constructor.BindEventCallback("add_mouse_pos", &MyData::AddMousePos, &my_data);

		my_model = constructor.GetModelHandle();
	}


	return true;
}


Rml::Core::Context* context = nullptr;
ShellRenderInterfaceExtensions *shell_renderer;
std::unique_ptr<DemoWindow> demo_window;

void GameLoop()
{
	if (my_model.IsVariableDirty("rating"))
	{
		my_model.DirtyVariable("good_rating");
		my_model.DirtyVariable("great_rating");

		size_t new_size = my_data.rating / 10 + 1;
		if (new_size != my_data.indices.size())
		{
			my_data.indices.resize(new_size);
			std::iota(my_data.indices.begin(), my_data.indices.end(), int(new_size));
			my_model.DirtyVariable("indices");
		}
	}

	const double t = Rml::Core::GetSystemInterface()->GetElapsedTime();

	const double t_next_spawn = invaders_data.time_last_invader_spawn + 60.0 / double(invaders_data.incoming_invaders_rate);
	if (t >= t_next_spawn)
	{
		using namespace Rml::Core;
		const int num_items = 4;
		static std::array<String, num_items> names = { "Angry invader", "Harmless invader", "Deceitful invader", "Cute invader" };
		static std::array<String, num_items> sprites = { "icon-invader", "icon-flag", "icon-game", "icon-waves" };
		static std::array<Colourb, num_items> colors = {{ { 255, 40, 30 }, {20, 40, 255}, {255, 255, 30}, {230, 230, 230} }};

		Invader new_invader;
		new_invader.name = names[rand() % num_items];
		new_invader.sprite = sprites[rand() % num_items];
		new_invader.color = colors[rand() % num_items];
		new_invader.danger_rating = float((rand() % 100) + 1);
		invaders_data.invaders.push_back(new_invader);

		invaders_model.DirtyVariable("invaders");
		invaders_data.time_last_invader_spawn = t;
	}

	if (t >= invaders_data.time_last_weapons_launched + 1.0)
	{
		if (!invaders_data.invaders.empty())
		{
			const size_t index = size_t(rand() % int(invaders_data.invaders.size()));

			Invader& invader = invaders_data.invaders[index];
			invader.damage.push_back(rand() % int(invader.danger_rating));

			invaders_model.DirtyVariable("invaders");
		}
		invaders_data.time_last_weapons_launched = t;
	}

	my_model.Update();

	invaders_model.Update();

	demo_window->Update();
	context->Update();

	shell_renderer->PrepareRenderBuffer();
	context->Render();
	shell_renderer->PresentRenderBuffer();
}




class DemoEventListener : public Rml::Core::EventListener
{
public:
	DemoEventListener(const Rml::Core::String& value, Rml::Core::Element* element) : value(value), element(element) {}

	void ProcessEvent(Rml::Core::Event& event) override
	{
		using namespace Rml::Core;

		if (value == "exit")
		{
			Shell::RequestExit();
		}
	}

	void OnDetach(Rml::Core::Element* element) override { delete this; }

private:
	Rml::Core::String value;
	Rml::Core::Element* element;
};



class DemoEventListenerInstancer : public Rml::Core::EventListenerInstancer
{
public:
	Rml::Core::EventListener* InstanceEventListener(const Rml::Core::String& value, Rml::Core::Element* element) override
	{
		return new DemoEventListener(value, element);
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

	const int width = 1600;
	const int height = 900;

	ShellRenderInterfaceOpenGL opengl_renderer;
	shell_renderer = &opengl_renderer;

	// Generic OS initialisation, creates a window and attaches OpenGL.
	if (!Shell::Initialise() ||
		!Shell::OpenWindow("Data Binding Sample", shell_renderer, width, height, true))
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

	if (!context || !SetupDataBinding(context))
	{
		Rml::Core::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	Rml::Controls::Initialise();
	Rml::Debugger::Initialise(context);
	Input::SetContext(context);
	shell_renderer->SetContext(context);
	
	DemoEventListenerInstancer event_listener_instancer;
	Rml::Core::Factory::RegisterEventListenerInstancer(&event_listener_instancer);

	Shell::LoadFonts("assets/");

	demo_window = std::make_unique<DemoWindow>("Data binding", Rml::Core::Vector2f(150, 50), context);
	demo_window->GetDocument()->AddEventListener(Rml::Core::EventId::Keydown, demo_window.get());
	demo_window->GetDocument()->AddEventListener(Rml::Core::EventId::Keyup, demo_window.get());

	Shell::EventLoop(GameLoop);

	demo_window->Shutdown();

	// Shutdown RmlUi.
	Rml::Core::Shutdown();

	Shell::CloseWindow();
	Shell::Shutdown();

	demo_window.reset();

	return 0;
}
