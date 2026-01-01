#include "Template.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../Include/RmlUi/Core/XMLParser.h"
#include "XMLParseTools.h"
#include <string.h>

namespace Rml {

Template::Template() {}

Template::~Template() {}

const String& Template::GetName() const
{
	return name;
}

bool Template::Load(Stream* stream)
{
	// Load the entire template into memory so we can pull out
	// the header and body tags
	String buffer;
	stream->Read(buffer, stream->Length());

	// Pull out the header
	const char* head_start = XMLParseTools::FindTag("head", buffer.c_str());
	if (!head_start)
		return false;

	const char* head_end = XMLParseTools::FindTag("head", head_start, true);
	if (!head_end)
		return false;
	// Advance to the end of the tag
	head_end = strchr(head_end, '>') + 1;

	// Pull out the body
	const char* body_start = XMLParseTools::FindTag("body", head_end);
	if (!body_start)
		return false;

	const char* body_end = XMLParseTools::FindTag("body", body_start, true);
	if (!body_end)
		return false;
	// Advance to the end of the tag
	body_end = strchr(body_end, '>') + 1;

	// Find the RML tag, skip over it and read the attributes,
	// storing the ones we're interested in.
	String attribute_name;
	String attribute_value;
	const char* ptr = XMLParseTools::FindTag("template", buffer.c_str());
	if (!ptr)
		return false;

	while (XMLParseTools::ReadAttribute(++ptr, attribute_name, attribute_value))
	{
		if (attribute_name == "name")
			name = attribute_value;
		if (attribute_name == "content")
			content = attribute_value;
	}

	// Create a stream around the header, parse it and store it
	auto header_stream = MakeUnique<StreamMemory>((const byte*)head_start, head_end - head_start);
	header_stream->SetSourceURL(stream->GetSourceURL());

	XMLParser parser(nullptr);
	parser.Parse(header_stream.get());

	header_stream.reset();

	header = *parser.GetDocumentHeader();

	// Store the body in stream form
	body = MakeUnique<StreamMemory>(body_end - body_start);
	body->SetSourceURL(stream->GetSourceURL());
	body->PushBack(body_start, body_end - body_start);

	return true;
}

Element* Template::ParseTemplate(Element* element)
{
	body->Seek(0, SEEK_SET);

	const int num_children_before = element->GetNumChildren();

	XMLParser parser(element);
	parser.Parse(body.get());

	// If there's an inject attribute on the template, attempt to find the required element.
	if (!content.empty())
	{
		const int num_children_after = element->GetNumChildren();

		// We look through the newly added elements (and only those), and look for the desired id.
		for (int i = num_children_before; i < num_children_after; ++i)
		{
			Element* content_element = ElementUtilities::GetElementById(element->GetChild(i), content);
			if (content_element)
			{
				element = content_element;
				break;
			}
		}
	}

	return element;
}

const DocumentHeader* Template::GetHeader()
{
	return &header;
}

} // namespace Rml
