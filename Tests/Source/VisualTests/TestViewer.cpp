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
#include "TestConfig.h"
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


class EventListenerLinks : public Rml::EventListener {
public:

	void ProcessEvent(Rml::Event& event) override
	{
		Rml::Element* element = event.GetCurrentElement();
		Rml::String href = element->GetAttribute<Rml::String>("href", "");

		if (href.empty() || !hover_text)
			return;
		
		if (event == Rml::EventId::Click)
		{
			Shell::SetClipboardText(href);
			hover_text->SetInnerRML("Copied to clipboard");
			hover_text->SetClass("confirmation", true);
		}
		else if (event == Rml::EventId::Mouseover)
		{
			hover_text->SetInnerRML(Rml::StringUtilities::EncodeRml(href));
		}
		else if (event == Rml::EventId::Mouseout)
		{
			hover_text->SetInnerRML("");
			hover_text->SetClass("confirmation", false);
		}
	}

	void SetHoverTextElement(Element* element)
	{
		hover_text = element;
	}

private:

	Element* hover_text = nullptr;
};
static EventListenerLinks event_listener_links;


TestViewer::TestViewer(Rml::Context* context) : context(context)
{
	InitializeXmlNodeHandlers();

	const String local_data_path_prefix = "/../Tests/Data/";

	document_description = context->LoadDocument(local_data_path_prefix + "description.rml");
	RMLUI_ASSERT(document_description);
	event_listener_links.SetHoverTextElement(document_description->GetElementById("hovertext"));
	document_description->Show();

	document_source = context->LoadDocument(local_data_path_prefix + "view_source.rml");
	RMLUI_ASSERT(document_source);
	document_help = context->LoadDocument(local_data_path_prefix + "visual_tests_help.rml");
	RMLUI_ASSERT(document_help);
	if (Element* element = document_help->GetElementById("test_directories"))
	{
		String rml;
		const StringList dirs = GetTestInputDirectories();
		for (const String& dir : dirs)
			rml += "<value>" + Rml::StringUtilities::EncodeRml(dir) + "</value>";
		element->SetInnerRML(rml);
	}
	if (Element* element = document_help->GetElementById("compare_input"))
	{
		element->SetInnerRML(Rml::StringUtilities::EncodeRml(GetCompareInputDirectory()));
	}
	if (Element* element = document_help->GetElementById("capture_output"))
	{
		element->SetInnerRML(Rml::StringUtilities::EncodeRml(GetCaptureOutputDirectory()));
	}
}

TestViewer::~TestViewer()
{
	event_listener_links.SetHoverTextElement(nullptr);

	for (ElementDocument* doc : { document_test, document_description, document_source, document_reference, document_help })
	{
		if (doc)
			doc->Close();
	}
}

static Rml::String LoadFile(const String& file_path)
{
	String result;
	Rml::GetFileInterface()->LoadFile(file_path, result);
	return result;
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

			document_source->Show(ModalFlag::None, FocusFlag::None);
		}
	}
}

void TestViewer::ShowHelp(bool show)
{
	if (show)
		document_help->Show();
	else
		document_help->Hide();
}

bool TestViewer::IsHelpVisible() const
{
	return document_help->IsVisible();
}



bool TestViewer::LoadTest(const Rml::String& directory, const Rml::String& filename, int test_index, int number_of_tests, int filtered_test_index, int filtered_number_of_tests, int suite_index, int number_of_suites)
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

	const Rml::String test_path = directory + '/' + filename;
	Rml::String reference_path;

	// Load test document, and reference document if it exists.
	{
		source_test = LoadFile(test_path);
		if (source_test.empty())
			return false;

		document_test = context->LoadDocumentFromMemory(source_test, Rml::StringUtilities::Replace(test_path, ':', '|'));
		if (!document_test)
			return false;

		document_test->Show(ModalFlag::None, FocusFlag::None);

		for (const LinkItem& item : link_handler->GetLinkList())
		{
			if (item.rel == "match")
			{
				reference_filename = item.href;
				break;
			}
		}

		reference_path = directory + '/' + reference_filename;

		if (!reference_filename.empty())
		{
			source_reference = LoadFile(reference_path);

			if (!source_reference.empty())
			{
				document_reference = context->LoadDocumentFromMemory(source_reference, Rml::StringUtilities::Replace(reference_path, ':', '|'));
				if (document_reference)
				{
					document_reference->SetProperty(PropertyId::Left, Property(510.f, Property::DP));
					document_reference->Show(ModalFlag::None, FocusFlag::None);
				}
			}
		}
	}

	// Description Header
	{
		Element* description_header = document_description->GetElementById("header");
		RMLUI_ASSERT(description_header);

		description_header->SetInnerRML(CreateString(512, "Test suite %d of %d<br/>Test %d of %d<br/>",
			suite_index + 1, number_of_suites, test_index + 1, number_of_tests));
	}

	// Description Filter
	{
		Element* description_filter_text = document_description->GetElementById("filter_text");
		RMLUI_ASSERT(description_filter_text);
		if (filtered_number_of_tests == 0)
			description_filter_text->SetInnerRML("No matches");
		else if (filtered_number_of_tests < number_of_tests && filtered_test_index >= 0)
			description_filter_text->SetInnerRML(CreateString(128, "Filtered %d of %d", filtered_test_index + 1, filtered_number_of_tests));
		else if (filtered_number_of_tests < number_of_tests && filtered_test_index < 0)
			description_filter_text->SetInnerRML(CreateString(128, "Filtered X of %d", filtered_number_of_tests));
		else
			description_filter_text->SetInnerRML("");
	}


	// Description Content
	{
		String rml_description = Rml::CreateString(512, "<h1>%s</h1><p><a href=\"%s\">%s</a>",
			document_test->GetTitle().c_str(), test_path.c_str(), filename.c_str());

		if (!reference_filename.empty())
		{
			if (document_reference)
				rml_description += "<br/><a href=\"" + reference_path + "\">" + reference_filename + "</a>";
			else
				rml_description += "<br/>(missing)&nbsp;" + reference_filename + "";
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
			rml_description += "<p>" + item.content + "</p>";
		}

		Element* description_content = document_description->GetElementById("content");
		RMLUI_ASSERT(description_content);
		description_content->SetInnerRML(rml_description);

		// Add link hover and click handler.
		Rml::ElementList link_elements;
		description_content->GetElementsByTagName(link_elements, "a");

		for (Rml::Element* element : link_elements) {
			element->AddEventListener(Rml::EventId::Click, &event_listener_links);
			element->AddEventListener(Rml::EventId::Mouseover, &event_listener_links);
			element->AddEventListener(Rml::EventId::Mouseout, &event_listener_links);
		}
	}

	return true;
}

void TestViewer::SetGoToText(const Rml::String& rml)
{
	Element* description_goto = document_description->GetElementById("goto");
	RMLUI_ASSERT(description_goto);
	description_goto->SetInnerRML(rml);
}
