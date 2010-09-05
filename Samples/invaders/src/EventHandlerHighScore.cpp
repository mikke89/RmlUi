/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#include "EventHandlerHighScore.h"
#include <Rocket/Core/ElementDocument.h>
#include <Rocket/Core/ElementUtilities.h>
#include <Rocket/Core/Input.h>
#include "EventManager.h"
#include "GameDetails.h"
#include "HighScores.h"

EventHandlerHighScore::EventHandlerHighScore()
{
}

EventHandlerHighScore::~EventHandlerHighScore()
{
}

void EventHandlerHighScore::ProcessEvent(Rocket::Core::Event& event, const Rocket::Core::String& value)
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
		if (event.GetParameter< int >("key_identifier", Rocket::Core::Input::KI_UNKNOWN) == Rocket::Core::Input::KI_RETURN)
		{
			Rocket::Core::String name = event.GetCurrentElement()->GetAttribute< Rocket::Core::String >("value", "Anon.");
			HighScores::SubmitName(name);
		}
	}
	else if (value == "check_input")
	{
		Rocket::Core::Element* name_input_field = event.GetTargetElement()->GetElementById("player_input");
		if (name_input_field)
		{
			name_input_field->Focus();
		}
	}
	else if (value == "check_name")
	{
		/* TODO: Check if the player hasn't set their name first. */
		HighScores::SubmitName("Anon.");
	}
}
