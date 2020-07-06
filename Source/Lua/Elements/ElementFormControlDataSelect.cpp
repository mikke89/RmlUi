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
 
#include "ElementFormControlDataSelect.h"
#include <RmlUi/Core/Elements/ElementFormControlSelect.h>
#include "ElementFormControlSelect.h"
#include <RmlUi/Lua/Utilities.h>

namespace Rml {
namespace Lua {

//method
int ElementFormControlDataSelectSetDataSource(lua_State* L, ElementFormControlDataSelect* obj)
{
    const char* source = luaL_checkstring(L,1);
    obj->SetDataSource(source);
    return 0;
}

RegType<ElementFormControlDataSelect> ElementFormControlDataSelectMethods[] =
{
    RMLUI_LUAMETHOD(ElementFormControlDataSelect,SetDataSource)
    { nullptr, nullptr },
};

luaL_Reg ElementFormControlDataSelectGetters[] =
{
    { nullptr, nullptr },
};

luaL_Reg ElementFormControlDataSelectSetters[] =
{
    { nullptr, nullptr },
};


//inherits from ElementFormControl which inherits from Element
template<> void ExtraInit<ElementFormControlDataSelect>(lua_State* L, int metatable_index)
{
    //do whatever ElementFormControlSelect did as far as inheritance
    ExtraInit<ElementFormControlSelect>(L,metatable_index);
    //then inherit from ElementFromControlSelect
    LuaType<ElementFormControlSelect>::_regfunctions(L,metatable_index,metatable_index-1);
    AddTypeToElementAsTable<ElementFormControlDataSelect>(L);
}

RMLUI_LUATYPE_DEFINE(ElementFormControlDataSelect)
} // namespace Lua
} // namespace Rml
