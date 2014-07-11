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

#include "Mothership.h"
#include <Rocket/Core/Math.h>
#include "Shell.h"
#include "Game.h"
#include "Sprite.h"

const int SPRITE_WIDTH = 64;

const float APPEARANCE_PROBABILITY = 0.001f;
const float UPDATE_FREQ = 0.025f;
const float MOVEMENT_SPEED = 5;

Mothership::Mothership(Game* game, int index) : Invader(game, Invader::MOTHERSHIP, index)
{
	// Start off dead, and set up our position
	state = DEAD;
	update_frame_start = 0;
	position = Rocket::Core::Vector2f(-SPRITE_WIDTH, 64.0f);
}

Mothership::~Mothership()
{
}

void Mothership::Update()
{
	// Generic Invader update
	Invader::Update();

	if (Shell::GetElapsedTime() - update_frame_start < UPDATE_FREQ)
		return;

	// We're alive, keep moving!
	if (state == ALIVE)
	{
		position.x += (direction * MOVEMENT_SPEED);

		if ((direction < 0.0f && position.x < -SPRITE_WIDTH)
			|| (direction > 0.0f && position.x > game->GetWindowDimensions().x))
			state = DEAD;

		update_frame_start = Shell::GetElapsedTime();
	}
	// Determine if we should come out of hiding
	else if (Rocket::Core::Math::RandomReal(1.0f) < APPEARANCE_PROBABILITY)
	{
		direction = Rocket::Core::Math::RandomReal(1.0f) < 0.5 ? -1.0f : 1.0f;

		if (direction < 0)
			position.x = game->GetWindowDimensions().x + SPRITE_WIDTH;
		else
			position.x = -SPRITE_WIDTH;

		state = ALIVE;
	}
}
