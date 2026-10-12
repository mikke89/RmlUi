#pragma once

#include "../../Include/RmlUi/Core/ElementDocument.h"

namespace Rml {
namespace Debugger {

class ElementDebugDocument : public ElementDocument {
public:
	RMLUI_RTTI_DeclareWithParent(ElementDebugDocument, ElementDocument)

	ElementDebugDocument(const String& tag);
};

} // namespace Debugger
} // namespace Rml
