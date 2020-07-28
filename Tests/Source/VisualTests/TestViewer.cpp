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

#include "TestViewer.h"
#include "XmlNodeHandlers.h"
#include "../Common/TestsShell.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Types.h>
#include <RmlUi/Core/XMLParser.h>
#include <Shell.h>

using namespace Rml;

static SharedPtr<XMLNodeHandlerMeta> meta_handler;
static SharedPtr<XMLNodeHandlerLink> link_handler;

static void InitializeXmlNodeHandlers()
{
	meta_handler = MakeShared<XMLNodeHandlerMeta>();
	Rml::XMLParser::RegisterNodeHandler("meta", meta_handler);

	link_handler = MakeShared<XMLNodeHandlerLink>();
	Rml::XMLParser::RegisterNodeHandler("link", link_handler);
}


TestViewer::TestViewer(Rml::Context* context) : context(context)
{
	InitializeXmlNodeHandlers();

	const String local_data_path_prefix = "/../Tests/Data/";

	document_description = context->LoadDocument(local_data_path_prefix + "description.rml");
	RMLUI_ASSERT(document_description);
	document_description->Show();

	document_source = context->LoadDocument(local_data_path_prefix + "view_source.rml");
	RMLUI_ASSERT(document_source);
}

TestViewer::~TestViewer()
{
	for (ElementDocument* doc : { document_test, document_description, document_source, document_reference })
	{
		if (doc)
			doc->Close();
	}
}

static Rml::String LoadFile(const String& file_path)
{
	FileInterface* file = Rml::GetFileInterface();
	FileHandle handle = file->Open(file_path);
	if (!handle)
	{
		return Rml::String();
	}

	const size_t length = file->Length(handle);
	UniquePtr<char[]> buf = UniquePtr<char[]>(new char[length + 1]);

	const size_t read_length = file->Read(buf.get(), length, handle);
	file->Close(handle);
	RMLUI_ASSERT(read_length > 0);
	RMLUI_ASSERT(read_length <= length);
	buf[read_length] = '\0';

	return String(buf.get());
}

void TestViewer::ShowSource(SourceType type)
{
	if (type == SourceType::None)
	{
		document_source->Hide();
	}
	else
	{
		const String& source_string = (type == SourceType::Test ? source_test : source_reference);

		if (source_string.empty())
		{
			document_source->Hide();
		}
		else
		{
			const String rml_source = StringUtilities::EncodeRml(source_string);

			Element* element = document_source->GetElementById("code");
			RMLUI_ASSERT(element);
			element->SetInnerRML(rml_source);

			document_source->Show();
		}
	}
}



bool TestViewer::LoadTest(const Rml::String& directory, const Rml::String& filename, int test_index, int number_of_tests, int suite_index, int number_of_suites)
{
	if (document_test)
	{
		document_test->Close();
		document_test = nullptr;
	}
	if (document_reference)
	{
		document_reference->Close();
		document_reference = nullptr;
	}

	reference_filename.clear();
	source_test.clear();
	source_reference.clear();

	meta_handler->ClearMetaList();
	link_handler->ClearLinkList();

	Element* description_test_suite = document_description->GetElementById("test_suite");
	RMLUI_ASSERT(description_test_suite);
	description_test_suite->SetInnerRML(CreateString(64, "Test suite %d of %d", suite_index + 1, number_of_suites));

	SetGoToText("");

	source_test = LoadFile(directory + '/' + filename);
	if (source_test.empty())
		return false;

	document_test = context->LoadDocumentFromMemory(source_test);
	if (!document_test)
		return false;

	document_test->Show();

	for (const LinkItem& item : link_handler->GetLinkList())
	{
		if (item.rel == "match")
		{
			reference_filename = item.href;
			break;
		}
	}

	if (!reference_filename.empty())
	{
		source_reference = LoadFile(directory + '/' + reference_filename);

		if (!source_reference.empty())
		{
			document_reference = context->LoadDocumentFromMemory(source_reference);
			if (document_reference)
			{
				document_reference->SetProperty(PropertyId::Left, Property(510.f, Property::PX));
				document_reference->Show();
			}
		}
	}

	String rml_description = Rml::CreateString(512, "<h1>%s</h1><p>Test %d of %d.<br/>%s", document_test->GetTitle().c_str(), test_index + 1, number_of_tests, filename.c_str());
	if (!reference_filename.empty())
	{
		if (document_reference)
			rml_description += "<br/>" + reference_filename;
		else
			rml_description += "<br/>(X " + reference_filename + ")";
	}
	rml_description += "</p>";

	const LinkList& link_list = link_handler->GetLinkList();
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

	for (const MetaItem& item : meta_handler->GetMetaList())
	{
		rml_description += "<h3>" + item.name + "</h3>";
		rml_description += "<p style=\"min-height: 120px;\">" + item.content + "</p>";
	}

	Element* description_content = document_description->GetElementById("content");
	RMLUI_ASSERT(description_content);
	description_content->SetInnerRML(rml_description);

	return true;
}

void TestViewer::SetGoToText(const Rml::String& rml)
{
	Element* description_goto = document_description->GetElementById("goto");
	RMLUI_ASSERT(description_goto);
	description_goto->SetInnerRML(rml);
}

const Rml::String& TestViewer::GetReferenceFilename()
{
	return reference_filename;
}
