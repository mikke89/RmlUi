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

#ifndef RMLUI_TESTS_VISUALTESTS_WINDOW_H
#define RMLUI_TESTS_VISUALTESTS_WINDOW_H

#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Types.h>
#include "XmlNodeHandlers.h"

namespace Rml { class Context; class ElementDocument; }

struct TestSuite {
	Rml::String directory;
	Rml::StringList files;
};
using TestSuiteList = Rml::Vector<TestSuite>;


class Window : public Rml::EventListener, Rml::NonCopyMoveable
{
public:
	Window(Rml::Context* context, TestSuiteList test_suites);
	~Window();
	
private:
	const TestSuite& GetCurrentTestSuite() const;
	Rml::String GetCurrentPath() const;
	Rml::String GetReferencePath() const;

	void OpenSource(const Rml::String& file_path);

	void OpenSource();

	void SwitchSource();

	void CloseSource();

	void ReloadDocument();

	void ProcessEvent(Rml::Event& event) override;

	Rml::Context* context;

	int goto_id = -1;

	int current_id = 0;
	Rml::ElementDocument* document = nullptr;
	Rml::ElementDocument* document_description = nullptr;
	Rml::ElementDocument* document_source = nullptr;
	Rml::ElementDocument* document_match = nullptr;
	Rml::String reference_file;
	bool viewing_reference_source = false;

	const TestSuiteList test_suites;
	int current_test_suite = 0;

	MetaList meta_list;
	LinkList link_list;
};


#endif
