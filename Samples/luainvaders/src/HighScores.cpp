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

#include "HighScores.h"
#include <Rocket/Core/TypeConverter.h>
#include <stdio.h>

HighScores* HighScores::instance = NULL;

HighScores::HighScores() : Rocket::Controls::DataSource("high_scores")
{
	ROCKET_ASSERT(instance == NULL);
	instance = this;

	for (int i = 0; i < NUM_SCORES; i++)
	{
		scores[i].score = -1;
	}

	LoadScores();
}

HighScores::~HighScores()
{
	ROCKET_ASSERT(instance == this);

	SaveScores();

	instance = NULL;
}

void HighScores::Initialise()
{
	new HighScores();
}

void HighScores::Shutdown()
{
	delete instance;
}

void HighScores::GetRow(Rocket::Core::StringList& row, const Rocket::Core::String& table, int row_index, const Rocket::Core::StringList& columns)
{
	if (table == "scores")
	{
		for (size_t i = 0; i < columns.size(); i++)
		{
			if (columns[i] == "name")
			{
				row.push_back(scores[row_index].name);
			}
			else if (columns[i] == "name_required")
			{
				row.push_back(Rocket::Core::String(4, "%d", scores[row_index].name_required));
			}
			else if (columns[i] == "score")
			{
				row.push_back(Rocket::Core::String(32, "%d", scores[row_index].score));
			}
			else if (columns[i] == "colour")
			{
				Rocket::Core::String colour_string;
				Rocket::Core::TypeConverter< Rocket::Core::Colourb, Rocket::Core::String >::Convert(scores[row_index].colour, colour_string);
				row.push_back(colour_string);
			}
			else if (columns[i] == "wave")
			{
				row.push_back(Rocket::Core::String(8, "%d", scores[row_index].wave));
			}
		}
	}
}

int HighScores::GetNumRows(const Rocket::Core::String& table)
{
	if (table == "scores")
	{
		for (int i = 0; i < NUM_SCORES; i++)
		{
			if (scores[i].score == -1)
			{
				return i;
			}
		}

		return NUM_SCORES;
	}

	return 0;
}

int HighScores::GetHighScore()
{
	if (instance->GetNumRows("scores") == 0)
	{
		return 0;
	}

	return instance->scores[0].score;
}

void HighScores::SubmitScore(const Rocket::Core::String& name, const Rocket::Core::Colourb& colour, int wave, int score)
{
	instance->SubmitScore(name, colour, wave, score, false);
}

void HighScores::SubmitScore(const Rocket::Core::Colourb& colour, int wave, int score)
{
	instance->SubmitScore("", colour, wave, score, true);
}

// Sets the name of the last player to submit their score.
void HighScores::SubmitName(const Rocket::Core::String& name)
{
	for (int i = 0; i < instance->GetNumRows("scores"); i++)
	{
		if (instance->scores[i].name_required)
		{
			instance->scores[i].name = name;
			instance->scores[i].name_required = false;
			instance->NotifyRowChange("scores", i, 1);
		}
	}
}

void HighScores::SubmitScore(const Rocket::Core::String& name, const Rocket::Core::Colourb& colour, int wave, int score, bool name_required)
{
	for (int i = 0; i < NUM_SCORES; i++)
	{
		if (score > scores[i].score)
		{
			// If we've already got the maximum number of scores, then we have
			// to send a RowsRemoved message as we're going to delete the last
			// row from the data source.
			bool max_rows = scores[NUM_SCORES - 1].score != -1;

			// Push down all the other scores.
			for (int j = NUM_SCORES - 1; j > i; j--)
			{
				scores[j] = scores[j - 1];
			}

			// Insert our new score.
			scores[i].name = name;
			scores[i].colour = colour;
			scores[i].wave = wave;
			scores[i].score = score;
			scores[i].name_required = name_required;

			// Send the row removal message (if necessary).
			if (max_rows)
			{
				NotifyRowRemove("scores", NUM_SCORES - 1, 1);
			}

			// Then send the rows added message.
			NotifyRowAdd("scores", i, 1);

			return;
		}
	}
}

void HighScores::LoadScores()
{
	// Open and read the high score file.
	FILE* scores_file = fopen("scores.txt", "rt");

	if (scores_file)
	{
		char buffer[1024];
		while (fgets(buffer, 1024, scores_file))
		{
			Rocket::Core::StringList score_parts;
			Rocket::Core::StringUtilities::ExpandString(score_parts, Rocket::Core::String(buffer), '\t');
			if (score_parts.size() == 4)
			{
				Rocket::Core::Colourb colour;
				int wave;
				int score;

				if (Rocket::Core::TypeConverter< Rocket::Core::String , Rocket::Core::Colourb >::Convert(score_parts[1], colour) &&
					Rocket::Core::TypeConverter< Rocket::Core::String, int >::Convert(score_parts[2], wave) &&
					Rocket::Core::TypeConverter< Rocket::Core::String, int >::Convert(score_parts[3], score))
				{
					SubmitScore(score_parts[0], colour, wave, score);
				}
			}
		}

		fclose(scores_file);
	}
}

void HighScores::SaveScores()
{
	FILE* scores_file = fopen("scores.txt", "wt");

	if (scores_file)
	{
		for (int i = 0; i < GetNumRows("scores"); i++)
		{
			Rocket::Core::String colour_string;
			Rocket::Core::TypeConverter< Rocket::Core::Colourb, Rocket::Core::String >::Convert(scores[i].colour, colour_string);

			Rocket::Core::String score(1024, "%s\t%s\t%d\t%d\n", scores[i].name.CString(), colour_string.CString(), scores[i].wave, scores[i].score);
			fputs(score.CString(), scores_file);		
		}

		fclose(scores_file);
	}
}
