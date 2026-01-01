#pragma once

#include <RmlUi/Core/Types.h>

class Game;
class Sprite;

/**
    Represents the Earth defender. Stores position and performs the update of the bullet position and collision detection.
 */

class Defender {
public:
	Defender(Game* game);
	~Defender();

	/// Update the defender state.
	void Update(double t);
	/// Render the defender.
	void Render(Rml::RenderManager& render_manager, float dp_ratio, Rml::Texture texture);

	/// Move the defender left.
	void StartMove(float direction);
	/// Stop the movement.
	void StopMove(float direction);
	/// Fire a bullet (if one isn't already in flight).
	void Fire();

	/// Check if an object at the given position would hit the defender.
	bool CheckHit(double t, const Rml::Vector2f& position);

private:
	/// Update the bullet, doing collision detection.
	void UpdateBullet(double t);

	Game* game;
	Rml::Vector2f position;

	float move_direction;

	bool bullet_in_flight;
	Rml::Vector2f bullet_position;

	double defender_frame_start;
	double respawn_start;

	bool render;

	enum State { ALIVE, RESPAWN };
	State state;
};
