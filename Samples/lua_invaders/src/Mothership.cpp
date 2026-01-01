#include "Mothership.h"
#include "Game.h"
#include "Shell.h"
#include "Sprite.h"
#include <RmlUi/Core/Math.h>

const int SPRITE_WIDTH = 64;

const float APPEARANCE_PROBABILITY = 0.001f;
const double UPDATE_FREQ = 0.025;
const float MOVEMENT_SPEED = 5;

Mothership::Mothership(Game* game, int index) : Invader(game, Invader::MOTHERSHIP, index)
{
	// Start off dead, and set up our position
	state = DEAD;
	update_frame_start = 0;
	direction = 0;
	position = Rml::Vector2f(-SPRITE_WIDTH, 64.0f);
}

Mothership::~Mothership() {}

void Mothership::Update(double t)
{
	// Generic Invader update
	Invader::Update(t);

	if (t - update_frame_start < UPDATE_FREQ)
		return;

	// We're alive, keep moving!
	if (state == ALIVE)
	{
		position.x += (direction * MOVEMENT_SPEED);

		if ((direction < 0.0f && position.x < -SPRITE_WIDTH) || (direction > 0.0f && position.x > game->GetWindowDimensions().x))
			state = DEAD;

		update_frame_start = t;
	}
	// Determine if we should come out of hiding
	else if (Rml::Math::RandomReal(1.0f) < APPEARANCE_PROBABILITY)
	{
		direction = Rml::Math::RandomReal(1.0f) < 0.5 ? -1.0f : 1.0f;

		if (direction < 0)
			position.x = game->GetWindowDimensions().x + SPRITE_WIDTH;
		else
			position.x = -SPRITE_WIDTH;

		state = ALIVE;
	}
}
