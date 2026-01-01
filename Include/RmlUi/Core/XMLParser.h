#pragma once

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
 */

class RMLUICORE_API XMLParser : public BaseXMLParser {
public:
	XMLParser(Element* root);
	~XMLParser();

	/// Registers a tag were its contents should be treated as CDATA.
	///		Whereas BaseXMLParser RegisterCDataTag only registeres a
	///		tag for a parser instance, this function willl register
	///		a tag for all XMLParser instances created after this call.
	/// @param[in] _tag The tag for contents to be treated as CDATA
	static void RegisterPersistentCDATATag(const String& _tag);

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

	/// Called when the parser encounters data.
	void HandleData(const String& data, XMLDataType type) override;

protected:
	/// Called when the parser finds the beginning of an element tag.
	void HandleElementStart(const String& name, const XMLAttributes& attributes) override;
	/// Called when the parser finds the end of an element tag.
	void HandleElementEnd(const String& name) override;

private:
	UniquePtr<DocumentHeader> header;
	XMLNodeHandler* active_handler;
	Stack<ParseFrame> stack;
};

} // namespace Rml
