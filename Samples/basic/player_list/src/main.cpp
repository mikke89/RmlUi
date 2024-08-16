/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2018 Michael R. P. Ragazzon
 * Copyright (c) 2019-2024 The RmlUi Team, and contributors
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

static const int GENERATED_PLAYER_COUNT = 100;

class PlayerEntry {
public:
	bool is_local;
	uint32_t entity_id;

	uint32_t score;

	uint16_t latency;

	bool is_muted;
	bool is_friend;

	Rml::String name;

	Rml::String tag_name;
	Rml::Colourb tag_color;

public:
	static void InitializeDataModel(Rml::DataModelConstructor& constructor)
	{
		if (auto player = constructor.RegisterStruct<PlayerEntry>())
		{
			player.RegisterMember("id", &PlayerEntry::entity_id);
			player.RegisterMember("score", &PlayerEntry::score);
			player.RegisterMember("latency", &PlayerEntry::latency);
			player.RegisterMember("is_muted", &PlayerEntry::is_muted);
			player.RegisterMember("is_friend", &PlayerEntry::is_friend);
			player.RegisterMember("name", &PlayerEntry::name);
			player.RegisterMember("tag_name", &PlayerEntry::tag_name);
			player.RegisterMember("tag_color", &PlayerEntry::tag_color);
		}
	}
};

class PlayerList final : public Rml::EventListener {
public:
	PlayerList(Rml::Context* context) : last_update(0)
	{
		InitializeDataModel(context);

		document = context->LoadDocument("basic/player_list/data/player_list.rml");

		if (document)
		{
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
			if (event.GetParameter<int>("key_identifier", 0) == Rml::Input::KI_ESCAPE)
				Backend::RequestExit();
		}
		break;
		default: break;
		}
	}

	void Update(double t)
	{
		if (t < last_update + 0.5)
			return;

		RMLUI_ZoneScoped;

		// for (PlayerEntry& player : data)
		//{
		//	player.score = (uint32_t)Rml::Math::RandomInteger(100);
		//	player.latency = (uint16_t)Rml::Math::RandomInteger(500);
		// }

		data[5].score = (uint32_t)Rml::Math::RandomInteger(100);

		// Filter();
		// Sort();
		//

		entries = data;

		RMLUI_ASSERT(data_model);
		data_model.DirtyVariable("players[5].score");
		// data_model.DirtyVariable("players");

		last_update = t;
	}

	Rml::ElementDocument* GetDocument() const { return document; }

	void AddPlayer(PlayerEntry&& player) { data.emplace_back(std::move(player)); }

private:
	void InitializeDataModel(Rml::Context* context)
	{
		if (Rml::DataModelConstructor constructor = context->CreateDataModel("player_list"))
		{
			constructor.RegisterScalar<Rml::Colourb>([](const Rml::Colourb& c, Rml::Variant& variant) {
				variant = Rml::CreateString("rgba(%u, %u, %u, %u)", c.red, c.green, c.blue, c.alpha);
			});

			constructor.Bind("search_players_query", &search_query);

			PlayerEntry::InitializeDataModel(constructor);

			constructor.RegisterArray<decltype(entries)>();
			constructor.Bind("players", &entries);

			data_model = constructor.GetModelHandle();
		}
	}

	void Filter()
	{
		if (search_query.empty())
		{
			entries = data;
			return;
		}

		const auto search = [](const PlayerEntry& player, const Rml::String& query) -> bool {
			if (player.is_local)
				return true;

			Rml::String needle = Rml::StringUtilities::ToLower(player.name);
			if (needle.find(query) != Rml::String::npos)
				return true;

			needle = Rml::StringUtilities::ToLower(player.tag_name);
			if (needle.find(query) != Rml::String::npos)
				return true;

			return false;
		};

		// Dump all the current data first.
		entries.clear();

		// Use lowercase strings for querying.
		const Rml::String query = Rml::StringUtilities::ToLower(search_query);

		for (const PlayerEntry& player : data)
			if (search(player, query))
				entries.emplace_back(player);
	}

	void Sort()
	{
		auto start_it = std::partition(entries.begin(), entries.end(), [](const PlayerEntry& player) { return player.is_local; });
		std::sort(start_it, entries.end(), [](const PlayerEntry& a, const PlayerEntry& b) { return a.score > b.score; });
	}

private:
	Rml::ElementDocument* document;
	Rml::DataModelHandle data_model;

	double last_update;

	// A collection of all player data from the players manager.
	Rml::Vector<PlayerEntry> data;
	// A collection of players that is used in the UI document itself. It may be sorted, filtered out, ...
	Rml::Vector<PlayerEntry> entries;

	Rml::String search_query;
};

void GenerateFakePlayerData(PlayerList* list);

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
	if (!Backend::Initialize("Player List Sample", width, height, true))
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

	if (!context)
	{
		Rml::Shutdown();
		Backend::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	Rml::Debugger::Initialise(context);
	Shell::LoadFonts();

	auto player_list = Rml::MakeUnique<PlayerList>(context);
	context->AddEventListener("keydown", player_list.get());

	GenerateFakePlayerData(player_list.get());

	bool running = true;
	while (running)
	{
		running = Backend::ProcessEvents(context, &Shell::ProcessKeyDownShortcuts);

		double t = Rml::GetSystemInterface()->GetElapsedTime();

		player_list->Update(t);
		context->Update();

		Backend::BeginFrame();
		context->Render();
		Backend::PresentFrame();
	}

	player_list->Shutdown();

	// Shutdown RmlUi.
	Rml::Shutdown();

	Backend::Shutdown();
	Shell::Shutdown();

	return 0;
}

void GenerateFakePlayerData(PlayerList* list)
{
	const auto generate_fake_player = [](bool is_local) -> PlayerEntry {
		PlayerEntry data{};
		data.is_local = is_local;
		data.entity_id = (uint32_t)Rml::Math::RandomInteger(100'000);
		data.score = (uint32_t)Rml::Math::RandomInteger(100);
		data.latency = (uint16_t)Rml::Math::RandomInteger(500);
		data.is_muted = Rml::Math::RandomBool();
		data.is_friend = Rml::Math::RandomBool();
		Rml::FormatString(data.tag_name, "tag %d", Rml::Math::RandomInteger(1'000'000));
		data.name = is_local ? "local player" : Rml::CreateString("player %d", data.entity_id);

		const auto random_color_component = []() -> uint8_t { return (uint8_t)Rml::Math::RandomInteger(0xff); };
		data.tag_color = Rml::Colourb(random_color_component(), random_color_component(), random_color_component());

		return data;
	};

	for (int i = 0; i < GENERATED_PLAYER_COUNT; ++i)
		list->AddPlayer(generate_fake_player(i == 0));
}
