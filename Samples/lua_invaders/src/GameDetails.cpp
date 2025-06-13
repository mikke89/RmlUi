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
