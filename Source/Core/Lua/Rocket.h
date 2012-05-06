#ifndef ROCKETCORELUAROCKET_H
#define ROCKETCORELUAROCKET_H

/*
    This declares rocket in the global Lua namespace

    It is not exactly type, but it is a table, and does have some methods and attributes
    
    methods: 
    rocket.CreateContext(string name, Vector2i dimensions)
    rocket.LoadFontFace(string font_path)
    rocket.RegisterTag(string tag, class) --NYI
    
    getters:
    rocket.contexts  returns a table of contexts, which are indexed by number AND string, so there are two copies of each context in the table

*/

#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Core/Lua/lua.hpp>

namespace Rocket {
namespace Core {
namespace Lua {
#define ROCKETLUA_INPUTENUM(keyident,tbl) lua_pushinteger(L,Input::KI_##keyident); lua_setfield(L,(tbl),#keyident);

//just need a class to take up a type name
class rocket { int to_remove_warning; };

template<> void LuaType<rocket>::extra_init(lua_State* L, int metatable_index);
int rocketCreateContext(lua_State* L);
int rocketLoadFontFace(lua_State* L);
int rocketRegisterTag(lua_State* L);
int rocketGetAttrcontexts(lua_State* L);

void rocketEnumkey_identifier(lua_State* L);

RegType<rocket> rocketMethods[];
luaL_reg rocketGetters[];
luaL_reg rocketSetters[];

/*
template<> const char* GetTClassName<rocket>();
template<> RegType<rocket>* GetMethodTable<rocket>();
template<> luaL_reg* GetAttrTable<rocket>();
template<> luaL_reg* SetAttrTable<rocket>();
*/
}
}
}
#endif
