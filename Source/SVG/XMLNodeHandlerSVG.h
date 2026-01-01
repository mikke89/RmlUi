#pragma once

#include "../../Include/RmlUi/Core/XMLParser.h"
#include "../../Include/RmlUi/SVG/ElementSVG.h"
#include "../Core/XMLNodeHandlerDefault.h"

namespace Rml {
namespace SVG {
	/**
	    Element Node handler that processes the SVG tag
	 */
	class XMLNodeHandlerSVG : public XMLNodeHandlerDefault {
	public:
		/// Called for element data
		bool ElementData(XMLParser* parser, const String& data, XMLDataType type) override;
	};

} // namespace SVG
} // namespace Rml
