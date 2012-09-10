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

#ifndef HIGHSCORESSHIPFORMATTER_H
#define HIGHSCORESSHIPFORMATTER_H

#include <Rocket/Controls/DataFormatter.h>

/**
	Formats the colour of the player's ship to a <defender> tag, which is linked to the defender decorator.
	@author Robert Curry
 */

class HighScoresShipFormatter : public Rocket::Controls::DataFormatter
{
	public:
		HighScoresShipFormatter();
		~HighScoresShipFormatter();

		void FormatData(Rocket::Core::String& formatted_data, const Rocket::Core::StringList& raw_data);
};

#endif
