#pragma once

/*
    This is mostly the definition of the Lua userdata that we give to the user
*/
#include "Header.h"
#include "lua.hpp"

namespace Rocket {
namespace Core {
namespace Lua {

//As an example, if you used this macro like
//LUAMETHOD(Unit,GetId)
//it would result in code that looks like
//{ "GetId", UnitGetId },
//Which would force you to create a global function named UnitGetId in C with the correct function signature, usually int(*)(lua_State*,type*);
#define LUAMETHOD(type,name) { #name, type##name },

//See above, but the method must match the function signature int(*)(lua_State*) and as example:
//LUAGETTER(Unit,Id) would mean you need a function named UnitGetAttrId
//The first stack position will be the userdata
#define LUAGETTER(type,varname) { #varname, type##GetAttr##varname },

//Same method signature as above, but as example:
//LUASETTER(Unit,Id) would mean you need a function named UnitSetAttrId
//The first stack position will be the userdata, and the second will be value on the other side of the equal sign
#define LUASETTER(type,varname) { #varname, type##SetAttr##varname },

#define CHECK_BOOL(L,narg) (lua_toboolean((L),(narg)) > 0 ? true : false )
#define LUACHECKOBJ(obj) if(obj == NULL) { lua_pushnil(L); return 1; }


//replacement for luaL_reg that uses a different function pointer signature, but similar syntax
template<typename T>
struct ROCKETLUA_API RegType
{
    const char* name;
    int (*ftnptr)(lua_State*,T*);
};

//this is for all of the methods available from Lua that call to the C functions
template<typename T> ROCKETLUA_API RegType<T>* GetMethodTable();
//this is for all of the function that 'get' an attribute/property
template<typename T> ROCKETLUA_API luaL_reg* GetAttrTable();
//this is for all of the functions that 'set' an attribute/property
template<typename T> ROCKETLUA_API luaL_reg* SetAttrTable();
//String representation of the class
template<typename T> ROCKETLUA_API const char* GetTClassName();

template<typename T>
class ROCKETLUA_API LuaType
{
public:
    typedef int (*ftnptr)(lua_State* L, T* ptr);
    typedef struct { const char* name; ftnptr func; } RegType;

    static void Register(lua_State *L);
    static int push(lua_State *L, T* obj, bool gc=false);
    static T* check(lua_State* L, int narg);

    //for calling a C closure with upvalues
    static int thunk(lua_State* L);
    //pointer to string
    static void tostring(char* buff, void* obj);
    //these are metamethods
    //.__gc
    static int gc_T(lua_State* L);
    //.__tostring
    static int tostring_T(lua_State* L);
    //.__index
    static int index(lua_State* L);
    //.__newindex
    static int newindex(lua_State* L);
	
    //gets called from the Register function, right before _regfunctions.
    //If you want to inherit from another class, in the function you would want
    //to call _regfunctions<superclass>, where method is metatable_index - 1. Anything
    //that has the same name in the subclass will be overwrite whatever had the 
    //same name in the superclass.
    static inline void extra_init(lua_State* L, int metatable_index);
    //Registers methods,getters,and setters to the type
    static inline void _regfunctions(lua_State* L, int meta, int method);
private:
    LuaType(); //hide constructor

};

}
}
}

#include "LuaType.inl"