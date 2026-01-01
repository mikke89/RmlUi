#include "../../Include/RmlUi/Core/XMLParser.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/Log.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/Stream.h"
#include "../../Include/RmlUi/Core/Types.h"
#include "../../Include/RmlUi/Core/URL.h"
#include "../../Include/RmlUi/Core/XMLNodeHandler.h"
#include "ControlledLifetimeResource.h"
#include "DocumentHeader.h"

namespace Rml {

struct XmlParserData {
	UnorderedMap<String, SharedPtr<XMLNodeHandler>> node_handlers;
	UnorderedSet<String> cdata_tags;
	SharedPtr<XMLNodeHandler> default_node_handler;
};

static ControlledLifetimeResource<XmlParserData> xml_parser_data;

XMLParser::XMLParser(Element* root)
{
	for (const String& cdata_tag : xml_parser_data->cdata_tags)
		RegisterCDATATag(cdata_tag);

	for (const String& name : Factory::GetStructuralDataViewAttributeNames())
		RegisterInnerXMLAttribute(name);

	// Add the first frame.
	ParseFrame frame;
	frame.element = root;
	if (root != nullptr)
	{
		frame.tag = root->GetTagName();
		auto itr = xml_parser_data->node_handlers.find(root->GetTagName());
		if (itr != xml_parser_data->node_handlers.end())
		{
			frame.node_handler = itr->second.get();
		}
		else
		{
			frame.node_handler = xml_parser_data->default_node_handler.get();
		}
	}
	stack.push(frame);

	active_handler = nullptr;

	header = MakeUnique<DocumentHeader>();
}

XMLParser::~XMLParser() {}

void XMLParser::RegisterPersistentCDATATag(const String& _tag)
{
	if (!xml_parser_data)
		xml_parser_data.Initialize();

	String tag = StringUtilities::ToLower(_tag);

	if (tag.empty())
		return;

	xml_parser_data->cdata_tags.insert(tag);
}

XMLNodeHandler* XMLParser::RegisterNodeHandler(const String& _tag, SharedPtr<XMLNodeHandler> handler)
{
	if (!xml_parser_data)
		xml_parser_data.Initialize();

	String tag = StringUtilities::ToLower(_tag);

	// Check for a default node registration.
	if (tag.empty())
	{
		xml_parser_data->default_node_handler = std::move(handler);
		return xml_parser_data->default_node_handler.get();
	}

	XMLNodeHandler* result = handler.get();
	xml_parser_data->node_handlers[tag] = std::move(handler);
	return result;
}

XMLNodeHandler* XMLParser::GetNodeHandler(const String& tag)
{
	auto it = xml_parser_data->node_handlers.find(tag);
	if (it != xml_parser_data->node_handlers.end())
		return it->second.get();

	return nullptr;
}

void XMLParser::ReleaseHandlers()
{
	xml_parser_data.Shutdown();
}

DocumentHeader* XMLParser::GetDocumentHeader()
{
	return header.get();
}

void XMLParser::PushDefaultHandler()
{
	active_handler = xml_parser_data->default_node_handler.get();
}

bool XMLParser::PushHandler(const String& tag)
{
	auto it = xml_parser_data->node_handlers.find(StringUtilities::ToLower(tag));
	if (it == xml_parser_data->node_handlers.end())
		return false;

	active_handler = it->second.get();
	return true;
}

const XMLParser::ParseFrame* XMLParser::GetParseFrame() const
{
	return &stack.top();
}

const URL& XMLParser::GetSourceURL() const
{
	RMLUI_ASSERT(GetSourceURLPtr());
	return *GetSourceURLPtr();
}

void XMLParser::HandleElementStart(const String& _name, const XMLAttributes& attributes)
{
	RMLUI_ZoneScoped;
	const String name = StringUtilities::ToLower(_name);

	// Check for a specific handler that will override the child handler.
	auto itr = xml_parser_data->node_handlers.find(name);
	if (itr != xml_parser_data->node_handlers.end())
		active_handler = itr->second.get();

	// Store the current active handler, so we can use it through this function (as active handler may change)
	XMLNodeHandler* node_handler = active_handler;

	Element* element = nullptr;

	// Get the handler to handle the open tag
	if (node_handler)
	{
		element = node_handler->ElementStart(this, name, attributes);
	}

	// Push onto the stack
	ParseFrame frame;
	frame.node_handler = node_handler;
	frame.child_handler = active_handler;
	frame.element = (element ? element : stack.top().element);
	frame.tag = name;
	stack.push(frame);
}

void XMLParser::HandleElementEnd(const String& _name)
{
	RMLUI_ZoneScoped;
	String name = StringUtilities::ToLower(_name);

	// Copy the top of the stack
	ParseFrame frame = stack.top();
	// Pop the frame
	stack.pop();
	// Restore active handler to the previous frame's child handler
	active_handler = stack.top().child_handler;

	// Check frame names
	if (name != frame.tag)
	{
		Log::Message(Log::LT_ERROR, "Closing tag '%s' mismatched on %s:%d was expecting '%s'.", name.c_str(), GetSourceURL().GetURL().c_str(),
			GetLineNumber(), frame.tag.c_str());
	}

	// Call element end handler
	if (frame.node_handler)
	{
		frame.node_handler->ElementEnd(this, name);
	}
}

void XMLParser::HandleData(const String& data, XMLDataType type)
{
	RMLUI_ZoneScoped;
	if (stack.top().node_handler)
		stack.top().node_handler->ElementData(this, data, type);
}

} // namespace Rml
