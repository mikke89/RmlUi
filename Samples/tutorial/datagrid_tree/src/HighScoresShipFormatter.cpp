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

#include "HighScoresShipFormatter.h"
#include <RmlUi/Core/TypeConverter.h>

HighScoresShipFormatter::HighScoresShipFormatter() : Rml::DataFormatter("ship")
{
}

HighScoresShipFormatter::~HighScoresShipFormatter()
{
}

void HighScoresShipFormatter::FormatData(Rml::String& formatted_data, const Rml::StringList& raw_data)
{
	// Data format:
	// raw_data[0] is the colour, in "%d, %d, %d, %d" format.

	Rml::Colourb ship_colour;
	Rml::TypeConverter< Rml::String, Rml::Colourb >::Convert(raw_data[0], ship_colour);

	Rml::String colour_string = Rml::CreateString(32, "%d,%d,%d", ship_colour.red, ship_colour.green, ship_colour.blue);

	formatted_data = "<defender style=\"color: rgb(" + colour_string + ");\" />";
}
