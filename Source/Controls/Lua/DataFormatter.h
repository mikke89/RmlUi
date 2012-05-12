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
 
#ifndef ROCKETCONTROLSLUADATAFORMATTER_H
#define ROCKETCONTROLSLUADATAFORMATTER_H
/*
    This defines the DataFormatter type in the Lua global namespace

    The usage is to create a new DataFormatter, and set the FormatData variable on it to a function, where you return a rml string that will
    be the formatted data

    //method
    --this method is NOT called from any particular instance, use it as from the type
    DataFormatter DataFormatter.new([string name[, function FormatData]]) --both are optional, but if you pass in a function, you must also pass in a name first

    //setter
    --this is called from a specific instance (one returned from DataFormatter.new)
    DataFormatter.FormatData = function( {key=int,value=string} ) --where indexes are sequential, like an array of strings
*/

#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Core/Lua/LuaType.h>
#include "LuaDataFormatter.h"

using Rocket::Core::Lua::LuaType;
namespace Rocket {
namespace Controls {
namespace Lua {
typedef LuaDataFormatter DataFormatter;
//method
int DataFormatternew(lua_State* L);

//setter
int DataFormatterSetAttrFormatData(lua_State* L);

Rocket::Core::Lua::RegType<DataFormatter> DataFormatterMethods[];
luaL_reg DataFormatterGetters[];
luaL_reg DataFormatterSetters[];
}
}
}
//for DataFormatter.new
template<> void Rocket::Core::Lua::ExtraInit<Rocket::Controls::Lua::DataFormatter>(lua_State* L, int metatable_index);
LUATYPEDECLARE(Rocket::Controls::Lua::DataFormatter)
#endif
