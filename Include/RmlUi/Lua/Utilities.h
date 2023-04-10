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

#ifndef RMLUI_LUA_UTILITIES_H
#define RMLUI_LUA_UTILITIES_H
/*
    This file is for free-floating functions that are used across more than one file.
*/
#include "Header.h"
#include "IncludeLua.h"
#include "LuaType.h"
#include <RmlUi/Core/Types.h>

namespace Rml {
namespace Lua {

#if LUA_VERSION_NUM < 502
	#define lua_setuservalue(L, i) (luaL_checktype((L), -1, LUA_TTABLE), lua_setfenv((L), (i)))

	inline int lua_absindex(lua_State* L, int idx)
	{
		if (idx > LUA_REGISTRYINDEX && idx < 0)
			return lua_gettop(L) + idx + 1;
		else
			return idx;
	}

	void lua_len(lua_State* L, int i);
	lua_Integer luaL_len(lua_State* L, int i);
#endif

	/** casts the variant to its specific type before pushing it to the stack
	@relates LuaType */
	void RMLUILUA_API PushVariant(lua_State* L, const Variant* var);

	/** Populate the variant based on the Lua value at the given index */
	void RMLUILUA_API GetVariant(lua_State* L, int index, Variant* variant);

	// Converts index from 0-based to 1-based before pushing it to the stack
	void RMLUILUA_API PushIndex(lua_State* L, int index);

	// Returns 0-based index after retrieving from stack and converting from 1-based
	// The index parameter refers to the position on the stack and is not affected
	// by the conversion
	int RMLUILUA_API GetIndex(lua_State* L, int index);

	// Helper function, so that the types don't have to define individual functions themselves
	//  to fill the Elements.As table
	template <typename ToType>
	int CastFromElementTo(lua_State* L)
	{
		Element* ele = LuaType<Element>::check(L, 1);
		RMLUI_CHECK_OBJ(ele);
		LuaType<ToType>::push(L, (ToType*)ele, false);
		return 1;
	}

	// Adds to the Element.As table the name of the type, and the function to use to cast
	template <typename T>
	void AddTypeToElementAsTable(lua_State* L)
	{
		int top = lua_gettop(L);
		lua_getglobal(L, "Element");
		lua_getfield(L, -1, "As");
		if (!lua_isnoneornil(L, -1))
		{
			lua_pushcfunction(L, CastFromElementTo<T>);
			lua_setfield(L, -2, GetTClassName<T>());
		}
		lua_settop(L, top); // pop "As" and "Element"
	}
} // namespace Lua
} // namespace Rml
#endif
