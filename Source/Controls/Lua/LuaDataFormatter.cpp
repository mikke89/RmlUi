#include "precompiled.h"
#include "LuaDataFormatter.h"
#include <Rocket/Core/Lua/Interpreter.h>
#include <Rocket/Core/Log.h>


namespace Rocket {
namespace Core {
namespace Lua {

LuaDataFormatter::LuaDataFormatter(const String& name) : Rocket::Controls::DataFormatter(name), ref_FormatData(LUA_NOREF)
{
    
}

LuaDataFormatter::~LuaDataFormatter()
{
    //empty
}

void LuaDataFormatter::FormatData(Rocket::Core::String& formatted_data, const Rocket::Core::StringList& raw_data)
{
    if(ref_FormatData == LUA_NOREF || ref_FormatData == LUA_REFNIL)
    {
        Log::Message(Log::LT_ERROR, "In LuaDataFormatter: There is no value assigned to the \"FormatData\" variable.");
        return;
    }
    lua_State* L = Interpreter::GetLuaState();
    PushDataFormatterFunctionTable(L); // push the table where the function resides
    lua_rawgeti(L,-1,ref_FormatData); //push the function
    if(lua_type(L,-1) != LUA_TFUNCTION)
    {
        Log::Message(Log::LT_ERROR, "In LuaDataFormatter: The value for the FormatData variable must be a function. You passed in a %s.", lua_typename(L,lua_type(L,-1)));
        return;
    }
    lua_newtable(L); //to hold raw_data
    int tbl = lua_gettop(L);
    for(unsigned int i = 0; i < raw_data.size(); i++)
    {
        lua_pushstring(L,raw_data[i].CString());
        lua_rawseti(L,tbl,i);
    }
    Interpreter::ExecuteCall(1,1); //1 parameter (the table), 1 result (a string)

    //top of the stack should be the return value
    if(lua_type(L,-1) != LUA_TSTRING)
    {
        Log::Message(Log::LT_ERROR, "In LuaDataFormatter: the return value of FormatData must be a string. You returned a %s.", lua_typename(L,lua_type(L,-1)));
        return;
    }
    formatted_data = String(lua_tostring(L,-1));
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

}
}
}
