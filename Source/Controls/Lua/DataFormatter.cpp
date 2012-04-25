#include "precompiled.h"
#include "DataFormatter.h"

namespace Rocket {
namespace Core {
namespace Lua {

//method
int DataFormatternew(lua_State* L)
{
    DataFormatter* df;
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
    LUACHECKOBJ(obj);
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
    { NULL, NULL },
};

luaL_reg DataFormatterGetters[] =
{
    { NULL, NULL },
};

luaL_reg DataFormatterSetters[] =
{
    LUASETTER(DataFormatter,FormatData)
    { NULL, NULL },
};

}
}
}