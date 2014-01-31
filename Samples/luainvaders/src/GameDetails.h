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

#ifndef ROCKETINVADERSGAMEDETAILS_H
#define ROCKETINVADERSGAMEDETAILS_H

#include <Rocket/Core/Types.h>

/**
	@author Peter Curry
 */

class GameDetails
{
public:
	enum Difficulty { EASY, HARD };

	/// Sets the game difficulty.
	/// @param[in] difficulty The new game difficulty.
	static void SetDifficulty(Difficulty difficulty);
	/// Returns the game difficulty.
	/// @return The current game difficulty.
	static Difficulty GetDifficulty();

	/// Sets the colour of the player's ship.
	/// @param[in] colour The new ship colour.
	static void SetDefenderColour(const Rocket::Core::Colourb& colour);
	/// Returns the player's ship colour.
	/// @return The colour of the player's ship.
	static const Rocket::Core::Colourb& GetDefenderColour();

	/// Sets the score the player achieved in the last game.
	/// @param[in] score The player's score.
	static void SetScore(int score);
	/// Resets the player's score.
	static void ResetScore();
	/// Returns the score the player achieved in the last game.
	/// @return The player's score.
	static int GetScore();

	/// Sets the current wave number
	/// @param[in] wave The wave number
	static void SetWave(int wave);
	/// Get the current wave number
	/// @return The wave number
	static int GetWave();

	/// Sets the pauses state of the game
	/// @param[in] paused Whether the game is paused or not
	static void SetPaused(bool paused);
	/// Gets if the game is paused or not
	/// @return True if the game is paused, false otherwise.
	static bool GetPaused();

	enum GraphicsQuality { GOOD, OK, BAD };

	/// Sets the quality of the graphics used in-game.
	/// @param[in] quality The new graphics quality.
	static void SetGraphicsQuality(GraphicsQuality quality);
	/// Returns the quality of the graphics to use in-game.
	/// @return The currently-set graphical quality.
	static GraphicsQuality GetGraphicsQuality();

	/// Enables or disables reverb.
	/// @param[in] enabled True to enable reverb, false to disable.
	static void SetReverb(bool enabled);
	/// Returns the current state of reverb.
	/// @return The current reverb state.
	static bool GetReverb();

	/// Enables or disables 3D spatialisation in the audio engine.
	/// @param[in] enabled True to enable spatialisation, false to disable.
	static void Set3DSpatialisation(bool enabled);
	/// Returns the current state of audio spatialisation.
	/// @return The current audio spatialisation state.
	static bool Get3DSpatialisation();

private:
	GameDetails();
	~GameDetails();
};

#endif
