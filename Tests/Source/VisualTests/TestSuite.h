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

#ifndef RMLUI_TESTS_VISUALTESTS_TESTSUITE_H
#define RMLUI_TESTS_VISUALTESTS_TESTSUITE_H

#include <RmlUi/Core/Types.h>


class TestSuite {
public:
	TestSuite(Rml::String directory, Rml::StringList files) : directory(std::move(directory)), files(std::move(files))
	{
		RMLUI_ASSERTMSG(!this->files.empty(), "At least one file in the test suite is required.");
	}

	const Rml::String& GetDirectory() const
	{
		return directory;
	}
	const Rml::String& GetFilename() const
	{
		return files[index];
	}
	Rml::String GetPath() const
	{
		return directory + '/' + files[index];
	}

	bool SetIndex(int new_index)
	{
		if (new_index < 0 || new_index >= (int)files.size())
			return false;
		index = new_index;
		return true;
	}

	int GetIndex() const
	{
		return index;
	}

	int GetNumTests() const
	{
		return (int)files.size();
	}

private:
	Rml::String directory;
	Rml::StringList files;

	int index = 0;
};

using TestSuiteList = Rml::Vector<TestSuite>;


#endif
