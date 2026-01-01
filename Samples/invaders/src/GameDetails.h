#pragma once

#include <RmlUi/Core/Types.h>

class GameDetails {
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
	static void SetDefenderColour(const Rml::Colourb& colour);
	/// Returns the player's ship colour.
	/// @return The colour of the player's ship.
	static const Rml::Colourb& GetDefenderColour();

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
