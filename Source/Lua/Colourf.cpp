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

#include "Colourf.h"

namespace Rml {
namespace Lua {

template <>
void ExtraInit<Colourf>(lua_State* L, int metatable_index)
{
	lua_pushcfunction(L, Colourfnew);
	lua_setfield(L, metatable_index - 1, "new");

	lua_pushcfunction(L, Colourf__eq);
	lua_setfield(L, metatable_index, "__eq");

	return;
}

// metamethods
int Colourfnew(lua_State* L)
{
	float red = (float)luaL_checknumber(L, 1);
	float green = (float)luaL_checknumber(L, 2);
	float blue = (float)luaL_checknumber(L, 3);
	float alpha = (float)luaL_checknumber(L, 4);

	Colourf* col = new Colourf(red, green, blue, alpha);

	LuaType<Colourf>::push(L, col, true);
	return 1;
}

int Colourf__eq(lua_State* L)
{
	Colourf* lhs = LuaType<Colourf>::check(L, 1);
	RMLUI_CHECK_OBJ(lhs);
	Colourf* rhs = LuaType<Colourf>::check(L, 2);
	RMLUI_CHECK_OBJ(rhs);

	lua_pushboolean(L, (*lhs) == (*rhs) ? 1 : 0);
	return 1;
}

// getters
int ColourfGetAttrred(lua_State* L)
{
	Colourf* obj = LuaType<Colourf>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	lua_pushnumber(L, obj->red);
	return 1;
}

int ColourfGetAttrgreen(lua_State* L)
{
	Colourf* obj = LuaType<Colourf>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	lua_pushnumber(L, obj->green);
	return 1;
}

int ColourfGetAttrblue(lua_State* L)
{
	Colourf* obj = LuaType<Colourf>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	lua_pushnumber(L, obj->blue);
	return 1;
}

int ColourfGetAttralpha(lua_State* L)
{
	Colourf* obj = LuaType<Colourf>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	lua_pushnumber(L, obj->alpha);
	return 1;
}

int ColourfGetAttrrgba(lua_State* L)
{
	Colourf* obj = LuaType<Colourf>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	lua_pushnumber(L, obj->red);
	lua_pushnumber(L, obj->green);
	lua_pushnumber(L, obj->blue);
	lua_pushnumber(L, obj->alpha);
	return 4;
}

// setters
int ColourfSetAttrred(lua_State* L)
{
	Colourf* obj = LuaType<Colourf>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	float red = (float)luaL_checknumber(L, 2);
	obj->red = red;
	return 0;
}

int ColourfSetAttrgreen(lua_State* L)
{
	Colourf* obj = LuaType<Colourf>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	float green = (float)luaL_checknumber(L, 2);
	obj->green = green;
	return 0;
}

int ColourfSetAttrblue(lua_State* L)
{
	Colourf* obj = LuaType<Colourf>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	float blue = (float)luaL_checknumber(L, 2);
	obj->blue = blue;
	return 0;
}

int ColourfSetAttralpha(lua_State* L)
{
	Colourf* obj = LuaType<Colourf>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	float alpha = (float)luaL_checknumber(L, 2);
	obj->alpha = alpha;
	return 0;
}

int ColourfSetAttrrgba(lua_State* L)
{
	Colourf* obj = nullptr;
	int top = lua_gettop(L);
	// each of the items are optional.
	if (top > 0)
	{
		obj = LuaType<Colourf>::check(L, 1);
		RMLUI_CHECK_OBJ(obj);
		if (top > 1)
		{
			if (top > 2)
			{
				if (top > 3)
					obj->alpha = (float)luaL_checknumber(L, 4);
				obj->blue = (float)luaL_checknumber(L, 3);
			}
			obj->green = (float)luaL_checknumber(L, 2);
		}
		obj->red = (float)luaL_checknumber(L, 1);
	}
	return 0;
}

RegType<Colourf> ColourfMethods[] = {
	{nullptr, nullptr},
};

luaL_Reg ColourfGetters[] = {
	RMLUI_LUAGETTER(Colourf, red),
	RMLUI_LUAGETTER(Colourf, green),
	RMLUI_LUAGETTER(Colourf, blue),
	RMLUI_LUAGETTER(Colourf, alpha),
	RMLUI_LUAGETTER(Colourf, rgba),
	{nullptr, nullptr},
};

luaL_Reg ColourfSetters[] = {
	RMLUI_LUASETTER(Colourf, red),
	RMLUI_LUASETTER(Colourf, green),
	RMLUI_LUASETTER(Colourf, blue),
	RMLUI_LUASETTER(Colourf, alpha),
	RMLUI_LUASETTER(Colourf, rgba),
	{nullptr, nullptr},
};

RMLUI_LUATYPE_DEFINE(Colourf)

} // namespace Lua
} // namespace Rml
