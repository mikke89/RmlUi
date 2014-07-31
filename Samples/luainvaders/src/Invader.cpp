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

#include "Invader.h"
#include <Rocket/Core/Math.h>
#include <Shell.h>
#include <ShellOpenGL.h>
#include "Defender.h"
#include "Game.h"
#include "GameDetails.h"
#include "Shield.h"
#include "Sprite.h"

const float BOMB_UPDATE_FREQ = 0.04f;
const float BOMB_RAY_SPEED = 10;
const float BOMB_MISSILE_SPEED = 7;
const float BOMB_PROBABILITY_EASY = 0.002f;
const float BOMB_PROBABILITY_HARD = 0.005f;
const float EXPLOSION_TIME = 0.25f;
const Rocket::Core::Colourb MOTHERSHIP_COLOUR = Rocket::Core::Colourb(255, 0, 0, 255);

Sprite invader_sprites[] =
{
	// Rank 1
	Sprite(Rocket::Core::Vector2f(48, 32), Rocket::Core::Vector2f(0.609375f, 0), Rocket::Core::Vector2f(0.796875f, 0.5f)),
	Sprite(Rocket::Core::Vector2f(48, 32), Rocket::Core::Vector2f(0.80078125f, 0), Rocket::Core::Vector2f(0.98828125f, 0.5f)),
	// Rank 2
	Sprite(Rocket::Core::Vector2f(44, 32), Rocket::Core::Vector2f(0.2578125f, 0), Rocket::Core::Vector2f(0.4296875f, 0.5f)),
	Sprite(Rocket::Core::Vector2f(44, 32), Rocket::Core::Vector2f(0.43359375f, 0), Rocket::Core::Vector2f(0.60546875f, 0.5f)),
	// Rank 3
	Sprite(Rocket::Core::Vector2f(32, 32), Rocket::Core::Vector2f(0, 0), Rocket::Core::Vector2f(0.125f, 0.5f)),
	Sprite(Rocket::Core::Vector2f(32, 32), Rocket::Core::Vector2f(0.12890625f, 0), Rocket::Core::Vector2f(0.25390625f, 0.5f)),
	// Mothership
	Sprite(Rocket::Core::Vector2f(64, 28), Rocket::Core::Vector2f(0.23828125f, 0.515625f), Rocket::Core::Vector2f(0.48828125f, 0.953125f)),
	// Explosion
	Sprite(Rocket::Core::Vector2f(52, 28), Rocket::Core::Vector2f(0.71484375f, 0.51562500f), Rocket::Core::Vector2f(0.91796875f, 0.95312500f))	
};

Sprite bomb_sprites[] =
{
	// Ray
	Sprite(Rocket::Core::Vector2f(12, 20), Rocket::Core::Vector2f(0.51171875f, 0.51562500f), Rocket::Core::Vector2f(0.55859375f, 0.82812500f)),
	Sprite(Rocket::Core::Vector2f(12, 20), Rocket::Core::Vector2f(0.56250000, 0.51562500), Rocket::Core::Vector2f(0.60937500, 0.82812500)),
	Sprite(Rocket::Core::Vector2f(12, 20), Rocket::Core::Vector2f(0.61328125, 0.51562500), Rocket::Core::Vector2f(0.66015625, 0.82812500)),
	Sprite(Rocket::Core::Vector2f(12, 20), Rocket::Core::Vector2f(0.66406250, 0.51562500), Rocket::Core::Vector2f(0.71093750, 0.82812500)),
	// Missile
	Sprite(Rocket::Core::Vector2f(12, 20), Rocket::Core::Vector2f(0.92578125, 0.51562500), Rocket::Core::Vector2f(0.97265625, 0.82812500))
};

Invader::Invader(Game* _game, InvaderType _type, int _index) : position(0,0)
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
	bomb_probability *= type;
}

Invader::~Invader()
{
}

void Invader::SetPosition(const Rocket::Core::Vector2f& _position)
{
	position = _position;
}

const Rocket::Core::Vector2f& Invader::GetPosition() const
{
	return position;
}

void Invader::Update()
{
	// Update the bombs
	if (Shell::GetElapsedTime() - bomb_frame_start > BOMB_UPDATE_FREQ)
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
				if (game->GetDefender()->CheckHit(bomb_position))
					bomb = NONE;
			}
		}
		else if (state == ALIVE &&
				 Rocket::Core::Math::RandomReal(1.0f) < bomb_probability &&
				 game->CanDropBomb(invader_index))
		{
			bomb = Rocket::Core::Math::RandomInteger(2) == 0 ? RAY : MISSILE;
			bomb_position = position;
			bomb_position.x += invader_sprites[GetSpriteIndex()].dimensions.x / 2;

			if (bomb == RAY)
				bomb_animation_frame = 0;
			else
				bomb_animation_frame = 4;
		}	

		bomb_frame_start = Shell::GetElapsedTime();
	}

	if (state == EXPLODING && Shell::GetElapsedTime() > death_time)
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

	default:
		break;
	}
}

void Invader::Render()
{
	if (type == MOTHERSHIP)
	{
		glColor4ubv(MOTHERSHIP_COLOUR);
	}
	int sprite_index = GetSpriteIndex();
	int sprite_offset = Rocket::Core::Math::RealToInteger((invader_sprites[sprite_index].dimensions.x - 48) / 2);

	if (state != DEAD)
		invader_sprites[sprite_index].Render(Rocket::Core::Vector2f(position.x - sprite_offset, position.y));
	
	if (bomb != NONE)
	{
		bomb_sprites[bomb_animation_frame].Render(bomb_position);
	}

	if (type == MOTHERSHIP)
	{
		glColor4ub(255, 255, 255, 255);
	}
}

Invader::InvaderState Invader::GetState()
{
	return state;
}

bool Invader::CheckHit(const Rocket::Core::Vector2f& check_position)
{
	// Get the sprite index we're currently using for collision detection
	int sprite_index = GetSpriteIndex();
	int sprite_offset = Rocket::Core::Math::RealToInteger((invader_sprites[sprite_index].dimensions.x - 48) / 2);
	float sprite_width = invader_sprites[sprite_index].dimensions.x;
	float sprite_height = invader_sprites[sprite_index].dimensions.y;

	// If we're alive and the position is within our bounds, set ourselves
	// as exploding and return a valid hit
	if (state == ALIVE
		&& check_position.x >= position.x - sprite_offset
		&& check_position.x <= position.x - sprite_offset + sprite_width
		&& check_position.y >= position.y
		&& check_position.y <= position.y + sprite_height)
	{
		int score = 0;
		switch (type)
		{
			ROCKET_UNUSED_SWITCH_ENUM(UNKNOWN);
			case MOTHERSHIP: score = (Rocket::Core::Math::RandomInteger(6) + 1) * 50; break;	// 50 -> 300
			case RANK3: score = 40; break;
			case RANK2: score = 20; break;
			case RANK1: score = 10; break;
		}

		// Add the number of points
		game->AddScore(score);

		// Set our state to exploding and start the timer to our doom
		state = EXPLODING;
		death_time = Shell::GetElapsedTime() + EXPLOSION_TIME;	

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
		ROCKET_UNUSED_SWITCH_ENUM(UNKNOWN);
		case RANK1:                 break;      // animation_frame is the right index already
		case RANK2:	index += 2; break;
		case RANK3:	index += 4; break;
		case MOTHERSHIP: index = 6; break;
	}

	// If we're in exploding state, use the exploding sprite
	if (state == EXPLODING)
		index = 7;

	return index;
}
