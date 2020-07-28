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

#include "Window.h"
#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Types.h>
#include <Shell.h>

#include <doctest.h>

using namespace Rml;


Window::Window(Rml::Context* context, TestSuiteList test_suites) : context(context), test_suites(std::move(test_suites))
{
	InitializeXmlNodeHandlers(&meta_list, &link_list);

	const String local_data_path_prefix = "/../Tests/Data/";

	document_description = context->LoadDocument(local_data_path_prefix + "description.rml");
	REQUIRE(document_description);
	document_description->Show();

	document_source = context->LoadDocument(local_data_path_prefix + "view_source.rml");
	REQUIRE(document_source);

	ReloadDocument();

	context->GetRootElement()->AddEventListener(Rml::EventId::Keydown, this);
	context->GetRootElement()->AddEventListener(Rml::EventId::Textinput, this);
}

Window::~Window()
{
	context->GetRootElement()->RemoveEventListener(Rml::EventId::Keydown, this);
	context->GetRootElement()->RemoveEventListener(Rml::EventId::Textinput, this);

	for (ElementDocument* doc : { document, document_description, document_source, document_match })
	{
		if (doc)
			doc->Close();
	}
}

const TestSuite& Window::GetCurrentTestSuite() const {
	REQUIRE(current_test_suite >= 0);
	REQUIRE(current_test_suite < (int)test_suites.size());
	return test_suites[current_test_suite];
}

String Window::GetCurrentPath() const {
	const TestSuite& test_suite = GetCurrentTestSuite();
	REQUIRE(current_id >= 0);
	REQUIRE(current_id < (int)test_suite.files.size());

	return test_suite.directory + '/' + test_suite.files[current_id];
}

String Window::GetReferencePath() const {
	if (reference_file.empty())
		return String();
	const TestSuite& test_suite = GetCurrentTestSuite();
	return test_suite.directory + '/' + reference_file;
}

void Window::OpenSource(const String& file_path)
{
	FileInterface* file = Rml::GetFileInterface();
	FileHandle handle = file->Open(file_path);
	if (!handle)
		return;

	const size_t length = file->Length(handle);
	UniquePtr<char[]> buf = UniquePtr<char[]>(new char[length + 1]);

	const size_t read_length = file->Read(buf.get(), length, handle);
	file->Close(handle);
	REQUIRE(read_length > 0);
	REQUIRE(read_length <= length);
	buf[read_length] = '\0';
	const String rml_source = StringUtilities::EncodeRml(String(buf.get()));

	Element* element = document_source->GetElementById("code");
	REQUIRE(element);
	element->SetInnerRML(rml_source);

	document_source->Show();
}

void Window::OpenSource()
{
	const String file_path = (viewing_reference_source ? GetReferencePath() : GetCurrentPath());
	OpenSource(file_path);
}

void Window::SwitchSource()
{
	if (document_source->IsVisible())
	{
		viewing_reference_source = !viewing_reference_source;
		OpenSource();
	}
}

void Window::CloseSource()
{
	document_source->Hide();
	viewing_reference_source = false;
}

void Window::ReloadDocument()
{
	const String current_path = GetCurrentPath();
	const TestSuite& test_suite = GetCurrentTestSuite();
	const String& current_filename = test_suite.files[current_id];

	if (document)
	{
		document->Close();
		document = nullptr;
	}
	if (document_match)
	{
		document_match->Close();
		document_match = nullptr;
	}

	meta_list.clear();
	link_list.clear();

	document = context->LoadDocument(current_path);
	REQUIRE(document);
	document->Show();

	reference_file.clear();
	for (const LinkItem& item : link_list)
	{
		if (item.rel == "match")
		{
			reference_file = item.href;
			break;
		}
	}

	if (!reference_file.empty())
	{
		const String reference_path = GetReferencePath();

		// See if we can open the file first to avoid logging warnings.
		FileInterface* file = Rml::GetFileInterface();
		FileHandle handle = file->Open(reference_path);
		
		if (handle)
		{
			file->Close(handle);

			document_match = context->LoadDocument(reference_path);
			if (document_match)
			{
				document_match->SetProperty(PropertyId::Left, Property(510.f, Property::PX));
				document_match->Show();
			}
		}
	}

	String rml_description = Rml::CreateString(512, "<h1>%s</h1><p>Test %d of %d.<br/>%s", document->GetTitle().c_str(), current_id + 1, (int)test_suite.files.size(), current_filename.c_str());
	if (!reference_file.empty())
	{
		if (document_match)
			rml_description += "<br/>" + reference_file;
		else
			rml_description += "<br/>(X " + reference_file + ")";
	}
	rml_description += "</p>";

	if(!link_list.empty())
	{
		rml_description += "<p class=\"links\">";
		for (const LinkItem& item : link_list)
		{
			if (item.rel == "match")
				continue;

			rml_description += "<a href=\"" + item.href + "\">" + item.rel + "</a> ";
		}
		rml_description += "</p>";
	}

	for (const MetaItem& item : meta_list)
	{
		rml_description += "<h3>" + item.name + "</h3>";
		rml_description += "<p style=\"min-height: 120px;\">" + item.content + "</p>";
	}

	Element* description_content = document_description->GetElementById("content");
	REQUIRE(description_content);
	description_content->SetInnerRML(rml_description);

	Element* description_test_suite = document_description->GetElementById("test_suite");
	REQUIRE(description_test_suite);
	description_test_suite->SetInnerRML(CreateString(64, "Test suite %d of %d", current_test_suite + 1, (int)test_suites.size()));

	Element* description_goto = document_description->GetElementById("goto");
	REQUIRE(description_goto);
	description_goto->SetInnerRML("");
	goto_id = 0;

	CloseSource();
}

void Window::ProcessEvent(Rml::Event& event)
{
	if (event == EventId::Keydown)
	{
		auto key_identifier = (Rml::Input::KeyIdentifier)event.GetParameter< int >("key_identifier", 0);
		bool key_ctrl = event.GetParameter< bool >("ctrl_key", false);
		bool key_shift = event.GetParameter< bool >("shift_key", false);

		if (key_identifier == Rml::Input::KI_LEFT)
		{
			current_id = std::max(0, current_id - 1);
			ReloadDocument();
		}
		else if (key_identifier == Rml::Input::KI_RIGHT)
		{
			current_id = std::min((int)GetCurrentTestSuite().files.size() - 1, current_id + 1);
			ReloadDocument();
		}
		else if (key_identifier == Rml::Input::KI_UP)
		{
			current_test_suite = std::max(0, current_test_suite - 1);
			current_id = 0;
			ReloadDocument();
		}
		else if (key_identifier == Rml::Input::KI_DOWN)
		{
			current_test_suite = std::min((int)test_suites.size() - 1, current_test_suite + 1);
			current_id = 0;
			ReloadDocument();
		}
		else if (key_identifier == Rml::Input::KI_S)
		{
			if (document_source->IsVisible())
			{
				if (key_shift)
					SwitchSource();
				else
					CloseSource();
			}
			else
			{
				viewing_reference_source = key_shift;
				OpenSource();
			}
		}
		else if (key_identifier == Rml::Input::KI_ESCAPE)
		{
			if (document_source->IsVisible())
				CloseSource();
			else
				TestsShell::RequestExit();
		}
		else if (key_identifier == Rml::Input::KI_C && key_ctrl)
		{
			if (key_shift)
				Shell::SetClipboardText(GetCurrentTestSuite().directory + '/' + reference_file);
			else
				Shell::SetClipboardText(GetCurrentPath());
		}
		else if (key_identifier == Rml::Input::KI_HOME)
		{
			current_id = 0;
			ReloadDocument();
		}
		else if (key_identifier == Rml::Input::KI_END)
		{
			current_id = (int)GetCurrentTestSuite().files.size() - 1;
			ReloadDocument();
		}
		else if (goto_id >= 0 && key_identifier == Rml::Input::KI_BACK)
		{
			if (goto_id <= 0)
			{
				goto_id = -1;
				document_description->GetElementById("goto")->SetInnerRML("");
			}
			else
			{
				goto_id = goto_id / 10;
				document_description->GetElementById("goto")->SetInnerRML(CreateString(64, "Go To: %d", goto_id));
			}
		}
	}

	if (event == EventId::Textinput)
	{
		const String text = event.GetParameter< String >("text", "");
		for (const char c : text)
		{
			if (c >= '0' && c <= '9')
			{
				if (goto_id < 0)
					goto_id = 0;

				goto_id = goto_id * 10 + int(c - '0');
				document_description->GetElementById("goto")->SetInnerRML(CreateString(64, "Go To: %d", goto_id));
			}
			else if (goto_id >= 0 && c == '\n')
			{
				if (goto_id > 0 && goto_id <= (int)GetCurrentTestSuite().files.size())
				{
					current_id = goto_id - 1;
					ReloadDocument();
				}
				else
				{
					document_description->GetElementById("goto")->SetInnerRML("Error: Go To out of bounds.");
				}
				goto_id = -1;
			}
		}
	}
}
