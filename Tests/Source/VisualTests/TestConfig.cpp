/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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

#include "TestConfig.h"
#include <Shell.h>
#include <RmlUi/Core/Types.h>
#include <RmlUi/Core/StringUtilities.h>


Rml::String GetCompareInputDirectory()
{
#ifdef RMLUI_VISUAL_TESTS_COMPARE_DIRECTORY
	const Rml::String input_directory = Rml::String(RMLUI_VISUAL_TESTS_COMPARE_DIRECTORY);
#else
	const Rml::String input_directory = Shell::FindSamplesRoot() + "../Tests/Output";
#endif
	return input_directory;
}

Rml::String GetCaptureOutputDirectory()
{
#ifdef RMLUI_VISUAL_TESTS_CAPTURE_DIRECTORY
	const Rml::String output_directory = Rml::String(RMLUI_VISUAL_TESTS_CAPTURE_DIRECTORY);
#else
	const Rml::String output_directory = Shell::FindSamplesRoot() + "../Tests/Output";
#endif
	return output_directory;
}

Rml::StringList GetTestInputDirectories()
{
	const Rml::String samples_root = Shell::FindSamplesRoot();

	Rml::StringList directories = { samples_root + "../Tests/Data/VisualTests" };

#ifdef RMLUI_VISUAL_TESTS_RML_DIRECTORIES
	Rml::StringUtilities::ExpandString(directories, RMLUI_VISUAL_TESTS_RML_DIRECTORIES, ';');
#endif

	return directories;
}
