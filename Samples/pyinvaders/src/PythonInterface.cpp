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

#include "PythonInterface.h"
#include <Shell.h>
#include "GameDetails.h"
#include "ElementGame.h"
#include "HighScores.h"

void SubmitHighScore()
{
	int score = GameDetails::GetScore();
	if (score > 0)
	{
		// Submit the score the player just got to the high scores chart.
		HighScores::SubmitScore(GameDetails::GetDefenderColour(), GameDetails::GetWave(), GameDetails::GetScore());
		// Reset the score so the chart won't get confused next time we enter.
		GameDetails::ResetScore();
	}
}

BOOST_PYTHON_MODULE(game)
{
	python::def("Shutdown", &Shell::RequestExit);
	python::def("SetPaused", &GameDetails::SetPaused);
	python::def("SetDifficulty", &GameDetails::SetDifficulty);
	python::def("SetDefenderColour", &GameDetails::SetDefenderColour);
	python::def("SubmitHighScore", &SubmitHighScore);
	python::def("SetHighScoreName", &HighScores::SubmitName);

	python::enum_<GameDetails::Difficulty>("difficulty")
		.value("HARD", GameDetails::HARD)
		.value("EASY", GameDetails::EASY)
	;

	ElementGame::InitialisePythonInterface();
}

PythonInterface::PythonInterface()
{
}

PythonInterface::~PythonInterface()
{
}

bool PythonInterface::Initialise(const char* path)
{
	// Initialise Python.
	Py_Initialize();

	// Setup the Python search path.
	const char* python_path = Py_GetPath();
	char buffer[1024];
	snprintf(buffer, 1024, "%s%s%s", path, PATH_SEPARATOR, python_path);
	buffer[1023] = '\0';
	PySys_SetPath(buffer);

	// Import Rocket.
	if (!Import("rocket"))
		return false;

	// Define our game specific interface.
	initgame();

	return true;
}

void PythonInterface::Shutdown()
{
	Py_Finalize();
}

bool PythonInterface::Import(const Rocket::Core::String& name)
{
	PyObject* module = PyImport_ImportModule(name.CString());
	if (!module)
	{
		PrintError(true);
		return false;
	}

	Py_DECREF(module);
	return true;
}

// Print the pending python error to stderr, optionally clearing it
void PythonInterface::PrintError(bool clear_error)
{
	// Print the error and restore it to the caller
	PyObject *type, *value, *traceback;
	PyErr_Fetch(&type, &value, &traceback);
	Py_XINCREF(type);
	Py_XINCREF(value);
	Py_XINCREF(traceback);
	PyErr_Restore(type, value, traceback);	
	PyErr_Print();
	if (!clear_error)
		PyErr_Restore(type, value, traceback);
}
