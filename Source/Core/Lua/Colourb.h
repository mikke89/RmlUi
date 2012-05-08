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
 
#ifndef ROCKETCORELUACOLOURB_H
#define ROCKETCORELUACOLOURB_H
/*
    Declares Colourb in the Lua global namespace. It implements the below (examples using Lua syntax) :

    Colourb.new(int red,int green,int blue,int alpha) creates a new Colourf (values must be bounded between 0 and 255 inclusive), 
    and gets deleted when Lua garbage collects
    
    everything after this will assume that you have a local variable named 'col', declared something similar to
    local col = Colourb.new(0,15,3,255)

    operators (the types that it can operate on are on the right):
    col == Colourb
    col + Colourb
    col * float

    no methods

    get and set attributes:
    col.red
    col.green
    col.blue
    col.alpha

    get attributes:
    local red,green,blue,alpha = col.rgba    
*/

#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Core/Types.h>

using Rocket::Core::Colourb;
namespace Rocket {
namespace Core {
namespace Lua {
template<> void LuaType<Colourb>::extra_init(lua_State* L, int metatable_index);
int Colourbnew(lua_State* L);
int Colourb__eq(lua_State* L);
int Colourb__add(lua_State* L);
int Colourb__mul(lua_State* L);


//getters
int ColourbGetAttrred(lua_State* L);
int ColourbGetAttrgreen(lua_State* L);
int ColourbGetAttrblue(lua_State* L);
int ColourbGetAttralpha(lua_State* L);
int ColourbGetAttrrgba(lua_State* L);

//setters
int ColourbSetAttrred(lua_State* L);
int ColourbSetAttrgreen(lua_State* L);
int ColourbSetAttrblue(lua_State* L);
int ColourbSetAttralpha(lua_State* L);

RegType<Colourb> ColourbMethods[];
luaL_reg ColourbGetters[];
luaL_reg ColourbSetters[];

/*
template<> const char* GetTClassName<Colourb>() { return "Colourb"; }
template<> RegType<Colourb>* GetMethodTable<Colourb>() { return ColourbMethods; }
template<> luaL_reg* GetAttrTable<Colourb>() { return ColourbGetters; }
template<> luaL_reg* SetAttrTable<Colourb>() { return ColourbSetters; }
*/
}
}
}
#endif