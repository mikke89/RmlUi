#pragma once

#include "Header.h"
#include "Traits.h"
#include "Types.h"

namespace Rml {

class Element;
class XMLParser;
enum class XMLDataType;

/**
    A handler gets ElementStart, ElementEnd and ElementData called by the XMLParser.
 */

class RMLUICORE_API XMLNodeHandler : public NonCopyMoveable {
public:
	virtual ~XMLNodeHandler();

	/// Called when a new element tag is opened.
	/// @param parser The parser executing the parse.
	/// @param name The XML tag name.
	/// @param attributes The tag attributes.
	/// @return The new element, may be nullptr if no element was created.
	virtual Element* ElementStart(XMLParser* parser, const String& name, const XMLAttributes& attributes) = 0;

	/// Called when an element is closed.
	/// @param parser The parser executing the parse.
	/// @param name The XML tag name.
	virtual bool ElementEnd(XMLParser* parser, const String& name) = 0;

	/// Called for element data.
	/// @param parser The parser executing the parse.
	/// @param data The element data.
	virtual bool ElementData(XMLParser* parser, const String& data, XMLDataType type) = 0;
};

} // namespace Rml
