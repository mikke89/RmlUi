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

#include "Defender.h"
#include "Game.h"
#include "GameDetails.h"
#include "Invader.h"
#include "Mothership.h"
#include "Shield.h"
#include "Sprite.h"
#include <Shell.h>

const float UPDATE_FREQ = 0.01f;
const float MOVEMENT_SPEED = 300;
const float BULLET_SPEED = 15;
const int SPRITE_WIDTH = 64;
const float RESPAWN_TIME = 1.0f;

Sprite defender_sprite(Rml::Vector2f(60, 31), Rml::Vector2f(0, 0.5), Rml::Vector2f(0.23437500, 0.98437500));
Sprite bullet_sprite(Rml::Vector2f(4, 20), Rml::Vector2f(0.4921875, 0.515625), Rml::Vector2f(0.5078125, 0.828125));
Sprite explosion_sprite(Rml::Vector2f(52, 28), Rml::Vector2f(0.71484375f, 0.51562500f), Rml::Vector2f(0.91796875f, 0.95312500f));

Defender::Defender(Game* _game)
{
	move_direction = 0;
	defender_frame_start = 0;
	bullet_in_flight = false;
	respawn_start = 0;
	game = _game;
	position.x = game->GetWindowDimensions().x / 2;
	position.y = game->GetWindowDimensions().y - 50;
	state = ALIVE;
	render = true;
}

Defender::~Defender() {}

void Defender::Update(double t)
{
	float dt = float(t - defender_frame_start);
	if (dt < UPDATE_FREQ)
		return;

	dt = Rml::Math::Min(dt, 0.1f);

	defender_frame_start = t;

	position.x += (move_direction * dt * MOVEMENT_SPEED);

	if (position.x < 5)
		position.x = 5;
	else if (position.x > (game->GetWindowDimensions().x - SPRITE_WIDTH - 5))
		position.x = game->GetWindowDimensions().x - SPRITE_WIDTH - 5;

	UpdateBullet(t);

	if (state == RESPAWN)
	{
		// Switch the render flag so the defender "flickers"
		render = !render;

		// Check if we should switch back to our alive state
		if (float(t - respawn_start) > RESPAWN_TIME)
		{
			state = ALIVE;
			render = true;
		}
	}
}

void Defender::Render(float dp_ratio, Rml::TextureHandle texture)
{
	Rml::Colourb color = GameDetails::GetDefenderColour();

	// Render our sprite if rendering is enabled
	if (render)
		defender_sprite.Render(position, dp_ratio, color, texture);

	// Render the bullet
	if (bullet_in_flight)
		bullet_sprite.Render(bullet_position, dp_ratio, color, texture);
}

void Defender::StartMove(float direction)
{
	move_direction = direction > 0.0f ? 1.0f : -1.0f;
}

void Defender::StopMove(float direction)
{
	float stop_direction = direction > 0.0f ? 1.0f : -1.0f;
	if (stop_direction == move_direction)
		move_direction = 0.0f;
}

void Defender::Fire()
{
	if (!bullet_in_flight)
	{
		bullet_position = position + Rml::Vector2f((SPRITE_WIDTH / 2) - 4, 0);
		bullet_in_flight = true;
	}
}

bool Defender::CheckHit(double t, const Rml::Vector2f& check_position)
{
	float sprite_width = defender_sprite.dimensions.x;
	float sprite_height = defender_sprite.dimensions.y;

	// If the position is within our bounds, set ourselves
	// as exploding and return a valid hit.
	if (state == ALIVE && check_position.x >= position.x && check_position.x <= position.x + sprite_width && check_position.y >= position.y &&
		check_position.y <= position.y + sprite_height)
	{
		game->RemoveLife();
		state = RESPAWN;
		respawn_start = t;

		return true;
	}

	return false;
}

void Defender::UpdateBullet(double t)
{
	if (bullet_in_flight)
	{
		// Move the bullet up and mark it dead if it flies off the top of the screen
		bullet_position.y -= BULLET_SPEED;
		if (bullet_position.y < 0)
		{
			bullet_in_flight = false;
			return;
		}

		// Check if we hit the shields
		for (int i = 0; i < game->GetNumShields(); i++)
		{
			if (game->GetShield(i)->CheckHit(bullet_position))
			{
				bullet_in_flight = false;
				return;
			}
		}

		// Check if we hit any invaders
		for (int i = 0; i < game->GetNumInvaders(); i++)
		{
			if (game->GetInvader(i)->CheckHit(t, bullet_position))
			{
				bullet_in_flight = false;
				return;
			}
		}
	}
}
