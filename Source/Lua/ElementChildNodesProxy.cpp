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

#include "ElementChildNodesProxy.h"
#include "Element.h"
#include "Pairs.h"

namespace Rml {
namespace Lua {

template <>
void ExtraInit<ElementChildNodesProxy>(lua_State* L, int metatable_index)
{
	lua_pushcfunction(L, ElementChildNodesProxy__index);
	lua_setfield(L, metatable_index, "__index");
	lua_pushcfunction(L, ElementChildNodesProxy__pairs);
	lua_setfield(L, metatable_index, "__pairs");
	lua_pushcfunction(L, ElementChildNodesProxy__len);
	lua_setfield(L, metatable_index, "__len");
}

int ElementChildNodesProxy__index(lua_State* L)
{
	/*the table obj and the missing key are currently on the stack(index 1 & 2) as defined by the Lua language*/
	int keytype = lua_type(L, 2);
	if (keytype == LUA_TNUMBER) // only valid key types
	{
		ElementChildNodesProxy* obj = LuaType<ElementChildNodesProxy>::check(L, 1);
		RMLUI_CHECK_OBJ(obj);
		int key = (int)luaL_checkinteger(L, 2);
		Element* child = obj->owner->GetChild(key - 1);
		LuaType<Element>::push(L, child, false);
		return 1;
	}
	else
		return LuaType<ElementChildNodesProxy>::index(L);
}

int ElementChildNodesProxy__pairs(lua_State* L)
{
	return MakeIntPairs(L);
}

int ElementChildNodesProxy__len(lua_State* L)
{
	ElementChildNodesProxy* obj = LuaType<ElementChildNodesProxy>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	lua_pushinteger(L, obj->owner->GetNumChildren());
	;
	return 1;
}

RegType<ElementChildNodesProxy> ElementChildNodesProxyMethods[] = {
	{nullptr, nullptr},
};
luaL_Reg ElementChildNodesProxyGetters[] = {
	{nullptr, nullptr},
};
luaL_Reg ElementChildNodesProxySetters[] = {
	{nullptr, nullptr},
};

RMLUI_LUATYPE_DEFINE(ElementChildNodesProxy)
} // namespace Lua
} // namespace Rml
