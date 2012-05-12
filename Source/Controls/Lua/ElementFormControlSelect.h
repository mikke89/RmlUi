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
 
#ifndef ROCKETCONTROLSLUAELEMENTFORMCONTROLSELECT_H
#define ROCKETCONTROLSLUAELEMENTFORMCONTROLSELECT_H

/*
    This defines the ElementFormControlSelect type in the Lua global namespace, for this documentation will be
    named EFCSelect

    It has one extra method than the Python api, which is GetOption

    //methods
    int EFCSelect:Add(string rml, string value, [int before]) --where 'before' is optional, and is an index
    noreturn EFCSelect:Remove(int index)
    {"element"=Element,"value"=string} EFCSelect:GetOption(int index) --this is a more efficient way to get an option if you know the index beforehand

    //getters
    {[int index]={"element"=Element,"value"=string}} EFCSelect.options --used to access options as a Lua table. Comparatively expensive operation, use GetOption when 
                                                                       --you only need one option and you know the index
    int EFCSelect.selection

    //setter
    EFCSelect.selection = int
*/
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Controls/ElementFormControlSelect.h>

using Rocket::Core::Lua::LuaType;
//inherits from ElementFormControl which inherits from Element
template<> void Rocket::Core::Lua::ExtraInit<Rocket::Controls::ElementFormControlSelect>(lua_State* L, int metatable_index);
namespace Rocket {
namespace Controls {
namespace Lua {

//methods
int ElementFormControlSelectAdd(lua_State* L, ElementFormControlSelect* obj);
int ElementFormControlSelectRemove(lua_State* L, ElementFormControlSelect* obj);
int ElementFormControlSelectGetOption(lua_State* L, ElementFormControlSelect* obj);

//getters
int ElementFormControlSelectGetAttroptions(lua_State* L);
int ElementFormControlSelectGetAttrselection(lua_State* L);

//setter
int ElementFormControlSelectSetAttrselection(lua_State* L);

Rocket::Core::Lua::RegType<ElementFormControlSelect> ElementFormControlSelectMethods[];
luaL_reg ElementFormControlSelectGetters[];
luaL_reg ElementFormControlSelectSetters[];

}
}
}
LUATYPEDECLARE(Rocket::Controls::ElementFormControlSelect)
#endif
