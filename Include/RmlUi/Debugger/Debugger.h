#pragma once

#include "Header.h"

namespace Rml {

class Context;

namespace Debugger {

	/// Initialises the debug plugin. The debugger will be loaded into the given context.
	/// @param[in] host_context RmlUi context to load the debugger into. The debugging tools will be displayed on this context. If this context is
	///     destroyed, the debugger will be released.
	/// @return True if the debugger was successfully initialised
	RMLUIDEBUGGER_API bool Initialise(Context* host_context);

	/// Shuts down the debugger.
	/// @note The debugger is automatically shutdown during the call to Rml::Shutdown(), calling this is only necessary to shutdown the debugger early
	///     or to re-initialize the debugger on another host context.
	RMLUIDEBUGGER_API void Shutdown();

	/// Sets the context to be debugged.
	/// @param[in] context The context to be debugged.
	/// @return True if the debugger is initialised and the context was switched, false otherwise.
	RMLUIDEBUGGER_API bool SetContext(Context* context);

	/// Sets the visibility of the debugger.
	/// @param[in] visibility True to show the debugger, false to hide it.
	RMLUIDEBUGGER_API void SetVisible(bool visibility);
	/// Returns the visibility of the debugger.
	/// @return True if the debugger is visible, false if not.
	RMLUIDEBUGGER_API bool IsVisible();

} // namespace Debugger
} // namespace Rml
