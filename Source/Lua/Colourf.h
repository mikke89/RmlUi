/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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
 
#ifndef RMLUI_LUA_COLOURF_H
#define RMLUI_LUA_COLOURF_H

#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>
#include <RmlUi/Core/Types.h>

namespace Rml {
namespace Lua {
template<> void ExtraInit<Colourf>(lua_State* L, int metatable_index);
//metamethods
int Colourfnew(lua_State* L);
int Colourf__eq(lua_State* L);

//getters
int ColourfGetAttrred(lua_State* L);
int ColourfGetAttrgreen(lua_State* L);
int ColourfGetAttrblue(lua_State* L);
int ColourfGetAttralpha(lua_State* L);
int ColourfGetAttrrgba(lua_State* L);

//setters
int ColourfSetAttrred(lua_State* L);
int ColourfSetAttrgreen(lua_State* L);
int ColourfSetAttrblue(lua_State* L);
int ColourfSetAttralpha(lua_State* L);
int ColourfSetAttrrgba(lua_State* L);

extern RegType<Colourf> ColourfMethods[];
extern luaL_Reg ColourfGetters[];
extern luaL_Reg ColourfSetters[];

RMLUI_LUATYPE_DECLARE(Colourf)
} // namespace Lua
} // namespace Rml
#endif
