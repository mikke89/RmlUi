/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
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

#include "../../Include/RmlUi/Core/XMLParser.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/Log.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/Stream.h"
#include "../../Include/RmlUi/Core/Types.h"
#include "../../Include/RmlUi/Core/URL.h"
#include "../../Include/RmlUi/Core/XMLNodeHandler.h"
#include "DocumentHeader.h"

namespace Rml {

using NodeHandlers = UnorderedMap<String, SharedPtr<XMLNodeHandler>>;
static NodeHandlers node_handlers;
static SharedPtr<XMLNodeHandler> default_node_handler;

XMLParser::XMLParser(Element* root)
{
	RegisterCDATATag("script");
	RegisterCDATATag("style");

	for (const String& name : Factory::GetStructuralDataViewAttributeNames())
		RegisterInnerXMLAttribute(name);

	// Add the first frame.
	ParseFrame frame;
	frame.element = root;
	stack.push(frame);

	active_handler = nullptr;

	header = MakeUnique<DocumentHeader>();
}

XMLParser::~XMLParser() {}

XMLNodeHandler* XMLParser::RegisterNodeHandler(const String& _tag, SharedPtr<XMLNodeHandler> handler)
{
	String tag = StringUtilities::ToLower(_tag);

	// Check for a default node registration.
	if (tag.empty())
	{
		default_node_handler = std::move(handler);
		return default_node_handler.get();
	}

	XMLNodeHandler* result = handler.get();
	node_handlers[tag] = std::move(handler);
	return result;
}

XMLNodeHandler* XMLParser::GetNodeHandler(const String& tag)
{
	auto it = node_handlers.find(tag);
	if (it != node_handlers.end())
		return it->second.get();

	return nullptr;
}

void XMLParser::ReleaseHandlers()
{
	default_node_handler.reset();
	node_handlers.clear();
}

DocumentHeader* XMLParser::GetDocumentHeader()
{
	return header.get();
}

void XMLParser::PushDefaultHandler()
{
	active_handler = default_node_handler.get();
}

bool XMLParser::PushHandler(const String& tag)
{
	NodeHandlers::iterator i = node_handlers.find(StringUtilities::ToLower(tag));
	if (i == node_handlers.end())
		return false;

	active_handler = i->second.get();
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
	NodeHandlers::iterator itr = node_handlers.find(name);
	if (itr != node_handlers.end())
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
