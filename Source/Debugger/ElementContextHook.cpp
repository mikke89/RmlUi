#include "ElementContextHook.h"
#include "DebuggerPlugin.h"

namespace Rml {
namespace Debugger {

ElementContextHook::ElementContextHook(const String& tag) : ElementDebugDocument(tag)
{
	debugger = nullptr;
}

ElementContextHook::~ElementContextHook() {}

void ElementContextHook::Initialise(DebuggerPlugin* _debugger)
{
	SetId("rmlui-debug-hook");
	SetProperty(PropertyId::ZIndex, Property(999'999, Unit::NUMBER));
	debugger = _debugger;
}

void ElementContextHook::OnRender()
{
	// Make sure we're in the front of the render queue for this context (at least next frame).
	PullToFront();

	// Render the debugging elements.
	debugger->Render();
}

} // namespace Debugger
} // namespace Rml
