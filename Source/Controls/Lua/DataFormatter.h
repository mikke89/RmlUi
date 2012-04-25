#pragma once
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

namespace Rocket {
namespace Core {
namespace Lua {
typedef LuaDataFormatter DataFormatter;
//for DataFormatter.new
template<> void LuaType<DataFormatter>::extra_init(lua_State* L, int metatable_index);

//method
int DataFormatternew(lua_State* L);

//setter
int DataFormatterSetAttrFormatData(lua_State* L);

RegType<DataFormatter> DataFormatterMethods[];
luaL_reg DataFormatterGetters[];
luaL_reg DataFormatterSetters[];
}
}
}