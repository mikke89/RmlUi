#pragma once

#include <RmlUi/Core/Types.h>

class Game;

const int NUM_SHIELD_CELLS = 6;
const int PIXEL_SIZE = 4;

/**
    A shield, protecting the player.
 */

class Shield {
public:
	enum ShieldType { REGULAR, TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT };

	/// Construct the invader
	Shield(Game* game, ShieldType type);
	~Shield();

	/// Sets up which cells are on and off, based on the type.
	void InitialiseCells();

	/// Set the shield's screen position
	/// @param position Position in screen space
	void SetPosition(const Rml::Vector2f& position);
	/// Get the current shield position
	/// @returns The shield's position in screen space
	const Rml::Vector2f& GetPosition() const;

	/// Render the shield.
	void Render(Rml::RenderManager& render_manager, float dp_ratio);

	/// Returns true if the position hits the shield
	/// If a hit is detected, will degrade the shield.
	/// @param position Position to do the hit check at
	/// @returns If the shield was hit
	bool CheckHit(const Rml::Vector2f& position);

protected:
	void SustainDamage();

	// Game this shield is in
	Game* game;

	// The invader type we represent
	ShieldType type;
	// The current position in screen space of the shield
	Rml::Vector2f position;

	// Our current state - starts at 4, degrades once per hit.
	int health;

	enum ShieldCellState { ON, OFF, DESTROYED };
	ShieldCellState shield_cells[NUM_SHIELD_CELLS][NUM_SHIELD_CELLS];
};
