#include "ElementDebugDocument.h"

namespace Rml {
namespace Debugger {

RMLUI_RTTI_Define_Implementation_WithParent(ElementDebugDocument, ElementDocument)

ElementDebugDocument::ElementDebugDocument(const String& tag) : ElementDocument(tag)
{
	SetFocusableFromModal(true);
}

} // namespace Debugger
} // namespace Rml
