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
 
#include "DataFormatter.h"
#include <RmlUi/Core/Log.h>


namespace Rml {
namespace Lua {
//method
int DataFormatternew(lua_State* L)
{
    DataFormatter* df = nullptr;
    int ref = LUA_NOREF;
    int top = lua_gettop(L);
    if(top == 0)
        df = new DataFormatter();
    else if (top > 0) //at least one element means at least a name
    {
        if(top > 1) //if a function as well
        {
            if(lua_type(L,2) != LUA_TFUNCTION)
            {
                Log::Message(Log::LT_ERROR, "Lua: In DataFormatter.new, the second argument MUST be a function (or not exist). You passed in a %s.", lua_typename(L,lua_type(L,2)));
            }
            else //if it is a function
            {
                LuaDataFormatter::PushDataFormatterFunctionTable(L);
                lua_pushvalue(L,2); //re-push the function so it is on the top of the stack
                ref = luaL_ref(L,-2);
                lua_pop(L,1); //pop the function table
            }
        }
        df = new DataFormatter(luaL_checkstring(L,1));
        df->ref_FormatData = ref; //will either be valid or LUA_NOREF
    }
    LuaType<DataFormatter>::push(L,df,true);
    return 1;
}

//setter
int DataFormatterSetAttrFormatData(lua_State* L)
{
    DataFormatter* obj = LuaType<DataFormatter>::check(L,1);
    RMLUI_CHECK_OBJ(obj);
    int ref = LUA_NOREF;
    if(lua_type(L,2) != LUA_TFUNCTION)
    {
        Log::Message(Log::LT_ERROR, "Lua: Setting DataFormatter.FormatData, the value must be a function. You passed in a %s.", lua_typename(L,lua_type(L,2)));
    }
    else //if it is a function
    {
        LuaDataFormatter::PushDataFormatterFunctionTable(L);
        lua_pushvalue(L,2); //re-push the function so it is on the top of the stack
        ref = luaL_ref(L,-2);
        lua_pop(L,1); //pop the function table
    }
    obj->ref_FormatData = ref;
    return 0;
}

RegType<DataFormatter> DataFormatterMethods[] =
{
    { nullptr, nullptr },
};

luaL_Reg DataFormatterGetters[] =
{
    { nullptr, nullptr },
};

luaL_Reg DataFormatterSetters[] =
{
    RMLUI_LUASETTER(DataFormatter,FormatData)
    { nullptr, nullptr },
};


template<> void ExtraInit<DataFormatter>(lua_State* L, int metatable_index)
{
    lua_pushcfunction(L,DataFormatternew);
    lua_setfield(L,metatable_index-1,"new");
    return;
}
RMLUI_LUATYPE_DEFINE(DataFormatter)
} // namespace Lua
} // namespace Rml
