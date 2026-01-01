#include "DecoratorDefender.h"
#include "DecoratorStarfield.h"
#include "ElementGame.h"
#include "EventHandlerHighScore.h"
#include "EventHandlerOptions.h"
#include "EventHandlerStartGame.h"
#include "EventListenerInstancer.h"
#include "EventManager.h"
#include "HighScores.h"
#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include <RmlUi_Backend.h>
#include <Shell.h>

Rml::Context* context = nullptr;

#if defined RMLUI_PLATFORM_WIN32
	#include <RmlUi_Include_Windows.h>
int APIENTRY WinMain(HINSTANCE /*instance_handle*/, HINSTANCE /*previous_instance_handle*/, char* /*command_line*/, int /*command_show*/)
#else
int main(int /*argc*/, char** /*argv*/)
#endif
{
	const int window_width = 1024;
	const int window_height = 768;

	// Initializes the shell which provides common functionality used by the included samples.
	if (!Shell::Initialize())
		return -1;

	// Constructs the system and render interfaces, creates a window, and attaches the renderer.
	if (!Backend::Initialize("RmlUi Invaders from Mars", window_width, window_height, false))
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

	// Initialise the RmlUi debugger.
	Rml::Debugger::Initialise(context);

	// Load the font faces required for Invaders.
	Shell::LoadFonts();

	// Register Invader's custom element and decorator instancers.
	Rml::ElementInstancerGeneric<ElementGame> element_instancer_game;
	Rml::Factory::RegisterElementInstancer("game", &element_instancer_game);

	DecoratorInstancerStarfield decorator_instancer_starfield;
	DecoratorInstancerDefender decorator_instancer_defender;
	Rml::Factory::RegisterDecoratorInstancer("starfield", &decorator_instancer_starfield);
	Rml::Factory::RegisterDecoratorInstancer("defender", &decorator_instancer_defender);

	// Construct the game singletons.
	HighScores::Initialise(context);

	// Initialise the event listener instancer and handlers.
	EventListenerInstancer event_listener_instancer;
	Rml::Factory::RegisterEventListenerInstancer(&event_listener_instancer);

	EventManager::RegisterEventHandler("start_game", Rml::MakeUnique<EventHandlerStartGame>());
	EventManager::RegisterEventHandler("high_score", Rml::MakeUnique<EventHandlerHighScore>());
	EventManager::RegisterEventHandler("options", Rml::MakeUnique<EventHandlerOptions>());

	// Start the game.
	bool running = (EventManager::LoadWindow("background") && EventManager::LoadWindow("main_menu"));

	while (running)
	{
		running = Backend::ProcessEvents(context, &Shell::ProcessKeyDownShortcuts);

		context->Update();

		Backend::BeginFrame();
		context->Render();
		Backend::PresentFrame();
	}

	// Shut down the game singletons.
	HighScores::Shutdown();

	// Release the event handlers.
	EventManager::Shutdown();

	// Shutdown RmlUi.
	Rml::Shutdown();

	Shell::Shutdown();
	Backend::Shutdown();

	return 0;
}
