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

#ifndef ROCKETINVADERSGAME_H
#define ROCKETINVADERSGAME_H

#include <Rocket/Core/Types.h>
#include <Rocket/Core/Texture.h>

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

class Game
{
public:
	Game();
	~Game();

	/// Initialise a new game
	void Initialise();

	/// Update the game
	void Update();

	/// Render the game
	void Render();

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
	/// Checks if the game is over
	bool IsGameOver() const;

	/// Get the dimensions of the game window.
	const Rocket::Core::Vector2f GetWindowDimensions();	

private:

	// The current invaders
	Invader** invaders;
	// The direction they're moving
	float current_invader_direction;
	// Time of the last invader update
	Rocket::Core::Time invader_frame_start;
	// How often the invaders move
	Rocket::Core::Time invader_move_freq;
	// Is the game over
	bool game_over;

	// Helper function to move the invaders
	void MoveInvaders();

	// Our current defener
	Defender* defender;

	// Number of lives the defender has left
	int defender_lives;

	// The current shields
	Shield** shields;

	// Texture that contains the sprites
	Rocket::Core::TextureHandle texture;

	void InitialiseShields();
	void InitialiseWave();
	void OnGameOver();
};

#endif
