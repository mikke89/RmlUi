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
 
#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Core/Lua/lua.hpp>

/*
    Declares "Log" in the global Lua namespace.

    //method, not called from a "Log" object, just from the Log table
    Log.Message(logtype type, string message)

    where logtype is defined in Log.logtype, and can be:
    logtype.always
    logtype.error
    logtype.warning
    logtype.info
    logtype.debug
    and they have the same value as the C++ Log::Type of the same name
*/

namespace Rocket {
namespace Core {
namespace Lua {

template<> void LuaType<Log>::extra_init(lua_State* L, int metatable_index);
int LogMessage(lua_State* L);

RegType<Log> LogMethods[];
luaL_reg LogGetters[];
luaL_reg LogSetters[];

/*
template<> const char* GetTClassName<Log>();
template<> RegType<Log>* GetMethodTable<Log>();
template<> luaL_reg* GetAttrTable<Log>();
template<> luaL_reg* SetAttrTable<Log>();
*/
}
}
}