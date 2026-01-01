#include "XMLNodeHandlerSVG.h"

namespace Rml {
namespace SVG {
	bool XMLNodeHandlerSVG::ElementData(XMLParser* parser, const String& data, XMLDataType /*type*/)
	{
		auto* element = rmlui_dynamic_cast<ElementSVG*>(parser->GetParseFrame()->element);
		RMLUI_ASSERT(element);
		if (element)
			element->SetInnerRML(data);
		return true;
	}
} // namespace SVG
} // namespace Rml
