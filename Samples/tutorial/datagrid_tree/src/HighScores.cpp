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
#include <RmlUi/Core/StringUtilities.h>
#include <RmlUi/Core/TypeConverter.h>
#include <RmlUi/Core.h>
#include <stdio.h>

HighScores* HighScores::instance = nullptr;

HighScores::HighScores() : Rml::DataSource("high_scores")
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

void HighScores::GetRow(Rml::StringList& row, const Rml::String& table, int row_index, const Rml::StringList& columns)
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
				row.push_back(Rml::CreateString(32, "%d", scores[row_index].score));
			}
			else if (columns[i] == "colour")
			{
				Rml::String colour_string;
				Rml::TypeConverter< Rml::Colourb, Rml::String >::Convert(scores[row_index].colour, colour_string);
				row.push_back(colour_string);
			}
			else if (columns[i] == "wave")
			{
				row.push_back(Rml::CreateString(8, "%d", scores[row_index].wave));
			}
		}
	}
}

int HighScores::GetNumRows(const Rml::String& table)
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

void HighScores::SubmitScore(const Rml::String& name, const Rml::Colourb& colour, int wave, int score, int alien_kills[])
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

			NotifyRowAdd("scores", (int)i, 1);

			return;
		}
	}
}

void HighScores::LoadScores()
{
	// Open and read the high score file.
	Rml::FileInterface* file_interface = Rml::GetFileInterface();
	Rml::FileHandle scores_file = file_interface->Open("tutorial/datagrid_tree/data/high_score.txt");
	
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

			Rml::StringList score_lines;
			Rml::StringUtilities::ExpandString(score_lines, buffer, '\n');
			delete[] buffer;

			for (size_t i = 0; i < score_lines.size(); i++)
			{
				Rml::StringList score_parts;
				Rml::StringUtilities::ExpandString(score_parts, score_lines[i], '\t');
				if (score_parts.size() == 4 + NUM_ALIEN_TYPES)
				{
					Rml::Colourb colour;
					int wave;
					int score;
					int alien_kills[NUM_ALIEN_TYPES];

					if (Rml::TypeConverter< Rml::String , Rml::Colourb >::Convert(score_parts[1], colour) &&
						Rml::TypeConverter< Rml::String, int >::Convert(score_parts[2], wave) &&
						Rml::TypeConverter< Rml::String, int >::Convert(score_parts[3], score))
					{
						for (int j = 0; j < NUM_ALIEN_TYPES; j++)
						{
							if (!Rml::TypeConverter< Rml::String, int >::Convert(score_parts[4 + j], alien_kills[j]))
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
