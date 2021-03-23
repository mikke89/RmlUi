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
#include <RmlUi/Debugger.h>
#include <Input.h>
#include <Shell.h>
#include <ShellRenderInterfaceOpenGL.h>
#include <numeric>


namespace BasicExample {

	Rml::DataModelHandle model_handle;

	struct MyData {
		Rml::String title = "Simple data binding example";
		Rml::String animal = "dog";
		bool show_text = true;
	} my_data;

	bool Initialize(Rml::Context* context)
	{
		Rml::DataModelConstructor constructor = context->CreateDataModel("basics");
		if (!constructor)
			return false;

		constructor.Bind("title", &my_data.title);
		constructor.Bind("animal", &my_data.animal);
		constructor.Bind("show_text", &my_data.show_text);

		model_handle = constructor.GetModelHandle();

		return true;
	}
}


namespace EventsExample {

	Rml::DataModelHandle model_handle;

	struct MyData {
		Rml::String hello_world = "Hello World!";
		Rml::String mouse_detector = "Mouse-move <em>Detector</em>.";
		int rating = 99;

		Rml::Vector<float> list = { 1, 2, 3, 4, 5 };

		Rml::Vector<Rml::Vector2f> positions;

		void AddMousePos(Rml::DataModelHandle model, Rml::Event& ev, const Rml::VariantList& /*arguments*/)
		{
			positions.emplace_back(ev.GetParameter("mouse_x", 0.f), ev.GetParameter("mouse_y", 0.f));
			model.DirtyVariable("positions");
		}

	} my_data;


	void ClearPositions(Rml::DataModelHandle model, Rml::Event& /*ev*/, const Rml::VariantList& /*arguments*/)
	{
		my_data.positions.clear();
		model.DirtyVariable("positions");
	}

	void HasGoodRating(Rml::Variant& variant)
	{
		variant = int(my_data.rating > 50);
	}


	bool Initialize(Rml::Context* context)
	{
		using namespace Rml;
		DataModelConstructor constructor = context->CreateDataModel("events");
		if (!constructor)
			return false;

		// Register all the types first
		constructor.RegisterArray<Rml::Vector<float>>();

		if (auto vec2_handle = constructor.RegisterStruct<Vector2f>())
		{
			vec2_handle.RegisterMember("x", &Vector2f::x);
			vec2_handle.RegisterMember("y", &Vector2f::y);
		}
		constructor.RegisterArray<Rml::Vector<Vector2f>>();

		// Bind the variables to the data model
		constructor.Bind("hello_world", &my_data.hello_world);
		constructor.Bind("mouse_detector", &my_data.mouse_detector);
		constructor.Bind("rating", &my_data.rating);
		constructor.BindFunc("good_rating", &HasGoodRating);
		constructor.BindFunc("great_rating", [](Variant& variant) {
			variant = int(my_data.rating > 80);
		});

		constructor.Bind("list", &my_data.list);

		constructor.Bind("positions", &my_data.positions);

		constructor.BindEventCallback("clear_positions", &ClearPositions);
		constructor.BindEventCallback("add_mouse_pos", &MyData::AddMousePos, &my_data);


		model_handle = constructor.GetModelHandle();

		return true;
	}

	void Update()
	{
		if (model_handle.IsVariableDirty("rating"))
		{
			model_handle.DirtyVariable("good_rating");
			model_handle.DirtyVariable("great_rating");

			size_t new_size = my_data.rating / 10 + 1;
			if (new_size != my_data.list.size())
			{
				my_data.list.resize(new_size);
				std::iota(my_data.list.begin(), my_data.list.end(), float(new_size));
				model_handle.DirtyVariable("list");
			}
		}
	}
}



namespace InvadersExample {

	Rml::DataModelHandle model_handle;

	struct Invader {
		Rml::String name;
		Rml::String sprite;
		Rml::Colourb color{ 255, 255, 255 };
		Rml::Vector<int> damage;
		float danger_rating = 50;
	};

	struct InvadersData {
		double time_last_invader_spawn = 0;
		double time_last_weapons_launched = 0;

		float incoming_invaders_rate = 10; // Per minute

		Rml::Vector<Invader> invaders = {
			Invader{"Angry invader", "icon-invader", {255, 40, 30}, {3, 6, 7}, 80}
		};

		void LaunchWeapons(Rml::DataModelHandle model, Rml::Event& /*ev*/, const Rml::VariantList& /*arguments*/)
		{
			invaders.clear();
			model.DirtyVariable("invaders");
		}

	} invaders_data;

	bool Initialize(Rml::Context* context)
	{
		Rml::DataModelConstructor constructor = context->CreateDataModel("invaders");
		if (!constructor)
			return false;

		// Register a custom getter/setter for the Colourb type.
		constructor.RegisterScalar<Rml::Colourb>(
			[](const Rml::Colourb& color, Rml::Variant& variant) {
				variant = "rgba(" + Rml::ToString(color) + ')';
			},
			[](Rml::Colourb& color, const Rml::Variant& variant) {
				Rml::String str = variant.Get<Rml::String>();
				bool success = false;
				if (str.size() > 6 && str.substr(0, 5) == "rgba(")
					success = Rml::TypeConverter<Rml::String, Rml::Colourb>::Convert(str.substr(5), color);
				if (!success)
					Rml::Log::Message(Rml::Log::LT_WARNING, "Invalid color specified: '%s'. Use syntax rgba(R,G,B,A).", str.c_str());
			}
		);

		// Since Invader::damage is an array type.
		constructor.RegisterArray<Rml::Vector<int>>();

		// Structs are registered by adding all its members through the returned handle.
		if (auto invader_handle = constructor.RegisterStruct<Invader>())
		{
			invader_handle.RegisterMember("name", &Invader::name);
			invader_handle.RegisterMember("sprite", &Invader::sprite);
			invader_handle.RegisterMember("color", &Invader::color);
			invader_handle.RegisterMember("damage", &Invader::damage);
			invader_handle.RegisterMember("danger_rating", &Invader::danger_rating);
		}

		// We can even have an Array of Structs, infinitely nested if we so desire.
		// Make sure the underlying type (here Invader) is registered before the array.
		constructor.RegisterArray<Rml::Vector<Invader>>();

		// Now we can bind the variables to the model.
		constructor.Bind("incoming_invaders_rate", &invaders_data.incoming_invaders_rate);
		constructor.Bind("invaders", &invaders_data.invaders);

		// This function will be called when the user clicks the 'Launch weapons' button.
		constructor.BindEventCallback("launch_weapons", &InvadersData::LaunchWeapons, &invaders_data);

		model_handle = constructor.GetModelHandle();

		return true;
	}

	void Update(const double t)
	{
		// Add new invaders at regular time intervals.
		const double t_next_spawn = invaders_data.time_last_invader_spawn + 60.0 / double(invaders_data.incoming_invaders_rate);
		if (t >= t_next_spawn)
		{
			using namespace Rml;
			const int num_items = 4;
			static Array<String, num_items> names = { "Angry invader", "Harmless invader", "Deceitful invader", "Cute invader" };
			static Array<String, num_items> sprites = { "icon-invader", "icon-flag", "icon-game", "icon-waves" };
			static Array<Colourb, num_items> colors = { { { 255, 40, 30 }, {20, 40, 255}, {255, 255, 30}, {230, 230, 230} } };

			Invader new_invader;
			new_invader.name = names[rand() % num_items];
			new_invader.sprite = sprites[rand() % num_items];
			new_invader.color = colors[rand() % num_items];
			new_invader.danger_rating = float((rand() % 100) + 1);
			invaders_data.invaders.push_back(new_invader);

			model_handle.DirtyVariable("invaders");
			invaders_data.time_last_invader_spawn = t;
		}

		// Launch shots from a random invader.
		if (t >= invaders_data.time_last_weapons_launched + 1.0)
		{
			if (!invaders_data.invaders.empty())
			{
				const size_t index = size_t(rand() % int(invaders_data.invaders.size()));

				Invader& invader = invaders_data.invaders[index];
				invader.damage.push_back(rand() % int(invader.danger_rating));

				model_handle.DirtyVariable("invaders");
			}
			invaders_data.time_last_weapons_launched = t;
		}
	}
}

namespace FormsExample {

	Rml::DataModelHandle model_handle;

	struct MyData {
		int rating = 50;
		bool pizza = true;
		bool pasta = false;
		bool lasagne = false;
		Rml::String animal = "dog";
		Rml::Vector<Rml::String> subjects = { "Choose your subject", "Feature request", "Bug report", "Praise", "Criticism" };
		int selected_subject = 0;
	} my_data;

	bool Initialize(Rml::Context* context)
	{
		Rml::DataModelConstructor constructor = context->CreateDataModel("forms");
		if (!constructor)
			return false;

		constructor.RegisterArray<Rml::Vector<Rml::String>>();

		constructor.Bind("rating", &my_data.rating);
		constructor.Bind("pizza", &my_data.pizza);
		constructor.Bind("pasta", &my_data.pasta);
		constructor.Bind("lasagne", &my_data.lasagne);
		constructor.Bind("animal", &my_data.animal);
		constructor.Bind("subjects", &my_data.subjects);
		constructor.Bind("selected_subject", &my_data.selected_subject);

		model_handle = constructor.GetModelHandle();

		return true;
	}
}


class DemoWindow : public Rml::EventListener
{
public:
	DemoWindow(const Rml::String &title, Rml::Context *context)
	{
		using namespace Rml;
		document = context->LoadDocument("basic/databinding/data/databinding.rml");
		if (document)
		{
			document->GetElementById("title")->SetInnerRML(title);
			document->Show();
		}
	}

	void Shutdown() 
	{
		if (document)
		{
			document->Close();
			document = nullptr;
		}
	}

	void ProcessEvent(Rml::Event& event) override
	{
		switch (event.GetId())
		{
		case Rml::EventId::Keydown:
		{
			Rml::Input::KeyIdentifier key_identifier = (Rml::Input::KeyIdentifier) event.GetParameter< int >("key_identifier", 0);

			if (key_identifier == Rml::Input::KI_ESCAPE)
			{
				Shell::RequestExit();
			}
		}
		break;

		default:
			break;
		}
	}

	Rml::ElementDocument * GetDocument() {
		return document;
	}


private:
	Rml::ElementDocument *document = nullptr;
};



Rml::Context* context = nullptr;
ShellRenderInterfaceExtensions *shell_renderer;

void GameLoop()
{
	const double t = Rml::GetSystemInterface()->GetElapsedTime();
	
	EventsExample::Update();
	InvadersExample::Update(t);

	context->Update();

	shell_renderer->PrepareRenderBuffer();
	context->Render();
	shell_renderer->PresentRenderBuffer();
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
	Rml::SetRenderInterface(&opengl_renderer);
	opengl_renderer.SetViewport(width, height);

	ShellSystemInterface system_interface;
	Rml::SetSystemInterface(&system_interface);

	Rml::Initialise();

	// Create the main RmlUi context and set it on the shell's input layer.
	context = Rml::CreateContext("main", Rml::Vector2i(width, height));

	if (!context
		|| !BasicExample::Initialize(context)
		|| !EventsExample::Initialize(context)
		|| !InvadersExample::Initialize(context)
		|| !FormsExample::Initialize(context)
		)
	{
		Rml::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	Rml::Debugger::Initialise(context);
	Input::SetContext(context);
	Shell::SetContext(context);
	
	Shell::LoadFonts("assets/");

	auto demo_window = Rml::MakeUnique<DemoWindow>("Data binding", context);
	demo_window->GetDocument()->AddEventListener(Rml::EventId::Keydown, demo_window.get());
	demo_window->GetDocument()->AddEventListener(Rml::EventId::Keyup, demo_window.get());

	Shell::EventLoop(GameLoop);

	demo_window->Shutdown();

	// Shutdown RmlUi.
	Rml::Shutdown();

	Shell::CloseWindow();
	Shell::Shutdown();

	demo_window.reset();

	return 0;
}
