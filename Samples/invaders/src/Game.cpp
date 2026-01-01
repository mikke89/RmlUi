#include "Game.h"
#include "Defender.h"
#include "EventManager.h"
#include "GameDetails.h"
#include "HighScores.h"
#include "Invader.h"
#include "Mothership.h"
#include "Shield.h"
#include <RmlUi/Core.h>
#include <Shell.h>

const int WINDOW_WIDTH = 1024;
const int WINDOW_HEIGHT = 768;

const int NUM_LIVES = 3;

const int NUM_INVADER_ROWS = 5;
const int NUM_INVADERS_PER_ROW = 11;
const int NUM_INVADERS = (NUM_INVADER_ROWS * NUM_INVADERS_PER_ROW);
const int INVADER_SPACING_X = 64;
const int INVADER_SPACING_Y = 48;
const int INVADER_START_Y = 96;

const float INVADER_MOVEMENT = 10;
const float INVADER_UPDATE_MODIFIER = 0.7f;

const int MOTHERSHIP = NUM_INVADERS;

const int NUM_SHIELD_ARRAYS = 4;
const int NUM_SHIELDS_PER_ARRAY = 10;
const int NUM_SHIELDS = (NUM_SHIELD_ARRAYS * NUM_SHIELDS_PER_ARRAY);
const int SHIELD_SIZE = (NUM_SHIELD_CELLS * PIXEL_SIZE);
const int SHIELD_SPACING_X = 192;
const int SHIELD_START_X = 176;
const int SHIELD_START_Y = 600;

// The game's element context (declared in main.cpp).
extern Rml::Context* context;

Game::Game()
{
	initialized = false;
	invader_frame_start = 0;
	defender_lives = 3;
	current_invader_direction = 1.0f;
	invaders = new Invader*[NUM_INVADERS + 1];
	for (int i = 0; i < NUM_INVADERS + 1; i++)
		invaders[i] = nullptr;

	shields = new Shield*[NUM_SHIELDS];
	for (int i = 0; i < NUM_SHIELDS; i++)
		shields[i] = nullptr;

	defender = new Defender(this);
}

Game::~Game()
{
	delete defender;

	for (int i = 0; i < NUM_INVADERS + 1; i++)
		delete invaders[i];

	delete[] invaders;

	for (int i = 0; i < NUM_SHIELDS; i++)
		delete shields[i];

	delete[] shields;
}

void Game::Initialise()
{
	// Initialise the scores and wave information
	SetScore(0);
	SetWave(1);
	SetHighScore(HighScores::GetHighScore());
	SetLives(NUM_LIVES);

	// Initialise the shields.
	InitialiseShields();

	// Create a new wave
	InitialiseWave();

	initialized = true;
}

void Game::Update(double t)
{
	if (!GameDetails::GetPaused() && initialized)
	{
		if (defender_lives <= 0)
			return;

		// Determine if we should advance the invaders
		if (t - invader_frame_start >= invader_move_freq)
		{
			MoveInvaders();

			invader_frame_start = t;
		}

		// Update all invaders
		for (int i = 0; i < NUM_INVADERS + 1; i++)
			invaders[i]->Update(t);

		defender->Update(t);
	}
}

void Game::Render(Rml::RenderManager& render_manager, float dp_ratio)
{
	if (defender_lives <= 0)
		return;

	if (!texture)
		texture = render_manager.LoadTexture("invaders/data/invaders.tga");

	// Render all available shields
	for (int i = 0; i < NUM_SHIELDS; i++)
		shields[i]->Render(render_manager, dp_ratio);

	// Render all available invaders
	for (int i = 0; i < NUM_INVADERS + 1; i++)
		invaders[i]->Render(render_manager, dp_ratio, texture);

	defender->Render(render_manager, dp_ratio, texture);
}

Defender* Game::GetDefender()
{
	return defender;
}

Invader* Game::GetInvader(int index)
{
	return invaders[index];
}

int Game::GetNumInvaders()
{
	return NUM_INVADERS + 1;
}

bool Game::CanDropBomb(int index)
{
	// Determines if the given invader can drop a bomb by checking if theres any other invaders under it
	int y = (index / NUM_INVADERS_PER_ROW);
	int x = index - (y * NUM_INVADERS_PER_ROW);

	// The mothership can never drop bombs
	if (index == MOTHERSHIP)
		return false;

	// Check each row under the given invader to see if there is space
	for (int i = y + 1; i < NUM_INVADER_ROWS; i++)
	{
		if (invaders[(i * NUM_INVADERS_PER_ROW) + x]->GetState() == Invader::ALIVE)
			return false;
	}

	return true;
}

Shield* Game::GetShield(int index)
{
	return shields[index];
}

int Game::GetNumShields()
{
	return NUM_SHIELDS;
}

void Game::AddScore(int score)
{
	SetScore(GameDetails::GetScore() + score);
}

void Game::SetScore(int score)
{
	GameDetails::SetScore(score);

	Rml::Element* score_element = context->GetDocument("game_window")->GetElementById("score");
	if (score_element != nullptr)
		score_element->SetInnerRML(Rml::CreateString("%d", score).c_str());

	// Update the high score if we've beaten it.
	if (score > HighScores::GetHighScore())
		SetHighScore(score);
}

void Game::SetHighScore(int score)
{
	Rml::Element* high_score_element = context->GetDocument("game_window")->GetElementById("hiscore");
	if (high_score_element != nullptr)
		high_score_element->SetInnerRML(Rml::CreateString("%d", score).c_str());
}

void Game::SetLives(int lives)
{
	defender_lives = lives;

	Rml::Element* score_element = context->GetDocument("game_window")->GetElementById("lives");
	if (score_element != nullptr)
		score_element->SetInnerRML(Rml::CreateString("%d", defender_lives).c_str());
}

void Game::SetWave(int wave)
{
	GameDetails::SetWave(wave);

	Rml::Element* waves_element = context->GetDocument("game_window")->GetElementById("waves");
	if (waves_element != nullptr)
		waves_element->SetInnerRML(Rml::CreateString("%d", wave).c_str());
}

void Game::RemoveLife()
{
	if (defender_lives > 0)
	{
		SetLives(defender_lives - 1);

		if (defender_lives == 0)
		{
			OnGameOver();
		}
	}
}

const Rml::Vector2f Game::GetWindowDimensions()
{
	return Rml::Vector2f((float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);
}

void Game::MoveInvaders()
{
	Rml::Vector2f new_positions[NUM_INVADERS];

	// We loop through all invaders, calculating their new positions, if any of them go over the screen bounds,
	// then we switch direction, move the invaders down and start at 0 again
	for (int i = 0; i < NUM_INVADERS; i++)
	{
		if (invaders[i]->GetState() == Invader::DEAD)
			continue;

		new_positions[i] = invaders[i]->GetPosition();
		new_positions[i].x += INVADER_MOVEMENT * current_invader_direction;
		if (new_positions[i].x < 0 || new_positions[i].x + INVADER_SPACING_X > WINDOW_WIDTH)
		{
			// Switch direction and start back at 0 (-1 as the for loop increments)
			current_invader_direction *= -1.0f;
			i = -1;

			// Move all invaders down
			for (int j = 0; j < NUM_INVADERS; j++)
			{
				if (invaders[j]->GetState() == Invader::DEAD)
					continue;

				Rml::Vector2f position = invaders[j]->GetPosition();
				position.y += INVADER_SPACING_Y;
				invaders[j]->SetPosition(position);
			}

			// Increase speed of invaders
			invader_move_freq *= INVADER_UPDATE_MODIFIER;
		}
	}

	// Assign invaders their new position and advance their animation frame
	bool invaders_alive = false;

	for (int i = 0; i < NUM_INVADERS; i++)
	{
		if (invaders[i]->GetState() == Invader::DEAD)
			continue;

		invaders[i]->SetPosition(new_positions[i]);
		invaders[i]->UpdateAnimation();

		// If an invader hits the bottom, instant death
		if (new_positions[i].y >= GetWindowDimensions().y - INVADER_SPACING_Y)
		{
			OnGameOver();
			return;
		}

		invaders_alive = true;
	}

	if (!invaders_alive)
	{
		SetWave(GameDetails::GetWave() + 1);
		InitialiseWave();
	}
}

void Game::OnGameOver()
{
	// Set lives to zero and continue to the high scores
	defender_lives = 0;
	EventManager::LoadWindow("high_score");
}

void Game::InitialiseShields()
{
	Rml::Vector2f shield_array_start_position((float)SHIELD_START_X, (float)SHIELD_START_Y);

	for (int x = 0; x < NUM_SHIELD_ARRAYS; x++)
	{
		// Top row (row of 4)
		for (int i = 0; i < 4; i++)
		{
			Shield::ShieldType type = Shield::REGULAR;
			if (i == 0)
				type = Shield::TOP_LEFT;
			else if (i == 3)
			{
				type = Shield::TOP_RIGHT;
			}

			shields[(x * NUM_SHIELDS_PER_ARRAY) + i] = new Shield(this, type);
			shields[(x * NUM_SHIELDS_PER_ARRAY) + i]->SetPosition(shield_array_start_position + Rml::Vector2f((float)SHIELD_SIZE * i, 0));
		}

		// Middle row (row of 4)
		for (int i = 0; i < 4; i++)
		{
			Shield::ShieldType type = Shield::REGULAR;
			if (i == 1)
				type = Shield::BOTTOM_RIGHT;
			else if (i == 2)
			{
				type = Shield::BOTTOM_LEFT;
			}

			shields[(x * NUM_SHIELDS_PER_ARRAY) + 4 + i] = new Shield(this, type);
			shields[(x * NUM_SHIELDS_PER_ARRAY) + 4 + i]->SetPosition(
				shield_array_start_position + Rml::Vector2f((float)SHIELD_SIZE * i, (float)SHIELD_SIZE));
		}

		// Bottom row (2, one on each end)
		shields[(x * NUM_SHIELDS_PER_ARRAY) + 8] = new Shield(this, Shield::REGULAR);
		shields[(x * NUM_SHIELDS_PER_ARRAY) + 8]->SetPosition(shield_array_start_position + Rml::Vector2f(0, (float)SHIELD_SIZE * 2));

		shields[(x * NUM_SHIELDS_PER_ARRAY) + 9] = new Shield(this, Shield::REGULAR);
		shields[(x * NUM_SHIELDS_PER_ARRAY) + 9]->SetPosition(
			shield_array_start_position + Rml::Vector2f((float)SHIELD_SIZE * 3, (float)SHIELD_SIZE * 2));

		shield_array_start_position.x += SHIELD_SPACING_X;
	}
}

void Game::InitialiseWave()
{
	// Set up the rows
	for (int y = 0; y < NUM_INVADER_ROWS; y++)
	{
		// Determine invader type
		Invader::InvaderType type = Invader::UNKNOWN;
		switch (y)
		{
		case 0: type = Invader::RANK3; break;
		case 1:
		case 2: type = Invader::RANK2; break;
		default: type = Invader::RANK1; break;
		}

		// Determine position of top left invader
		Rml::Vector2f invader_position((float)(WINDOW_WIDTH - (NUM_INVADERS_PER_ROW * INVADER_SPACING_X)) / 2,
			(float)(INVADER_START_Y + (y * INVADER_SPACING_Y)));

		for (int x = 0; x < NUM_INVADERS_PER_ROW; x++)
		{
			// Determine invader type based on row position
			int index = (y * NUM_INVADERS_PER_ROW) + x;

			delete invaders[index];
			invaders[index] = new Invader(this, type, (y * NUM_INVADERS_PER_ROW) + x);
			invaders[index]->SetPosition(invader_position);

			// Increase invader position
			invader_position.x += INVADER_SPACING_X;
		}
	}

	// Reset mother ship
	delete invaders[MOTHERSHIP];
	invaders[MOTHERSHIP] = new Mothership(this, MOTHERSHIP);

	// Update the move frequency
	invader_move_freq = ((((float)GameDetails::GetWave()) - 100.0f) / 140.0f);
	invader_move_freq *= invader_move_freq;
}
