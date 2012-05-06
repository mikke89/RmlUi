#ifndef ROCKETCORELUADATASOURCE_H
#define ROCKETCORELUADATASOURCE_H

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

namespace Rocket {
namespace Core {
namespace Lua {
typedef LuaDataSource DataSource;

int DataSourceNotifyRowAdd(lua_State* L, DataSource* obj);
int DataSourceNotifyRowRemove(lua_State* L, DataSource* obj);
int DataSourceNotifyRowChange(lua_State* L, DataSource* obj);

int DataSourceSetAttrGetNumRows(lua_State* L);
int DataSourceSetAttrGetRow(lua_State* L);

RegType<DataSource> DataSourceMethods[];
luaL_reg DataSourceGetters[];
luaL_reg DataSourceSetters[];

}
}
}
#endif
