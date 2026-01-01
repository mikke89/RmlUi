#include "DecoratorDefender.h"
#include "DecoratorStarfield.h"
#include "ElementGame.h"
#include "HighScores.h"
#include "LuaInterface.h"
#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include <RmlUi/Lua.h>
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
	int window_width = 1024;
	int window_height = 768;

	// Initializes the shell which provides common functionality used by the included samples.
	if (!Shell::Initialize())
		return -1;

	// Constructs the system and render interfaces, creates a window, and attaches the renderer.
	if (!Backend::Initialize("RmlUi Invaders from Mars (Lua Powered)", window_width, window_height, false))
	{
		Shell::Shutdown();
		return -1;
	}

	// Install the custom interfaces constructed by the backend before initializing RmlUi.
	Rml::SetSystemInterface(Backend::GetSystemInterface());
	Rml::SetRenderInterface(Backend::GetRenderInterface());

	// RmlUi initialisation.
	Rml::Initialise();

	// Initialise the Lua interface
	Rml::Lua::Initialise();

	// Create the main RmlUi context.
	context = Rml::CreateContext("main", Rml::Vector2i(window_width, window_height));
	if (context == nullptr)
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

	// Register Invader's custom decorator instancers.
	DecoratorInstancerStarfield decorator_starfield;
	DecoratorInstancerDefender decorator_defender;
	Rml::Factory::RegisterDecoratorInstancer("starfield", &decorator_starfield);
	Rml::Factory::RegisterDecoratorInstancer("defender", &decorator_defender);

	// Construct the game singletons.
	HighScores::Initialise(context);

	// Fire off the startup script.
	LuaInterface::Initialise(Rml::Lua::Interpreter::GetLuaState()); // the tables/functions defined in the samples
	Rml::Lua::Interpreter::LoadFile(Rml::String("lua_invaders/lua/start.lua"));

	bool running = true;
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

	// Shutdown RmlUi.
	Rml::Shutdown();

	Backend::Shutdown();
	Shell::Shutdown();

	return 0;
}
