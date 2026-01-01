#ifndef RMLUI_DEBUGGER_ELEMENTDEBUGDOCUMENT_H
#define RMLUI_DEBUGGER_ELEMENTDEBUGDOCUMENT_H

#include "../../Include/RmlUi/Core/ElementDocument.h"

namespace Rml {
namespace Debugger {

class ElementDebugDocument : public ElementDocument {
public:
	RMLUI_RTTI_DefineWithParent(ElementDebugDocument, ElementDocument)

	ElementDebugDocument(const String& tag);
};

} // namespace Debugger
} // namespace Rml

#endif
