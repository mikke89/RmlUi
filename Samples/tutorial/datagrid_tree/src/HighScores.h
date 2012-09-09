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

#include <EMP/Core/Types.h>
#include <EMP/Core/DataSource.h>

const int NUM_SCORES = 10;
const int NUM_ALIEN_TYPES = 3;

/**
	This class stores a list of high scores, and loads the high scores list from a file upon initialisation.
	@author Robert Curry
 */

class HighScores : public EMP::Core::DataSource
{
public:
	static void Initialise();
	static void Shutdown();

	void GetRow(EMP::Core::StringList& row, const EMP::Core::String& table, int row_index, const EMP::Core::StringList& columns);
	int GetNumRows(const EMP::Core::String& table);

private:
	HighScores();
	~HighScores();

	static HighScores* instance;

	void SubmitScore(const EMP::Core::String& name, const EMP::Core::Colourb& colour, int wave, int score, int alien_kills[]);
	void LoadScores();

	struct Score
	{
		EMP::Core::String name;
		EMP::Core::Colourb colour;
		int score;
		int wave;

		int alien_kills[NUM_ALIEN_TYPES];
	};

	Score scores[NUM_SCORES];
};

#endif
