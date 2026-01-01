#pragma once

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
	void Render(Rml::RenderManager& render_manager, float dp_ratio);

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
