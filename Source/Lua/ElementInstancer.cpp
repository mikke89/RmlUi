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

#include "ElementInstancer.h"

namespace Rml {
namespace Lua {
template <>
void ExtraInit<ElementInstancer>(lua_State* L, int metatable_index)
{
	lua_pushcfunction(L, ElementInstancernew);
	lua_setfield(L, metatable_index - 1, "new");
}

// method
int ElementInstancernew(lua_State* L)
{
	LuaElementInstancer* lei = new LuaElementInstancer(L);
	LuaType<ElementInstancer>::push(L, lei, true);
	return 1;
}

// setter
int ElementInstancerSetAttrInstanceElement(lua_State* L)
{
	LuaElementInstancer* lei = (LuaElementInstancer*)LuaType<ElementInstancer>::check(L, 1);
	RMLUI_CHECK_OBJ(lei);

	if (lua_type(L, 2) != LUA_TFUNCTION)
	{
		Log::Message(Log::LT_ERROR, "The argument to ElementInstancer.InstanceElement must be a function. You passed in a %s.", luaL_typename(L, 2));
		return 0;
	}
	lei->PushFunctionsTable(L); // top of the stack is now ELEMENTINSTANCERFUNCTIONS table
	lua_pushvalue(L, 2);        // copy of the function
	lei->ref_InstanceElement = luaL_ref(L, -2);
	lua_pop(L, 1);              // pop the ELEMENTINSTANCERFUNCTIONS table
	return 0;
}

RegType<ElementInstancer> ElementInstancerMethods[] = {
	{nullptr, nullptr},
};

luaL_Reg ElementInstancerGetters[] = {
	{nullptr, nullptr},
};

luaL_Reg ElementInstancerSetters[] = {
	RMLUI_LUASETTER(ElementInstancer, InstanceElement),
	{nullptr, nullptr},
};

RMLUI_LUATYPE_DEFINE(ElementInstancer)
} // namespace Lua
} // namespace Rml
