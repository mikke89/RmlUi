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
 
#ifndef ROCKETCORELUACOLOURF_H
#define ROCKETCORELUACOLOURF_H
/*
    Declares Colourf in the Lua global namespace. It implements the below (examples using Lua syntax) :

    Colourf.new(float red,float green,float blue,float alpha) creates a new Colourf (values must be bounded between 0 and 1 inclusive), 
    and gets deleted when Lua garbage collects
    
    everything after this will assume that you have a local variable named 'col', declared something similar to
    local col = Colourf.new(0.1,0.5,0.25,1.0)

    operators (the types that it can operate on are on the right):
    col == Colourf

    no methods

    get and set attributes:
    col.red
    col.green
    col.blue
    col.alpha

    get attributes:
    local red,green,blue,alpha = col.rgba    
*/

#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Core/Types.h>

using Rocket::Core::Colourf;
namespace Rocket {
namespace Core {
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

RegType<Colourf> ColourfMethods[];
luaL_reg ColourfGetters[];
luaL_reg ColourfSetters[];

LUATYPEDECLARE(Colourf)
}
}
}
#endif

