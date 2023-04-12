/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
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

#ifndef RMLUI_INVADERS_GAME_H
#define RMLUI_INVADERS_GAME_H

#include <RmlUi/Core/Texture.h>
#include <RmlUi/Core/Types.h>

class Shield;
class Invader;
class Defender;
class Mothership;

/**
    Runs the game.
    - Updates the Invader positions, animations and bombs.
    - Updates the player position and bullets

    @author Lloyd Weehuizen
 */

class Game {
public:
	Game();
	~Game();

	/// Initialise a new game
	void Initialise();

	/// Update the game
	void Update(double t);

	/// Render the game
	void Render(float dp_ratio);

	/// Access the defender
	Defender* GetDefender();
	/// Access the invaders
	Invader* GetInvader(int index);
	/// Get the total number of invaders
	int GetNumInvaders();

	/// Returns true if the given invader can drop bomb
	bool CanDropBomb(int invader_index);

	/// Access the shields.
	Shield* GetShield(int index);
	/// Get the total number of shields
	int GetNumShields();

	/// Adds a score onto the player's score.
	void AddScore(int score);
	/// Sets the player's score.
	void SetScore(int score);
	/// Sets the player's high-score.
	void SetHighScore(int score);
	/// Set the number of defender lives.
	void SetLives(int lives);
	/// Set the current wave
	void SetWave(int wave);
	/// Remove a defender life
	void RemoveLife();

	/// Get the dimensions of the game window.
	const Rml::Vector2f GetWindowDimensions();

private:
	bool initialized;
	// The current invaders
	Invader** invaders;
	// The direction they're moving
	float current_invader_direction;
	// Time of the last invader update
	double invader_frame_start;
	// How often the invaders move
	double invader_move_freq;

	// Helper function to move the invaders
	void MoveInvaders();

	// Our current defener
	Defender* defender;

	// Number of lives the defender has left
	int defender_lives;

	// The current shields
	Shield** shields;

	// Texture that contains the sprites
	Rml::Texture texture;

	void InitialiseShields();
	void InitialiseWave();
	void OnGameOver();
};

#endif
