#ifndef RMLUI_CORE_ELEMENTS_XMLNODEHANDLERTEXTAREA_H
#define RMLUI_CORE_ELEMENTS_XMLNODEHANDLERTEXTAREA_H

#include "../../../Include/RmlUi/Core/XMLNodeHandler.h"

namespace Rml {

/**
    Node handler that processes the contents of the textarea tag.

    @author Peter Curry
 */

class XMLNodeHandlerTextArea : public XMLNodeHandler {
public:
	XMLNodeHandlerTextArea();
	virtual ~XMLNodeHandlerTextArea();

	/// Called when a new element is opened.
	Element* ElementStart(XMLParser* parser, const String& name, const XMLAttributes& attributes) override;
	/// Called when an element is closed.
	bool ElementEnd(XMLParser* parser, const String& name) override;
	/// Called for element data.
	bool ElementData(XMLParser* parser, const String& data, XMLDataType type) override;
};

} // namespace Rml
#endif
