/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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

#ifndef RMLUI_LUAINVADERS_SHIELD_H
#define RMLUI_LUAINVADERS_SHIELD_H

#include <RmlUi/Core/Types.h>

class Game;

const int NUM_SHIELD_CELLS = 6;
const int PIXEL_SIZE = 4;

/**
	A shield, protecting the player.

	@author Robert Curry
 */

class Shield
{
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
	void Render(float dp_ratio);

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

#endif
