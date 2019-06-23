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
 
#ifndef RMLUICONTROLSLUAAS_H
#define RMLUICONTROLSLUAAS_H
/*
    These are helper functions to fill up the Element.As table with types that are able to be casted
*/

#include <RmlUi/Core/Lua/LuaType.h>
#include <RmlUi/Core/Lua/lua.hpp>
#include <RmlUi/Core/Element.h>

namespace Rml {
namespace Controls {
namespace Lua {

//Helper function for the controls, so that the types don't have to define individual functions themselves
// to fill the Elements.As table
template<typename ToType>
int CastFromElementTo(lua_State* L)
{
    Rml::Core::Element* ele = Rml::Core::Lua::LuaType<Rml::Core::Element>::check(L,1);
    LUACHECKOBJ(ele);
    Rml::Core::Lua::LuaType<ToType>::push(L,(ToType*)ele,false);
    return 1;
}

//Adds to the Element.As table the name of the type, and the function to use to cast
template<typename T>
void AddCastFunctionToElementAsTable(lua_State* L)
{
    int top = lua_gettop(L);
    lua_getglobal(L,"Element");
    lua_getfield(L,-1,"As");
    if(!lua_isnoneornil(L,-1))
    {
        lua_pushcfunction(L,Rml::Controls::Lua::CastFromElementTo<T>);
        lua_setfield(L,-2,Rml::Core::Lua::GetTClassName<T>());
    }
    lua_settop(L,top); //pop "As" and "Element"
}

}
}
}

#endif
