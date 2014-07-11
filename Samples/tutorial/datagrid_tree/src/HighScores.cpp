/*
 * Copyright (c) 2006 - 2008
 * Wandering Monster Studios Limited
 *
 * Any use of this program is governed by the terms of Wandering Monster
 * Studios Limited's Licence Agreement included with this program, a copy
 * of which can be obtained by contacting Wandering Monster Studios
 * Limited at info@wanderingmonster.co.nz.
 *
 */

#include "HighScores.h"
#include <Rocket/Core/StringUtilities.h>
#include <Rocket/Core/TypeConverter.h>
#include <Rocket/Core.h>
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

void HighScores::SubmitScore(const Rocket::Core::String& name, const Rocket::Core::Colourb& colour, int wave, int score, int alien_kills[])
{
	for (size_t i = 0; i < NUM_SCORES; i++)
	{
		if (score > scores[i].score)
		{
			// Push down all the other scores.
			for (size_t j = NUM_SCORES - 1; j > i; j--)
			{
				scores[j] = scores[j - 1];
			}

			// Insert our new score.
			scores[i].name = name;
			scores[i].colour = colour;
			scores[i].wave = wave;
			scores[i].score = score;

			for (int j = 0; j < NUM_ALIEN_TYPES; j++)
			{
				scores[i].alien_kills[j] = alien_kills[j];
			}

			NotifyRowAdd("scores", i, 1);

			return;
		}
	}
}

void HighScores::LoadScores()
{
	// Open and read the high score file.
	Rocket::Core::FileInterface* file_interface = Rocket::Core::GetFileInterface();
	Rocket::Core::FileHandle scores_file = file_interface->Open("high_score.txt");
	
	if (scores_file)
	{
		file_interface->Seek(scores_file, 0, SEEK_END);
		size_t scores_length = file_interface->Tell(scores_file);
		file_interface->Seek(scores_file, 0, SEEK_SET);

		if (scores_length > 0)
		{
			char* buffer = new char[scores_length + 1];
			file_interface->Read(buffer, scores_length, scores_file);
			file_interface->Close(scores_file);
			buffer[scores_length] = 0;

			Rocket::Core::StringList score_lines;
			Rocket::Core::StringUtilities::ExpandString(score_lines, buffer, '\n');
			delete[] buffer;

			for (size_t i = 0; i < score_lines.size(); i++)
			{
				Rocket::Core::StringList score_parts;
				Rocket::Core::StringUtilities::ExpandString(score_parts, score_lines[i], '\t');
				if (score_parts.size() == 4 + NUM_ALIEN_TYPES)
				{
					Rocket::Core::Colourb colour;
					int wave;
					int score;
					int alien_kills[NUM_ALIEN_TYPES];

					if (Rocket::Core::TypeConverter< Rocket::Core::String , Rocket::Core::Colourb >::Convert(score_parts[1], colour) &&
						Rocket::Core::TypeConverter< Rocket::Core::String, int >::Convert(score_parts[2], wave) &&
						Rocket::Core::TypeConverter< Rocket::Core::String, int >::Convert(score_parts[3], score))
					{
						for (int j = 0; j < NUM_ALIEN_TYPES; j++)
						{
							if (!Rocket::Core::TypeConverter< Rocket::Core::String, int >::Convert(score_parts[4 + j], alien_kills[j]))
							{
								break;
							}
						}

						SubmitScore(score_parts[0], colour, wave, score, alien_kills);
					}
				}
			}
		}
	}
}
