/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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
 
#ifndef ROCKETCORELUAVECTOR2I_H
#define ROCKETCORELUAVECTOR2I_H
/*
    Declares Vector2i in the Lua global namespace. It implements the below (examples using Lua syntax) :

    Vector2i(int,int) creates a new Vector2i, and gets deleted when Lua garbage collects
    
    everything after this will assume that you have a local variable named 'vect', declared something similar to
    local vect = Vector2i.new(50,90)
    operators (the types that it can operate on are on the right):
    vect * int
    vect / int
    vect + Vector2i
    vect - Vector2i
    vect == Vector2i

    no methods

    get and set attributes:
    vect.x
    vect.y
*/
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Core/Types.h>

using Rocket::Core::Vector2i;
namespace Rocket {
namespace Core {
namespace Lua {
template<> void LuaType<Vector2i>::extra_init(lua_State* L, int metatable_index);
int Vector2inew(lua_State* L);
int Vector2i__mul(lua_State* L);
int Vector2i__div(lua_State* L);
int Vector2i__add(lua_State* L);
int Vector2i__sub(lua_State* L);
int Vector2i__eq(lua_State* L);

//getters
int Vector2iGetAttrx(lua_State*L);
int Vector2iGetAttry(lua_State*L);
int Vector2iGetAttrmagnitude(lua_State*L);

//setters
int Vector2iSetAttrx(lua_State*L);
int Vector2iSetAttry(lua_State*L);


RegType<Vector2i> Vector2iMethods[];
luaL_reg Vector2iGetters[];
luaL_reg Vector2iSetters[];

/*
template<> const char* GetTClassName<Vector2i>() { return "Vector2i"; }
template<> RegType<Vector2i>* GetMethodTable<Vector2i>() { return Vector2iMethods; }
template<> luaL_reg* GetAttrTable<Vector2i>() { return Vector2iGetters; }
template<> luaL_reg* SetAttrTable<Vector2i>() { return Vector2iSetters; }
*/
}
}
}
#endif

