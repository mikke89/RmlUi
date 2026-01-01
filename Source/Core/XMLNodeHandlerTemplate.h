#pragma once

#include "../../Include/RmlUi/Core/XMLNodeHandler.h"

namespace Rml {

/**
    Element Node handler that processes the custom template tags
 */

class XMLNodeHandlerTemplate : public XMLNodeHandler {
public:
	XMLNodeHandlerTemplate();
	~XMLNodeHandlerTemplate();

	/// Called when a new element start is opened
	Element* ElementStart(XMLParser* parser, const String& name, const XMLAttributes& attributes) override;
	/// Called when an element is closed
	bool ElementEnd(XMLParser* parser, const String& name) override;
	/// Called for element data
	bool ElementData(XMLParser* parser, const String& data, XMLDataType type) override;
};

} // namespace Rml
