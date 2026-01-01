#ifndef RMLUI_CORE_XMLNODEHANDLERBODY_H
#define RMLUI_CORE_XMLNODEHANDLERBODY_H

#include "../../Include/RmlUi/Core/XMLNodeHandler.h"

namespace Rml {

/**
    Element Node handler that processes the HEAD tag

    @author Lloyd Weehuizen
 */

class XMLNodeHandlerBody : public XMLNodeHandler {
public:
	XMLNodeHandlerBody();
	~XMLNodeHandlerBody();

	/// Called when a new element start is opened
	Element* ElementStart(XMLParser* parser, const String& name, const XMLAttributes& attributes) override;
	/// Called when an element is closed
	bool ElementEnd(XMLParser* parser, const String& name) override;
	/// Called for element data
	bool ElementData(XMLParser* parser, const String& data, XMLDataType type) override;
};

} // namespace Rml
#endif
