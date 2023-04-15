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

#include "Invader.h"
#include "Defender.h"
#include "Game.h"
#include "GameDetails.h"
#include "Shield.h"
#include "Sprite.h"
#include <RmlUi/Core/Math.h>
#include <Shell.h>

const float BOMB_UPDATE_FREQ = 0.04f;
const float BOMB_RAY_SPEED = 10;
const float BOMB_MISSILE_SPEED = 7;
const float BOMB_PROBABILITY_EASY = 0.002f;
const float BOMB_PROBABILITY_HARD = 0.005f;
const float EXPLOSION_TIME = 0.25f;
const Rml::Colourb MOTHERSHIP_COLOUR = Rml::Colourb(255, 0, 0, 255);

Sprite invader_sprites[] = {
	// Rank 1
	Sprite(Rml::Vector2f(48, 32), Rml::Vector2f(0.609375f, 0), Rml::Vector2f(0.796875f, 0.5f)),
	Sprite(Rml::Vector2f(48, 32), Rml::Vector2f(0.80078125f, 0), Rml::Vector2f(0.98828125f, 0.5f)),
	// Rank 2
	Sprite(Rml::Vector2f(44, 32), Rml::Vector2f(0.2578125f, 0), Rml::Vector2f(0.4296875f, 0.5f)),
	Sprite(Rml::Vector2f(44, 32), Rml::Vector2f(0.43359375f, 0), Rml::Vector2f(0.60546875f, 0.5f)),
	// Rank 3
	Sprite(Rml::Vector2f(32, 32), Rml::Vector2f(0, 0), Rml::Vector2f(0.125f, 0.5f)),
	Sprite(Rml::Vector2f(32, 32), Rml::Vector2f(0.12890625f, 0), Rml::Vector2f(0.25390625f, 0.5f)),
	// Mothership
	Sprite(Rml::Vector2f(64, 28), Rml::Vector2f(0.23828125f, 0.515625f), Rml::Vector2f(0.48828125f, 0.953125f)),
	// Explosion
	Sprite(Rml::Vector2f(52, 28), Rml::Vector2f(0.71484375f, 0.51562500f), Rml::Vector2f(0.91796875f, 0.95312500f))};

Sprite bomb_sprites[] = {
	// Ray
	Sprite(Rml::Vector2f(12, 20), Rml::Vector2f(0.51171875f, 0.51562500f), Rml::Vector2f(0.55859375f, 0.82812500f)),
	Sprite(Rml::Vector2f(12, 20), Rml::Vector2f(0.56250000, 0.51562500), Rml::Vector2f(0.60937500, 0.82812500)),
	Sprite(Rml::Vector2f(12, 20), Rml::Vector2f(0.61328125, 0.51562500), Rml::Vector2f(0.66015625, 0.82812500)),
	Sprite(Rml::Vector2f(12, 20), Rml::Vector2f(0.66406250, 0.51562500), Rml::Vector2f(0.71093750, 0.82812500)),
	// Missile
	Sprite(Rml::Vector2f(12, 20), Rml::Vector2f(0.92578125, 0.51562500), Rml::Vector2f(0.97265625, 0.82812500))};

Invader::Invader(Game* _game, InvaderType _type, int _index) : position(0, 0)
{
	type = UNKNOWN;
	animation_frame = 0;
	bomb_animation_frame = 0;
	bomb_frame_start = 0;
	death_time = 0;
	state = ALIVE;
	bomb = NONE;
	game = _game;
	type = _type;
	death_time = 0;
	invader_index = _index;

	bomb_probability = GameDetails::GetDifficulty() == GameDetails::EASY ? BOMB_PROBABILITY_EASY : BOMB_PROBABILITY_HARD;
	bomb_probability *= float(type);
}

Invader::~Invader() {}

void Invader::SetPosition(const Rml::Vector2f& _position)
{
	position = _position;
}

const Rml::Vector2f& Invader::GetPosition() const
{
	return position;
}

void Invader::Update(double t)
{
	// Update the bombs
	if (float(t - bomb_frame_start) > BOMB_UPDATE_FREQ)
	{
		// Update the bomb position if its in flight, or check if we should drop one
		if (bomb != NONE)
		{
			if (bomb == RAY)
			{
				bomb_animation_frame++;
				if (bomb_animation_frame > 3)
					bomb_animation_frame = 0;

				bomb_position.y += BOMB_RAY_SPEED;
			}
			else
			{
				bomb_position.y += BOMB_MISSILE_SPEED;
			}

			if (bomb_position.y > game->GetWindowDimensions().y)
				bomb = NONE;

			// Check if we hit the shields
			for (int i = 0; i < game->GetNumShields(); i++)
			{
				if (game->GetShield(i)->CheckHit(bomb_position))
				{
					bomb = NONE;
					break;
				}
			}

			// Check if we hit the defender
			if (bomb != NONE)
			{
				if (game->GetDefender()->CheckHit(t, bomb_position))
					bomb = NONE;
			}
		}
		else if (state == ALIVE && Rml::Math::RandomReal(1.0f) < bomb_probability && game->CanDropBomb(invader_index))
		{
			bomb = Rml::Math::RandomInteger(2) == 0 ? RAY : MISSILE;
			bomb_position = position;
			bomb_position.x += invader_sprites[GetSpriteIndex()].dimensions.x / 2;

			if (bomb == RAY)
				bomb_animation_frame = 0;
			else
				bomb_animation_frame = 4;
		}

		bomb_frame_start = t;
	}

	if (state == EXPLODING && t - death_time > 0.0)
		state = DEAD;
}

void Invader::UpdateAnimation()
{
	switch (state)
	{
	case ALIVE:
		animation_frame++;
		if (animation_frame > 1)
			animation_frame = 0;
		break;

	default: break;
	}
}

void Invader::Render(float dp_ratio, Rml::TextureHandle texture)
{
	Rml::Colourb color(255);

	if (type == MOTHERSHIP)
		color = MOTHERSHIP_COLOUR;

	int sprite_index = GetSpriteIndex();
	int sprite_offset = int((invader_sprites[sprite_index].dimensions.x - 48) / 2);

	if (state != DEAD)
		invader_sprites[sprite_index].Render(Rml::Vector2f(position.x - sprite_offset, position.y), dp_ratio, color, texture);

	if (bomb != NONE)
		bomb_sprites[bomb_animation_frame].Render(bomb_position, dp_ratio, color, texture);
}

Invader::InvaderState Invader::GetState()
{
	return state;
}

bool Invader::CheckHit(double t, const Rml::Vector2f& check_position)
{
	// Get the sprite index we're currently using for collision detection
	int sprite_index = GetSpriteIndex();
	int sprite_offset = int((invader_sprites[sprite_index].dimensions.x - 48) / 2);
	float sprite_width = invader_sprites[sprite_index].dimensions.x;
	float sprite_height = invader_sprites[sprite_index].dimensions.y;

	// If we're alive and the position is within our bounds, set ourselves
	// as exploding and return a valid hit
	if (state == ALIVE && check_position.x >= position.x - sprite_offset && check_position.x <= position.x - sprite_offset + sprite_width &&
		check_position.y >= position.y && check_position.y <= position.y + sprite_height)
	{
		int score = 0;
		switch (type)
		{
		case MOTHERSHIP: score = (Rml::Math::RandomInteger(6) + 1) * 50; break; // 50 -> 300
		case RANK3: score = 40; break;
		case RANK2: score = 20; break;
		case RANK1: score = 10; break;
		case UNKNOWN: break;
		}

		// Add the number of points
		game->AddScore(score);

		// Set our state to exploding and start the timer to our doom
		state = EXPLODING;
		death_time = t + EXPLOSION_TIME;

		return true;
	}

	return false;
}

int Invader::GetSpriteIndex() const
{
	// Calculate our sprite index based on animation and type
	int index = animation_frame;
	switch (type)
	{
	case RANK1: break; // animation_frame is the right index already
	case RANK2: index += 2; break;
	case RANK3: index += 4; break;
	case MOTHERSHIP: index = 6; break;
	case UNKNOWN: break;
	}

	// If we're in exploding state, use the exploding sprite
	if (state == EXPLODING)
		index = 7;

	return index;
}
