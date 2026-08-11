#include "XMLNodeHandlerHead.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/StringUtilities.h"
#include "../../Include/RmlUi/Core/SystemInterface.h"
#include "../../Include/RmlUi/Core/URL.h"
#include "../../Include/RmlUi/Core/XMLParser.h"
#include "DocumentHeader.h"
#include "TemplateCache.h"

namespace Rml {

static String AbsolutePath(const String& source, const String& base)
{
	String joined_path;
	::Rml::GetSystemInterface()->JoinPath(joined_path, StringUtilities::Replace(base, '|', ':'), StringUtilities::Replace(source, '|', ':'));
	return StringUtilities::Replace(joined_path, ':', '|');
}

static DocumentHeader::Resource MakeInlineResource(XMLParser* parser, const String& data)
{
	DocumentHeader::Resource resource;
	resource.is_inline = true;
	resource.content = data;
	resource.path = parser->GetSourceURL().GetURL();
	resource.line = parser->GetLineNumberOpenTag();
	return resource;
}

static DocumentHeader::Resource MakeExternalResource(XMLParser* parser, const String& path)
{
	DocumentHeader::Resource resource;
	resource.is_inline = false;
	resource.path = AbsolutePath(path, parser->GetSourceURL().GetURL());
	return resource;
}

XMLNodeHandlerHead::XMLNodeHandlerHead() {}

XMLNodeHandlerHead::~XMLNodeHandlerHead() {}

Element* XMLNodeHandlerHead::ElementStart(XMLParser* parser, const String& name, const XMLAttributes& attributes)
{
	if (name == "head")
	{
		parser->GetDocumentHeader()->source = parser->GetSourceURL().GetURL();
	}

	else if (name == "link")
	{
		// Lookup the type and href
		String type = StringUtilities::ToLower(Get<String>(attributes, "type", ""));
		String href = Get<String>(attributes, "href", "");

		if (!type.empty() && !href.empty())
		{
			// If its RCSS (... or CSS!), add to the RCSS fields.
			if (type == "text/rcss" || type == "text/css")
			{
				parser->GetDocumentHeader()->rcss.push_back(MakeExternalResource(parser, href));
			}

			// If its an template, add to the template fields
			else if (type == "text/template")
			{
				parser->GetDocumentHeader()->template_resources.push_back(href);
			}

			else
			{
				Log::ParseError(parser->GetSourceURL().GetURL(), parser->GetLineNumber(), "Invalid link type '%s'", type.c_str());
			}
		}
		else
		{
			Log::ParseError(parser->GetSourceURL().GetURL(), parser->GetLineNumber(), "Link tag requires type and href attributes");
		}
	}

	else if (name == "script")
	{
		// Check if its an external string
		String src = Get<String>(attributes, "src", "");
		if (src.size() > 0)
		{
			parser->GetDocumentHeader()->scripts.push_back(MakeExternalResource(parser, src));
		}
	}
	else if (name == "style")
	{
		// Add a placeholder for the style sheet, its contents are filled in as we encounter the tag's data. The optional
		// 'path' attribute declares the location the contents originate from, so that relative URLs in a serialized
		// document still resolve against their original source. With 'inline' disabled the contents are instead loaded
		// from that path, just like a link tag.
		const String path = Get<String>(attributes, "path", "");
		const bool is_inline = path.empty() || Get<bool>(attributes, "inline", true);

		DocumentHeader::Resource resource;
		if (!is_inline)
		{
			resource = MakeExternalResource(parser, path);
		}
		else
		{
			resource.is_inline = true;
			resource.path = (path.empty() ? parser->GetSourceURL().GetURL() : path);
			resource.line = (path.empty() ? parser->GetLineNumberOpenTag() : 0);
		}

		parser->GetDocumentHeader()->rcss.push_back(std::move(resource));
	}

	// No elements constructed
	return nullptr;
}

bool XMLNodeHandlerHead::ElementEnd(XMLParser* parser, const String& name)
{
	// When the head tag closes, inject the header into the active document
	if (name == "head")
	{
		Element* element = parser->GetParseFrame()->element;
		if (!element)
			return true;

		ElementDocument* document = element->GetOwnerDocument();
		if (document)
			document->ProcessHeader(parser->GetDocumentHeader(), parser->GetCallbackCompiledDocumentHead());
	}
	return true;
}

bool XMLNodeHandlerHead::ElementData(XMLParser* parser, const String& data, XMLDataType /*type*/)
{
	const String& tag = parser->GetParseFrame()->tag;

	// Store the title
	if (tag == "title")
	{
		SystemInterface* system_interface = GetSystemInterface();
		if (system_interface != nullptr)
			system_interface->TranslateString(parser->GetDocumentHeader()->title, data);
	}

	// Store an inline script
	if (tag == "script" && data.size() > 0)
	{
		parser->GetDocumentHeader()->scripts.push_back(MakeInlineResource(parser, data));
	}

	// Store the inline style data, we've already added a placeholder from ElementStart.
	DocumentHeader::ResourceList& rcss = parser->GetDocumentHeader()->rcss;
	if (tag == "style" && data.size() > 0 && !rcss.empty())
	{
		rcss.back().content = data;
	}

	return true;
}

} // namespace Rml
