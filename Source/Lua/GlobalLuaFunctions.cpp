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


#include "GlobalLuaFunctions.h"
#include <RmlUi/Lua/LuaType.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/Utilities.h>
#include <RmlUi/Core/Types.h>
#include <RmlUi/Core/Log.h>

namespace Rml {
namespace Lua {
/*
Below here are global functions and their helper functions that help overwrite the Lua global functions
*/

//Based off of the luaB_print function from Lua's lbaselib.c
int LuaPrint(lua_State* L)
{
    int n = lua_gettop(L);  /* number of arguments */
    int i;
    lua_getglobal(L, "tostring");
    StringList string_list = StringList();
    String output = "";
    for (i=1; i<=n; i++) 
    {
        const char *s;
        lua_pushvalue(L, -1);  /* function to be called */
        lua_pushvalue(L, i);   /* value to print */
        lua_call(L, 1, 1);
        s = lua_tostring(L, -1);  /* get result */
        if (s == nullptr)
            return luaL_error(L, "'tostring' must return a string to 'print'");
        if (i>1) 
            output += "\t";
        output += String(s);
        lua_pop(L, 1);  /* pop result */
    }
    output += "\n";
    Log::Message(Log::LT_INFO, "%s", output.c_str());
    return 0;
}

void OverrideLuaGlobalFunctions(lua_State* L)
{
    lua_getglobal(L,"_G");

    lua_pushcfunction(L,LuaPrint);
    lua_setfield(L,-2,"print");

    lua_pop(L,1); //pop _G
}

} // namespace Lua
} // namespace Rml
