#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include <RmlUi_Backend.h>
#include <Shell.h>
#include <numeric>

namespace {
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
} // namespace BasicExample

namespace EventsExample {

	Rml::DataModelHandle model_handle;

	struct MyData {
		Rml::String hello_world = "Hello World!";
		Rml::String mouse_detector = "Mouse-move <em>Detector</em>.";
		int rating = 99;

		Rml::Vector<float> list = {1, 2, 3, 4, 5};

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
		constructor.BindFunc("great_rating", [](Variant& variant) { variant = int(my_data.rating > 80); });

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
} // namespace EventsExample

namespace InvadersExample {

	Rml::DataModelHandle model_handle;

	static constexpr int num_invaders = 12;
	static constexpr double incoming_invaders_rate = 50; // Per minute

	struct Invader {
		Rml::String name;
		Rml::String sprite;
		Rml::Colourb color{255, 255, 255};
		float max_health = 0;
		float charge_rate = 0;
		float health = 0;
		float charge = 0;
	};

	struct InvadersData {
		float health = 0;
		float charge = 0;
		int score = 0;

		double elapsed_time = 0;
		double next_invader_spawn_time = 0;

		int num_games_played = 0;

		Rml::Array<Invader, num_invaders> invaders;

		// Start a new game.
		void StartGame(Rml::DataModelHandle model, Rml::Event& /*ev*/, const Rml::VariantList& /*arguments*/)
		{
			health = 100;
			charge = 30;
			score = 0;
			elapsed_time = 0;
			next_invader_spawn_time = 0;
			num_games_played += 1;

			for (Invader& invader : invaders)
				invader.health = 0;

			model.DirtyVariable("health");
			model.DirtyVariable("charge");
			model.DirtyVariable("score");
			model.DirtyVariable("elapsed_time");
			model.DirtyVariable("num_games_played");
			model.DirtyVariable("invaders");
		}

		// Fire on the invader of the given index (first argument).
		void Fire(Rml::DataModelHandle model, Rml::Event& /*ev*/, const Rml::VariantList& arguments)
		{
			if (arguments.size() != 1)
				return;
			const size_t index = arguments[0].Get<size_t>();
			if (index >= invaders.size())
				return;

			Invader& invader = invaders[index];
			if (health <= 0 || invader.health <= 0)
				return;

			const float new_health = Rml::Math::Max(invader.health - charge * Rml::Math::SquareRoot(charge), 0.0f);

			charge = 30.f;
			score += int(invader.health - new_health) + 1000 * (new_health == 0);

			invader.health = new_health;

			model.DirtyVariable("invaders");
			model.DirtyVariable("charge");
			model.DirtyVariable("score");
		}

	} data;

	bool Initialize(Rml::Context* context)
	{
		Rml::DataModelConstructor constructor = context->CreateDataModel("invaders");
		if (!constructor)
			return false;

		// Register a custom getter for the Colourb type.
		constructor.RegisterScalar<Rml::Colourb>([](const Rml::Colourb& color, Rml::Variant& variant) { variant = Rml::ToString(color); });
		// Register a transform function for formatting time
		constructor.RegisterTransformFunc("format_time", [](const Rml::VariantList& arguments) -> Rml::Variant {
			if (arguments.empty())
				return {};
			const double t = arguments[0].Get<double>();
			const int minutes = int(t) / 60;
			const double seconds = t - 60.0 * double(minutes);
			return Rml::Variant(Rml::CreateString("%02d:%05.2f", minutes, seconds));
		});

		// Structs are registered by adding all their members through the returned handle.
		if (auto invader_handle = constructor.RegisterStruct<Invader>())
		{
			invader_handle.RegisterMember("name", &Invader::name);
			invader_handle.RegisterMember("sprite", &Invader::sprite);
			invader_handle.RegisterMember("color", &Invader::color);
			invader_handle.RegisterMember("max_health", &Invader::max_health);
			invader_handle.RegisterMember("charge_rate", &Invader::charge_rate);
			invader_handle.RegisterMember("health", &Invader::health);
			invader_handle.RegisterMember("charge", &Invader::charge);
		}

		// We can even have an Array of Structs, infinitely nested if we so desire.
		// Make sure the underlying type (here Invader) is registered before the array.
		constructor.RegisterArray<decltype(data.invaders)>();

		// Now we can bind the variables to the model.
		constructor.Bind("invaders", &data.invaders);
		constructor.Bind("health", &data.health);
		constructor.Bind("charge", &data.charge);
		constructor.Bind("score", &data.score);
		constructor.Bind("elapsed_time", &data.elapsed_time);
		constructor.Bind("num_games_played", &data.num_games_played);

		// This function will be called when the user clicks on the (re)start game button.
		constructor.BindEventCallback("start_game", &InvadersData::StartGame, &data);
		// This function will be called when the user clicks on any of the invaders.
		constructor.BindEventCallback("fire", &InvadersData::Fire, &data);

		model_handle = constructor.GetModelHandle();

		return true;
	}

	void Update(const double dt)
	{
		using namespace Rml;

		if (data.health == 0)
			return;

		data.elapsed_time += dt;
		model_handle.DirtyVariable("elapsed_time");

		// Steadily increase the player charge.
		data.charge = Math::Min(data.charge + float(40.0 * dt), 100.f);
		model_handle.DirtyVariable("charge");

		// Add new invaders at the scheduled time.
		if (data.elapsed_time >= data.next_invader_spawn_time)
		{
			constexpr int num_items = 4;
			static Array<String, num_items> names = {"Angry invader", "Harmless invader", "Deceitful invader", "Cute invader"};
			static Array<String, num_items> sprites = {"icon-invader", "icon-flag", "icon-game", "icon-waves"};
			static Array<Colourb, num_items> colors = {{{255, 40, 30}, {20, 40, 255}, {255, 255, 30}, {230, 230, 230}}};

			Invader new_invader;
			new_invader.name = names[Math::RandomInteger(num_items)];
			new_invader.sprite = sprites[Math::RandomInteger(num_items)];
			new_invader.color = colors[Math::RandomInteger(num_items)];

			new_invader.max_health = 300.f + float(30.0 * data.elapsed_time) + Math::RandomReal(300.f);
			new_invader.charge_rate = 10.f + Math::RandomReal(50.f);

			new_invader.health = new_invader.max_health;

			// Find an available slot to spawn the new invader in.
			const int i_begin = Math::RandomInteger(num_invaders);
			for (int i = 0; i < num_invaders; i++)
			{
				Invader& invader = data.invaders[(i + i_begin) % num_invaders];
				if (invader.health <= 0)
				{
					invader = std::move(new_invader);
					model_handle.DirtyVariable("invaders");
					break;
				}
			}

			// Add new invaders at steadily decreasing time intervals.
			data.next_invader_spawn_time = data.elapsed_time + 60.0 / (incoming_invaders_rate + 0.1 * data.elapsed_time);
		}

		// Iterate through the invaders and fire at the player.
		for (Invader& invader : data.invaders)
		{
			if (invader.health > 0)
			{
				invader.charge = invader.charge + invader.charge_rate * float(dt);

				if (invader.charge >= 100)
				{
					data.health = Math::Max(data.health - float(10.0 * dt), 0.0f);
					model_handle.DirtyVariable("health");
				}

				if (invader.charge >= 120)
					invader.charge = 0;

				model_handle.DirtyVariable("invaders");
			}
		}
	}
} // namespace InvadersExample

namespace FormsExample {

	Rml::DataModelHandle model_handle;

	struct MyData {
		int rating = 50;
		bool pizza = true;
		bool pasta = false;
		bool lasagne = false;
		Rml::String animal = "dog";
		Rml::Vector<Rml::String> subjects = {"Choose your subject", "Feature request", "Bug report", "Praise", "Criticism"};
		int selected_subject = 0;
		Rml::String new_subject = "New subject";
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
		constructor.Bind("new_subject", &my_data.new_subject);

		constructor.BindEventCallback("add_subject", [](Rml::DataModelHandle model, Rml::Event& /*ev*/, const Rml::VariantList& arguments) {
			Rml::String name = (arguments.size() == 1 ? arguments[0].Get<Rml::String>() : "");
			if (!name.empty())
			{
				my_data.subjects.push_back(std::move(name));
				model.DirtyVariable("subjects");
			}
		});
		constructor.BindEventCallback("erase_subject", [](Rml::DataModelHandle model, Rml::Event& /*ev*/, const Rml::VariantList& arguments) {
			const int i = (arguments.size() == 1 ? arguments[0].Get<int>(-1) : -1);
			if (i >= 0 && i < (int)my_data.subjects.size())
			{
				my_data.subjects.erase(my_data.subjects.begin() + i);
				my_data.selected_subject = 0;
				model.DirtyVariable("subjects");
				model.DirtyVariable("selected_subject");
			}
		});

		model_handle = constructor.GetModelHandle();

		return true;
	}
} // namespace FormsExample
} // namespace

class DemoWindow : public Rml::EventListener {
public:
	DemoWindow(const Rml::String& title, Rml::Context* context)
	{
		document = context->LoadDocument("basic/data_binding/data/data_binding.rml");
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
			Rml::Input::KeyIdentifier key_identifier = (Rml::Input::KeyIdentifier)event.GetParameter<int>("key_identifier", 0);

			if (key_identifier == Rml::Input::KI_ESCAPE)
				Backend::RequestExit();
		}
		break;
		default: break;
		}
	}

	Rml::ElementDocument* GetDocument() { return document; }

private:
	Rml::ElementDocument* document = nullptr;
};

#if defined RMLUI_PLATFORM_WIN32
	#include <RmlUi_Include_Windows.h>
int APIENTRY WinMain(HINSTANCE /*instance_handle*/, HINSTANCE /*previous_instance_handle*/, char* /*command_line*/, int /*command_show*/)
#else
int main(int /*argc*/, char** /*argv*/)
#endif
{
	const int width = 1600;
	const int height = 900;

	// Initializes the shell which provides common functionality used by the included samples.
	if (!Shell::Initialize())
		return -1;

	// Constructs the system and render interfaces, creates a window, and attaches the renderer.
	if (!Backend::Initialize("Data Binding Sample", width, height, true))
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
	Rml::Context* context = Rml::CreateContext("main", Rml::Vector2i(width, height));

	if (!context                                 //
		|| !BasicExample::Initialize(context)    //
		|| !EventsExample::Initialize(context)   //
		|| !InvadersExample::Initialize(context) //
		|| !FormsExample::Initialize(context)    //
	)
	{
		Rml::Shutdown();
		Backend::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	Rml::Debugger::Initialise(context);
	Shell::LoadFonts();

	auto demo_window = Rml::MakeUnique<DemoWindow>("Data binding", context);
	demo_window->GetDocument()->AddEventListener(Rml::EventId::Keydown, demo_window.get());
	demo_window->GetDocument()->AddEventListener(Rml::EventId::Keyup, demo_window.get());

	double t_prev = 0;
	bool running = true;
	while (running)
	{
		running = Backend::ProcessEvents(context, &Shell::ProcessKeyDownShortcuts);

		const double t = Rml::GetSystemInterface()->GetElapsedTime();
		const double dt = Rml::Math::Min(t - t_prev, 0.1);
		t_prev = t;

		EventsExample::Update();
		InvadersExample::Update(dt);

		context->Update();

		Backend::BeginFrame();
		context->Render();
		Backend::PresentFrame();
	}

	demo_window->Shutdown();

	// Shutdown RmlUi.
	Rml::Shutdown();

	Backend::Shutdown();
	Shell::Shutdown();

	demo_window.reset();

	return 0;
}
