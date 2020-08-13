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

#ifndef RMLUI_TESTS_VISUALTESTS_TESTVIEWER_H
#define RMLUI_TESTS_VISUALTESTS_TESTVIEWER_H

#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Types.h>

namespace Rml { class Context; class ElementDocument; }

enum class SourceType { None, Test, Reference };


class TestViewer
{
public:
	TestViewer(Rml::Context* context);
	~TestViewer();
	
	void ShowSource(SourceType type);
	void ShowHelp(bool show);
	bool IsHelpVisible() const;

	bool LoadTest(const Rml::String& directory, const Rml::String& filename, int test_index, int number_of_tests, int filtered_test_index, int filtered_number_of_tests, int suite_index, int number_of_suites);

	void SetGoToText(const Rml::String& rml);

private:
	Rml::Context* context;

	Rml::ElementDocument* document_test = nullptr;
	Rml::ElementDocument* document_description = nullptr;
	Rml::ElementDocument* document_source = nullptr;
	Rml::ElementDocument* document_reference = nullptr;
	Rml::ElementDocument* document_help = nullptr;

	Rml::String source_test;
	Rml::String source_reference;

	Rml::String reference_filename;
};


#endif
