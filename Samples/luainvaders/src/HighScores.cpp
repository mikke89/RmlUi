/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#include "HighScores.h"
#include <RmlUi/Core/TypeConverter.h>
#include <stdio.h>

HighScores* HighScores::instance = nullptr;

HighScores::HighScores() : Rml::Controls::DataSource("high_scores")
{
	RMLUI_ASSERT(instance == nullptr);
	instance = this;

	for (int i = 0; i < NUM_SCORES; i++)
	{
		scores[i].score = -1;
	}

	LoadScores();
}

HighScores::~HighScores()
{
	RMLUI_ASSERT(instance == this);

	SaveScores();

	instance = nullptr;
}

void HighScores::Initialise()
{
	new HighScores();
}

void HighScores::Shutdown()
{
	delete instance;
}

void HighScores::GetRow(Rml::Core::StringList& row, const Rml::Core::String& table, int row_index, const Rml::Core::StringList& columns)
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
				row.push_back(Rml::Core::CreateString(4, "%d", scores[row_index].name_required));
			}
			else if (columns[i] == "score")
			{
				row.push_back(Rml::Core::CreateString(32, "%d", scores[row_index].score));
			}
			else if (columns[i] == "colour")
			{
				Rml::Core::String colour_string;
				Rml::Core::TypeConverter< Rml::Core::Colourb, Rml::Core::String >::Convert(scores[row_index].colour, colour_string);
				row.push_back(colour_string);
			}
			else if (columns[i] == "wave")
			{
				row.push_back(Rml::Core::CreateString(8, "%d", scores[row_index].wave));
			}
		}
	}
}

int HighScores::GetNumRows(const Rml::Core::String& table)
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

void HighScores::SubmitScore(const Rml::Core::String& name, const Rml::Core::Colourb& colour, int wave, int score)
{
	instance->SubmitScore(name, colour, wave, score, false);
}

void HighScores::SubmitScore(const Rml::Core::Colourb& colour, int wave, int score)
{
	instance->SubmitScore("", colour, wave, score, true);
}

// Sets the name of the last player to submit their score.
void HighScores::SubmitName(const Rml::Core::String& name)
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

void HighScores::SubmitScore(const Rml::Core::String& name, const Rml::Core::Colourb& colour, int wave, int score, bool name_required)
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
			Rml::Core::StringList score_parts;
			Rml::Core::StringUtilities::ExpandString(score_parts, Rml::Core::String(buffer), '\t');
			if (score_parts.size() == 4)
			{
				Rml::Core::Colourb colour;
				int wave;
				int score;

				if (Rml::Core::TypeConverter< Rml::Core::String , Rml::Core::Colourb >::Convert(score_parts[1], colour) &&
					Rml::Core::TypeConverter< Rml::Core::String, int >::Convert(score_parts[2], wave) &&
					Rml::Core::TypeConverter< Rml::Core::String, int >::Convert(score_parts[3], score))
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
			Rml::Core::String colour_string;
			Rml::Core::TypeConverter< Rml::Core::Colourb, Rml::Core::String >::Convert(scores[i].colour, colour_string);
      
			Rml::Core::String score = Rml::Core::CreateString(1024, "%s\t%s\t%d\t%d\n", scores[i].name.c_str(), colour_string.c_str(), scores[i].wave, scores[i].score);
			fputs(score.c_str(), scores_file);		
		}

		fclose(scores_file);
	}
}
