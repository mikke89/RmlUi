#pragma once

#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "ElementDebugDocument.h"

namespace Rml {
namespace Debugger {

class DebuggerPlugin;

/**
    An element that the debugger uses to render into a foreign context.
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
