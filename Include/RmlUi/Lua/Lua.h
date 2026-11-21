#pragma once

#include "Header.h"

typedef struct lua_State lua_State;

namespace Rml {
namespace Lua {

	/** Initialise the Lua plugin.
	    @remark This is equivalent to calling Initialise(nullptr). */
	RMLUILUA_API void Initialise();

	/** Options for initializing the Lua plugin. */
	struct Options {
		/// Replace the global 'print' function with one that writes to the RmlUi log. Disable this to keep the
		/// application's own 'print' in the Lua state.
		bool replace_print = true;

		/// A Lua message handler for errors in Lua code run by the plugin. For runtime errors, Lua calls it at the point of
		/// the error with the error object as its argument, so it can inspect the call stack. For syntax errors, the plugin
		/// calls it with the error message after the code fails to load. When set, the plugin doesn't log these errors
		/// itself, but still logs errors that Lua doesn't pass to a message handler, such as running out of memory or an
		/// error inside the handler. By default, errors are logged as warnings with a stack traceback.
		int (*error_handler)(lua_State* L) = nullptr;
	};

	/** Initialise the Lua plugin and add RmlUi to an existing Lua state if one is provided.
	 @remark If nullptr is passed as an argument, the plugin will automatically create the lua state during initialisation
	   and close the state during the call to Rml::Shutdown(). Otherwise, if a Lua state is provided, the user is
	   responsible for closing the provided Lua state. The state must then be closed after the call to Rml::Shutdown().
	 @remark The plugin registers the "body" tag to generate a LuaDocument rather than a ElementDocument. */
	RMLUILUA_API void Initialise(lua_State* L, const Options& options = Options());

} // namespace Lua
} // namespace Rml
