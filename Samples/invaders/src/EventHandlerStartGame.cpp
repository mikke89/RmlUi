#include "EventHandlerStartGame.h"
#include "EventManager.h"
#include "GameDetails.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/ElementUtilities.h>
#include <RmlUi/Core/Event.h>

// The game's element context (declared in main.cpp).
extern Rml::Context* context;

EventHandlerStartGame::EventHandlerStartGame() {}

EventHandlerStartGame::~EventHandlerStartGame() {}

void EventHandlerStartGame::ProcessEvent(Rml::Event& event, const Rml::String& value)
{
	if (value == "start")
	{
		// Set the difficulty.
		Rml::String difficulty = event.GetParameter<Rml::String>("difficulty", "easy");
		if (difficulty == "hard")
			GameDetails::SetDifficulty(GameDetails::HARD);
		else
			GameDetails::SetDifficulty(GameDetails::EASY);

		// Set the defender colour.
		Rml::StringList colour_components;
		Rml::StringUtilities::ExpandString(colour_components, event.GetParameter<Rml::String>("colour", "255,255,255"));

		Rml::Colourb colour(255, 255, 255);
		for (size_t i = 0; i < colour_components.size() && i < 3; ++i)
		{
			int colour_component = atoi(colour_components[i].c_str());
			if (colour_component < 0)
				colour_component = 0;
			if (colour_component > 255)
				colour_component = 255;

			colour[i] = (Rml::byte)colour_component;
		}

		GameDetails::SetDefenderColour(colour);

		// Go to the game window and close the start game window.
		if (EventManager::LoadWindow("game"))
		{
			Rml::ElementUtilities::GetElementById(context->GetFocusElement(), "game")->Focus();
			event.GetTargetElement()->GetOwnerDocument()->Close();
		}
	}
}
