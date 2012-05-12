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
 
#ifndef ROCKETCONTROLSLUAELEMENTFORM_H
#define ROCKETCONTROLSLUAELEMENTFORM_H
/*
    This defines the ElementForm type in the Lua global namespace

    methods:
    ElementForm:Submit(string name,string value)

    for everything else, see the documentation for "Element"
*/

#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Controls/ElementForm.h>

using Rocket::Core::Lua::LuaType;
//this will be used to "inherit" from Element
template<> void Rocket::Core::Lua::ExtraInit<Rocket::Controls::ElementForm>(lua_State* L, int metatable_index);

namespace Rocket {
namespace Controls {
namespace Lua {


//method
int ElementFormSubmit(lua_State* L, ElementForm* obj);

Rocket::Core::Lua::RegType<ElementForm> ElementFormMethods[];
luaL_reg ElementFormGetters[];
luaL_reg ElementFormSetters[];



}
}
}
LUATYPEDECLARE(Rocket::Controls::ElementForm)
#endif
