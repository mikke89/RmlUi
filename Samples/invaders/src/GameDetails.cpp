#include "GameDetails.h"

static GameDetails::Difficulty difficulty = GameDetails::EASY;
static Rml::Colourb colour = Rml::Colourb(255, 0, 0);
static int score = -1;
static int wave = 0;
static bool paused = false;

static GameDetails::GraphicsQuality graphics_quality = GameDetails::OK;
static bool reverb = true;
static bool spatialisation = false;

GameDetails::GameDetails() {}

GameDetails::~GameDetails() {}

void GameDetails::SetDifficulty(Difficulty _difficulty)
{
	difficulty = _difficulty;
}

GameDetails::Difficulty GameDetails::GetDifficulty()
{
	return difficulty;
}

void GameDetails::SetDefenderColour(const Rml::Colourb& _colour)
{
	colour = _colour;
}

const Rml::Colourb& GameDetails::GetDefenderColour()
{
	return colour;
}

void GameDetails::SetScore(int _score)
{
	score = _score;
}

void GameDetails::ResetScore()
{
	score = -1;
}

int GameDetails::GetScore()
{
	return score;
}

void GameDetails::SetWave(int _wave)
{
	wave = _wave;
}

int GameDetails::GetWave()
{
	return wave;
}

void GameDetails::SetPaused(bool _paused)
{
	paused = _paused;
}

bool GameDetails::GetPaused()
{
	return paused;
}

void GameDetails::SetGraphicsQuality(GraphicsQuality quality)
{
	graphics_quality = quality;
}

GameDetails::GraphicsQuality GameDetails::GetGraphicsQuality()
{
	return graphics_quality;
}

void GameDetails::SetReverb(bool enabled)
{
	reverb = enabled;
}

bool GameDetails::GetReverb()
{
	return reverb;
}

void GameDetails::Set3DSpatialisation(bool enabled)
{
	spatialisation = enabled;
}

bool GameDetails::Get3DSpatialisation()
{
	return spatialisation;
}
