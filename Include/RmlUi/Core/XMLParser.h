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

#ifndef RMLUI_CORE_XMLPARSER_H
#define RMLUI_CORE_XMLPARSER_H

#include "BaseXMLParser.h"
#include "Header.h"
#include <stack>

namespace Rml {

class DocumentHeader;
class Element;
class XMLNodeHandler;
class URL;

/**
    RmlUi's XML parsing engine. The factory creates an instance of this class for each RML parse.

    @author Lloyd Weehuizen
 */

class RMLUICORE_API XMLParser : public BaseXMLParser {
public:
	XMLParser(Element* root);
	~XMLParser();

	/// Registers a custom node handler to be used to a given tag.
	/// @param[in] tag The tag the custom parser will handle.
	/// @param[in] handler The custom handler.
	/// @return The registered XML node handler.
	static XMLNodeHandler* RegisterNodeHandler(const String& tag, SharedPtr<XMLNodeHandler> handler);
	/// Retrieve a registered node handler.
	/// @param[in] tag The tag the custom parser handles.
	/// @return The registered XML node handler or nullptr if it does not exist for the given tag.
	static XMLNodeHandler* GetNodeHandler(const String& tag);
	/// Releases all registered node handlers. This is called internally.
	static void ReleaseHandlers();

	/// Returns the XML document's header.
	/// @return The document header.
	DocumentHeader* GetDocumentHeader();

	// The parse stack.
	struct ParseFrame {
		// Tag being parsed.
		String tag;

		// Element representing this frame.
		Element* element = nullptr;

		// Handler used for this frame.
		XMLNodeHandler* node_handler = nullptr;

		// The default handler used for this frame's children.
		XMLNodeHandler* child_handler = nullptr;
	};

	/// Pushes an element handler onto the parse stack for parsing child elements.
	/// @param[in] tag The tag the handler was registered with.
	/// @return True if an appropriate handler was found and pushed onto the stack, false if not.
	bool PushHandler(const String& tag);
	/// Pushes the default element handler onto the parse stack.
	void PushDefaultHandler();

	/// Access the current parse frame.
	const ParseFrame* GetParseFrame() const;

	/// Returns the source URL of this parse.
	const URL& GetSourceURL() const;

protected:
	/// Called when the parser finds the beginning of an element tag.
	void HandleElementStart(const String& name, const XMLAttributes& attributes) override;
	/// Called when the parser finds the end of an element tag.
	void HandleElementEnd(const String& name) override;
	/// Called when the parser encounters data.
	void HandleData(const String& data, XMLDataType type) override;

private:
	// The header of the document being parsed.
	UniquePtr<DocumentHeader> header;

	// The active node handler.
	XMLNodeHandler* active_handler;

	// The parser stack.
	using ParserStack = Stack<ParseFrame>;
	ParserStack stack;
};

} // namespace Rml
#endif
