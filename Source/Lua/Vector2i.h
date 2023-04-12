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

#ifndef RMLUI_LUA_VECTOR2I_H
#define RMLUI_LUA_VECTOR2I_H

#include <RmlUi/Core/Types.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {
template <>
void ExtraInit<Vector2i>(lua_State* L, int metatable_index);
int Vector2inew(lua_State* L);
int Vector2i__mul(lua_State* L);
int Vector2i__div(lua_State* L);
int Vector2i__add(lua_State* L);
int Vector2i__sub(lua_State* L);
int Vector2i__eq(lua_State* L);

// getters
int Vector2iGetAttrx(lua_State* L);
int Vector2iGetAttry(lua_State* L);
int Vector2iGetAttrmagnitude(lua_State* L);

// setters
int Vector2iSetAttrx(lua_State* L);
int Vector2iSetAttry(lua_State* L);

extern RegType<Vector2i> Vector2iMethods[];
extern luaL_Reg Vector2iGetters[];
extern luaL_Reg Vector2iSetters[];

RMLUI_LUATYPE_DECLARE(Vector2i)
} // namespace Lua
} // namespace Rml
#endif
