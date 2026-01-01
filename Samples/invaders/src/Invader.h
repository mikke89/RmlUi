#pragma once

#include <RmlUi/Core/Types.h>

class Game;

/**
    An alien invader.
 */

class Invader {
public:
	enum InvaderType { UNKNOWN, RANK1, RANK2, RANK3, MOTHERSHIP };
	enum BombType { NONE, RAY, MISSILE };

	/// Construct the invader
	Invader(Game* game, InvaderType type, int index);
	virtual ~Invader();

	/// Set the invaders screen position
	/// @param position Position in screen space
	void SetPosition(const Rml::Vector2f& position);
	/// Get the current invader position
	/// @returns The invaders position in screen space
	const Rml::Vector2f& GetPosition() const;

	/// Update the invader
	virtual void Update(double t);

	/// Render the invader
	void Render(Rml::RenderManager& render_manager, float dp_ratio, Rml::Texture texture);

	/// Update the invaders animation
	void UpdateAnimation();

	/// The current invaders state
	enum InvaderState { ALIVE, EXPLODING, DEAD };
	/// Get the current invader state
	/// @returns Invader state
	InvaderState GetState();

	/// Returns true if the position hits the invader
	/// If a hit is detected, will explode and start the death timer
	/// @param position Position to do the hit check at
	/// @returns If the invader was hit
	bool CheckHit(double t, const Rml::Vector2f& position);

protected:
	// Game this invader is in
	Game* game;
	// The index/id of this invader
	int invader_index;

	// The invader type we represent
	InvaderType type;
	// The current position in screen space of the invader
	Rml::Vector2f position;
	// Our current animation frame
	int animation_frame;

	// Our current state
	InvaderState state;

	// Our current in-flight bomb, or none. (may be not none if we're dead.)
	BombType bomb;
	// The current position of the bomb in screen space
	Rml::Vector2f bomb_position;
	// The animation frame the bomb is on
	int bomb_animation_frame;
	// When the last bomb update occured
	double bomb_frame_start;
	// Probability of us dropping a bomb - this is calculated
	// at construction time and based on our rank and the game
	// difficulty level
	float bomb_probability;

	// Time when we should die - 0, until we're hit
	double death_time;

	int GetSpriteIndex() const;
};
