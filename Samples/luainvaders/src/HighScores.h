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

#ifndef ROCKETINVADERSHIGHSCORES_H
#define ROCKETINVADERSHIGHSCORES_H

#include <Rocket/Controls/DataSource.h>
#include <Rocket/Core/Types.h>

const int NUM_SCORES = 10;

class HighScores : public Rocket::Controls::DataSource
{
public:
	static void Initialise();
	static void Shutdown();

	void GetRow(Rocket::Core::StringList& row, const Rocket::Core::String& table, int row_index, const Rocket::Core::StringList& columns);
	int GetNumRows(const Rocket::Core::String& table);

	static int GetHighScore();

	/// Two functions to add a score to the chart.
	/// Adds a full score, including a name. This won't prompt the user to enter their name.
	static void SubmitScore(const Rocket::Core::String& name, const Rocket::Core::Colourb& colour, int wave, int score);
	/// Adds a score, and causes an input field to appear to request the user for their name.
	static void SubmitScore(const Rocket::Core::Colourb& colour, int wave, int score);
	/// Sets the name of the last player to submit their score.
	static void SubmitName(const Rocket::Core::String& name);

private:
	HighScores();
	~HighScores();

	static HighScores* instance;

	void SubmitScore(const Rocket::Core::String& name, const Rocket::Core::Colourb& colour, int wave, int score, bool name_required);
	void LoadScores();
	void SaveScores();

	struct Score
	{
		Rocket::Core::String name;
		bool name_required;
		Rocket::Core::Colourb colour;
		int score;
		int wave;
	};

	Score scores[NUM_SCORES];
};

#endif
