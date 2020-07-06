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

#ifndef HIGHSCORES_H
#define HIGHSCORES_H

#include <RmlUi/Core/Types.h>
#include <RmlUi/Core/Elements/DataSource.h>

const int NUM_SCORES = 10;
const int NUM_ALIEN_TYPES = 3;

/**
	This class stores a list of high scores, and loads the high scores list from a file upon initialisation.
	@author Robert Curry
 */

class HighScores : public Rml::DataSource
{
public:
	static void Initialise();
	static void Shutdown();

	void GetRow(Rml::StringList& row, const Rml::String& table, int row_index, const Rml::StringList& columns);
	int GetNumRows(const Rml::String& table);

private:
	HighScores();
	~HighScores();

	static HighScores* instance;

	void SubmitScore(const Rml::String& name, const Rml::Colourb& colour, int wave, int score, int alien_kills[]);
	void LoadScores();

	struct Score
	{
		Rml::String name;
		Rml::Colourb colour;
		int score;
		int wave;

		int alien_kills[NUM_ALIEN_TYPES];
	};

	Score scores[NUM_SCORES];
};

#endif
