/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef RMLUI_LUA_LUA_H
#define RMLUI_LUA_LUA_H

#include "Header.h"

typedef struct lua_State lua_State;

namespace Rml {
namespace Lua {

	/** Initialise the Lua plugin.
	    @remark This is equivalent to calling Initialise(nullptr). */
	RMLUILUA_API void Initialise();

	/** Initialise the Lua plugin and add RmlUi to an existing Lua state if one is provided.
	 @remark If nullptr is passed as an argument, the plugin will automatically create the lua state during initialisation
	   and close the state during the call to Rml::Shutdown(). Otherwise, if a Lua state is provided, the user is
	   responsible for closing the provided Lua state. The state must then be closed after the call to Rml::Shutdown().
	 @remark The plugin registers the "body" tag to generate a LuaDocument rather than a ElementDocument. */
	RMLUILUA_API void Initialise(lua_State* L);

} // namespace Lua
} // namespace Rml
#endif
