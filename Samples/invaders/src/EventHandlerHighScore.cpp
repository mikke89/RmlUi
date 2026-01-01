#include "EventHandlerHighScore.h"
#include "EventManager.h"
#include "GameDetails.h"
#include "HighScores.h"
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/ElementUtilities.h>
#include <RmlUi/Core/Input.h>

EventHandlerHighScore::EventHandlerHighScore() {}

EventHandlerHighScore::~EventHandlerHighScore() {}

void EventHandlerHighScore::ProcessEvent(Rml::Event& event, const Rml::String& value)
{
	if (value == "add_score")
	{
		int score = GameDetails::GetScore();
		if (score > 0)
		{
			// Submit the score the player just got to the high scores chart.
			HighScores::SubmitScore(GameDetails::GetDefenderColour(), GameDetails::GetWave(), GameDetails::GetScore());
			// Reset the score so the chart won't get confused next time we enter.
			GameDetails::ResetScore();
		}
	}
	else if (value == "enter_name")
	{
		if (event.GetParameter<int>("key_identifier", Rml::Input::KI_UNKNOWN) == Rml::Input::KI_RETURN)
		{
			Rml::String name = event.GetCurrentElement()->GetAttribute<Rml::String>("value", "Anon.");
			HighScores::SubmitName(name);
		}
	}
	else if (value == "check_name")
	{
		Rml::String name = "Anon.";

		// Submit the name the user started typing
		if (auto element = event.GetCurrentElement()->GetOwnerDocument()->GetElementById("player_input"))
			name = element->GetAttribute<Rml::String>("value", name);

		HighScores::SubmitName(name);
	}
}
