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
	};

	/** Initialise the Lua plugin and add RmlUi to an existing Lua state if one is provided.
	 @remark If nullptr is passed as an argument, the plugin will automatically create the lua state during initialisation
	   and close the state during the call to Rml::Shutdown(). Otherwise, if a Lua state is provided, the user is
	   responsible for closing the provided Lua state. The state must then be closed after the call to Rml::Shutdown().
	 @remark The plugin registers the "body" tag to generate a LuaDocument rather than a ElementDocument. */
	RMLUILUA_API void Initialise(lua_State* L, const Options& options = Options());

} // namespace Lua
} // namespace Rml
