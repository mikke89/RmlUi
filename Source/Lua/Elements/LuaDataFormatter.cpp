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
 
#include "LuaDataFormatter.h"
#include <RmlUi/Lua/Interpreter.h>
#include <RmlUi/Core/Log.h>

namespace Rml {
namespace Lua {


LuaDataFormatter::LuaDataFormatter(const String& name) : DataFormatter(name), ref_FormatData(LUA_NOREF)
{}

LuaDataFormatter::~LuaDataFormatter()
{}

void LuaDataFormatter::FormatData(String& formatted_data, const StringList& raw_data)
{
    if(ref_FormatData == LUA_NOREF || ref_FormatData == LUA_REFNIL)
    {
        Log::Message(Log::LT_ERROR, "In LuaDataFormatter: There is no value assigned to the \"FormatData\" variable.");
        return;
    }
    lua_State* L = Interpreter::GetLuaState();
    int top = lua_gettop(L);
    PushDataFormatterFunctionTable(L); // push the table where the function resides
    lua_rawgeti(L,-1,ref_FormatData); //push the function
    if(lua_type(L,-1) != LUA_TFUNCTION)
    {
        Log::Message(Log::LT_ERROR, "In LuaDataFormatter: The value for the FormatData variable must be a function. You passed in a %s.", lua_typename(L,lua_type(L,-1)));
        lua_settop(L,top);
        return;
    }
    lua_newtable(L); //to hold raw_data
    int tbl = lua_gettop(L);
    for(unsigned int i = 0; i < raw_data.size(); i++)
    {
        lua_pushstring(L,raw_data[i].c_str());
        lua_rawseti(L,tbl,i);
    }
    Interpreter::ExecuteCall(1,1); //1 parameter (the table), 1 result (a string)

    //top of the stack should be the return value
    if(lua_type(L,-1) != LUA_TSTRING)
    {
        Log::Message(Log::LT_ERROR, "In LuaDataFormatter: the return value of FormatData must be a string. You returned a %s.", lua_typename(L,lua_type(L,-1)));
        lua_settop(L,top);
        return;
    }
    formatted_data = String(lua_tostring(L,-1));
    lua_settop(L,top);
}

void LuaDataFormatter::PushDataFormatterFunctionTable(lua_State* L)
{
    lua_getglobal(L,"LUADATAFORMATTERFUNCTIONS");
    if(lua_isnoneornil(L,-1))
    {
        lua_newtable(L);
        lua_setglobal(L,"LUADATAFORMATTERFUNCTIONS");
        lua_pop(L,1); //pop the unsucessful getglobal
        lua_getglobal(L,"LUADATAFORMATTERFUNCTIONS");
    }
}

} // namespace Lua
} // namespace Rml
