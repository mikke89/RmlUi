#ifndef RMLUI_DEBUGGER_ELEMENTCONTEXTHOOK_H
#define RMLUI_DEBUGGER_ELEMENTCONTEXTHOOK_H

#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "ElementDebugDocument.h"

namespace Rml {
namespace Debugger {

class DebuggerPlugin;

/**
    An element that the debugger uses to render into a foreign context.

    @author Peter Curry
 */

class ElementContextHook : public ElementDebugDocument {
public:
	RMLUI_RTTI_DefineWithParent(ElementContextHook, ElementDebugDocument)

	ElementContextHook(const String& tag);
	virtual ~ElementContextHook();

	void Initialise(DebuggerPlugin* debugger);

	void OnRender() override;

private:
	DebuggerPlugin* debugger;
};

} // namespace Debugger
} // namespace Rml

#endif
