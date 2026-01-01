#include "Shield.h"
#include "Game.h"
#include "GameDetails.h"
#include "Sprite.h"
#include <RmlUi/Core/Math.h>

const int MAX_HEALTH = 4;

Shield::Shield(Game* _game, ShieldType _type) : position(0, 0)
{
	game = _game;
	type = _type;
	health = MAX_HEALTH;

	InitialiseCells();
}

Shield::~Shield() {}

void Shield::InitialiseCells()
{
	if (type == REGULAR || type == TOP_LEFT || type == TOP_RIGHT)
	{
		for (int i = 0; i < NUM_SHIELD_CELLS; i++)
		{
			for (int j = 0; j < NUM_SHIELD_CELLS; j++)
			{
				shield_cells[i][j] = ON;
			}
		}

		// Take the bites out of the cells if they're a corner cell:
		if (type == TOP_LEFT)
		{
			for (int x = 0; x < NUM_SHIELD_CELLS - 1; x++)
			{
				for (int y = 0; y < (NUM_SHIELD_CELLS - 1) - x; y++)
					shield_cells[x][y] = OFF;
			}
		}

		else if (type == TOP_RIGHT)
		{
			for (int x = 0; x < NUM_SHIELD_CELLS - 1; x++)
			{
				for (int y = 0; y < (NUM_SHIELD_CELLS - 1) - x; y++)
					shield_cells[(NUM_SHIELD_CELLS - 1) - x][y] = OFF;
			}
		}
	}
	else
	{
		for (int i = 0; i < NUM_SHIELD_CELLS; i++)
		{
			for (int j = 0; j < NUM_SHIELD_CELLS; j++)
			{
				shield_cells[i][j] = OFF;
			}
		}

		if (type == BOTTOM_LEFT)
		{
			for (int x = 0; x < NUM_SHIELD_CELLS - 1; x++)
			{
				for (int y = 0; y < (NUM_SHIELD_CELLS - 1) - x; y++)
					shield_cells[(NUM_SHIELD_CELLS - 1) - x][y] = ON;
			}
		}

		else if (type == BOTTOM_RIGHT)
		{
			for (int x = 0; x < NUM_SHIELD_CELLS - 1; x++)
			{
				for (int y = 0; y < (NUM_SHIELD_CELLS - 1) - x; y++)
					shield_cells[x][y] = ON;
			}
		}
	}
}

void Shield::SetPosition(const Rml::Vector2f& _position)
{
	position = _position;
}

const Rml::Vector2f& Shield::GetPosition() const
{
	return position;
}

void Shield::Render(Rml::RenderManager& render_manager, float dp_ratio)
{
	if (health > 0)
	{
		const Rml::Vector2f scaled_position = (dp_ratio * position).Round();
		const int scaled_pixel = Rml::Math::RoundUpToInteger(PIXEL_SIZE * dp_ratio);

		Rml::ColourbPremultiplied color = GameDetails::GetDefenderColour().ToPremultiplied();
		ColoredPointList points;
		points.reserve(NUM_SHIELD_CELLS * NUM_SHIELD_CELLS);

		for (int i = 0; i < NUM_SHIELD_CELLS; i++)
		{
			for (int j = 0; j < NUM_SHIELD_CELLS; j++)
			{
				if (shield_cells[i][j] == ON)
				{
					Rml::Vector2f cell_position = scaled_position + Rml::Vector2f(float(scaled_pixel * i), float(scaled_pixel * j));
					points.push_back(ColoredPoint{color, cell_position});
				}
			}
		}

		DrawPoints(render_manager, (float)scaled_pixel, points);
	}
}

bool Shield::CheckHit(const Rml::Vector2f& check_position)
{
	float sprite_size = PIXEL_SIZE * NUM_SHIELD_CELLS;

	// If we're alive and the position is within our bounds, set ourselves
	// as exploding and return a valid hit
	if (health > 0 && check_position.x >= position.x && check_position.x <= position.x + sprite_size && check_position.y >= position.y &&
		check_position.y <= position.y + sprite_size)
	{
		// Take damage.
		SustainDamage();

		return true;
	}

	return false;
}

void Shield::SustainDamage()
{
	health--;

	if (health > 0)
	{
		int num_shields_to_lose = (NUM_SHIELD_CELLS * NUM_SHIELD_CELLS) / MAX_HEALTH;
		while (num_shields_to_lose > 0)
		{
			int x = Rml::Math::RandomInteger(NUM_SHIELD_CELLS);
			int y = Rml::Math::RandomInteger(NUM_SHIELD_CELLS);
			if (shield_cells[x][y] != DESTROYED)
			{
				shield_cells[x][y] = DESTROYED;
				num_shields_to_lose--;
			}
		}
	}
}
