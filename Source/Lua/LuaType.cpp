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
 
#include <RmlUi/Lua/LuaType.h>
#include <RmlUi/Lua/Utilities.h>
#include <RmlUi/Core/Types.h>

namespace Rml {
namespace Lua {


int LuaTypeImpl::index(lua_State* L, const char* class_name)
{
    /*the table obj and the missing key are currently on the stack(index 1 & 2) as defined by the Lua language*/
    lua_getglobal(L, class_name); //stack pos [3] (fairly important, just refered to as [3])
                                  // string form of the key.
    const char* key = luaL_checkstring(L, 2);
    if (lua_istable(L, -1))  //[-1 = 3]
    {
        lua_pushvalue(L, 2); //[2] = key, [4] = copy of key
        lua_rawget(L, -2); //[-2 = 3] -> pop top and push the return value to top [4]
                           //If the key were looking for is not in the table, retrieve its' metatables' index value.
        if (lua_isnil(L, -1)) //[-1 = 4] is value from rawget above
        {
            //try __getters
            lua_pop(L, 1); //remove top item (nil) from the stack
            lua_pushstring(L, "__getters");
            lua_rawget(L, -2); //[-2 = 3], <ClassName>._getters -> result to [4]
            lua_pushvalue(L, 2); //[2 = key] -> copy to [5]
            lua_rawget(L, -2); //[-2 = __getters] -> __getters[key], result to [5]
            if (lua_type(L, -1) == LUA_TFUNCTION) //[-1 = 5]
            {
                lua_pushvalue(L, 1); //push the userdata to the stack [6]
                lua_call(L, 1, 1); //remove one, result is at [6]
            }
            else
            {
                lua_settop(L, 4); //forget everything we did above
                lua_getmetatable(L, -2); //[-2 = 3] -> metatable from <ClassName> to top [5]
                if (lua_istable(L, -1)) //[-1 = 5] = the result of the above
                {
                    lua_getfield(L, -1, "__index"); //[-1 = 5] = check the __index metamethod for the metatable-> push result to [6]
                    if (lua_isfunction(L, -1)) //[-1 = 6] = __index metamethod
                    {
                        lua_pushvalue(L, 1); //[1] = object -> [7] = object
                        lua_pushvalue(L, 2); //[2] = key -> [8] = key
                        lua_call(L, 2, 1); //call function at top of stack (__index) -> pop top 2 as args; [7] = return value
                    }
                    else if (lua_istable(L, -1))
                        lua_getfield(L, -1, key); //shorthand version of above -> [7] = return value
                    else
                        lua_pushnil(L); //[7] = nil
                }
                else
                    lua_pushnil(L); //[6] = nil
            }
        }
        else if (lua_istable(L, -1))//[-1 = 4] is value from rawget [3]
        {
            lua_pushvalue(L, 2); //[2] = key, [5] = key
            lua_rawget(L, -2); //[-2 = 3] = table of <ClassName> -> pop top and push the return value to top [5]
        }
    }
    else
        lua_pushnil(L); //[4] = nil

    lua_insert(L, 1); //top element to position 1 -> [1] = top element as calculated in the earlier rest of the function
    lua_settop(L, 1); // -> [1 = -1], removes the other elements
    return 1;
}

int LuaTypeImpl::newindex(lua_State* L, const char* class_name)
{
    //[1] = obj, [2] = key, [3] = value
    //look for it in __setters
    lua_getglobal(L, class_name); //[4] = this table
    lua_pushstring(L, "__setters"); //[5]
    lua_rawget(L, -2); //[-2 = 4] -> <ClassName>.__setters to [5]
    lua_pushvalue(L, 2); //[2 = key] -> [6] = copy of key
    lua_rawget(L, -2); //[-2 = __setters] -> __setters[key] to [6]
    if (lua_type(L, -1) == LUA_TFUNCTION)
    {
        lua_pushvalue(L, 1); //userdata at [7]
        lua_pushvalue(L, 3); //[8] = copy of [3]
        lua_call(L, 2, 0); //call function, pop 2 off push 0 on
    }
    else
        lua_pop(L, 1); //not a setter function.
    lua_pop(L, 2); //pop __setters and the <Classname> table
    return 0;
}


} // namespace Lua
} // namespace Rml
