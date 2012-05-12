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
 
#ifndef ROCKETCONTROLSLUADATASOURCE_H
#define ROCKETCONTROLSLUADATASOURCE_H

/*
    This defines the DataSource type in the Lua global namespace
    
    //methods
    noreturn DataSource:NotifyRowAdd(string tablename, int firstadded, int numadded)
    noreturn DataSource:NotifyRemove(string tablename, int firstremoved, int numremoved)
    noreturn DataSource:NotifyRowChange(string tablename, int firstchanged, int numchanged)
    noreturn DataSource:NotifyRowChange(string tablename) --the same as above, but for all rows
    
    when you get an instance of this, you MUST set two functions:
    DataSource.GetNumRows = function(tablename) ....... end --returns an int
    DataSource.GetRow = function(tablename,index,columns) ......end
    --where columns is a numerically indexed table of strings, and the function needs to
    --return a table of numerically indexed strings
*/

#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Core/Lua/lua.hpp>
#include "LuaDataSource.h"

using Rocket::Core::Lua::LuaType;

namespace Rocket {
namespace Controls {
namespace Lua {
typedef LuaDataSource DataSource;

int DataSourceNotifyRowAdd(lua_State* L, DataSource* obj);
int DataSourceNotifyRowRemove(lua_State* L, DataSource* obj);
int DataSourceNotifyRowChange(lua_State* L, DataSource* obj);

int DataSourceSetAttrGetNumRows(lua_State* L);
int DataSourceSetAttrGetRow(lua_State* L);

Rocket::Core::Lua::RegType<DataSource> DataSourceMethods[];
luaL_reg DataSourceGetters[];
luaL_reg DataSourceSetters[];



}
}
}

LUATYPEDECLARE(Rocket::Controls::Lua::DataSource)
#endif
