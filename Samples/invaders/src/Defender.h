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

#ifndef ROCKETINVADERSDEFENDER_H
#define ROCKETINVADERSDEFENDER_H

#include <Rocket/Core/Types.h>

class Game;
class Sprite;

/**
	Represents the Earth defender. Stores position and performs the update of the bullet position and collision detection.
	@author Lloyd Weehuizen
 */

class Defender
{
public:
	Defender(Game* game);
	~Defender();

	/// Update the defender state.
	void Update();
	/// Render the defender.
	void Render();

	/// Move the defender left.
	void StartMove(float direction);	
	/// Stop the movement.
	void StopMove(float direction);
	/// Fire a bullet (if one isn't already in flight).
	void Fire();	

	/// Check if an object at the given position would hit the defender.
	bool CheckHit(const Rocket::Core::Vector2f& position);

private:
	Game* game;
	Rocket::Core::Vector2f position;	
	
	float move_direction;
	
	bool bullet_in_flight;
	Rocket::Core::Vector2f bullet_position;

	float defender_frame_start;
	float respawn_start;

	bool render;

	enum State { ALIVE, RESPAWN };
	State state;
};


#endif
