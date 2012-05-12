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
 
#ifndef ROCKETCORELUALUATYPE_H
#define ROCKETCORELUALUATYPE_H


/*
    This is mostly the definition of the Lua userdata that we give to the user
*/
#include <Rocket/Core/Lua/Header.h>
#include <Rocket/Core/Lua/lua.hpp>


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
#define LUACHECKOBJ(obj) if((obj) == NULL) { lua_pushnil(L); return 1; }

//put this in the type.cpp file
    /*
#define LUATYPEDEFINE(type) \
    template<> inline const char* GetTClassName<type>() { return #type; } \
    template<> inline RegType<type>* GetMethodTable<type>() { return type##Methods; } \
    template<> inline luaL_reg* GetAttrTable<type>() { return type##Getters; } \
    template<> inline luaL_reg* SetAttrTable<type>() { return type##Setters; } \
*/
 //put this in the type.cpp file
#define LUATYPEDEFINE(type,is_ref_counted) \
    template<> const char* Rocket::Core::Lua::GetTClassName<type>() { return #type; } \
    template<> Rocket::Core::Lua::RegType<type>* Rocket::Core::Lua::GetMethodTable<type>() { return type##Methods; } \
    template<> luaL_reg* Rocket::Core::Lua::GetAttrTable<type>() { return type##Getters; } \
    template<> luaL_reg* Rocket::Core::Lua::SetAttrTable<type>() { return type##Setters; } \
    template<> bool Rocket::Core::Lua::IsReferenceCounted<type>() { return (is_ref_counted); } \

//put this in the type.h file. Not used at the moment
    /*
#define LUATYPEDECLARE(type) \
    template<> const char* GetTClassName<type>(); \
    template<> RegType<type>* GetMethodTable<type>(); \
    template<> luaL_reg* GetAttrTable<type>(); \
    template<> luaL_reg* SetAttrTable<type>(); \
*/
//put this in the type.h file
#define LUATYPEDECLARE(type) \
    template<> ROCKETLUA_API const char* Rocket::Core::Lua::GetTClassName<type>(); \
    template<> ROCKETLUA_API Rocket::Core::Lua::RegType<type>* Rocket::Core::Lua::GetMethodTable<type>(); \
    template<> ROCKETLUA_API luaL_reg* Rocket::Core::Lua::GetAttrTable<type>(); \
    template<> ROCKETLUA_API luaL_reg* Rocket::Core::Lua::SetAttrTable<type>(); \
    template<> ROCKETLUA_API bool Rocket::Core::Lua::IsReferenceCounted<type>(); \

namespace Rocket {
namespace Core {
namespace Lua {
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
//bool for if it is reference counted
template<typename T> ROCKETLUA_API bool IsReferenceCounted();
//gets called from the LuaType<T>::Register function, right before _regfunctions.
//If you want to inherit from another class, in the function you would want
//to call _regfunctions<superclass>, where method is metatable_index - 1. Anything
//that has the same name in the subclass will be overwrite whatever had the 
//same name in the superclass.
template<typename T> ROCKETLUA_API void ExtraInit(lua_State* L, int metatable_index) { return; }

template<typename T>
class ROCKETLUA_API LuaType
{
public:
    typedef int (*ftnptr)(lua_State* L, T* ptr);
    typedef struct { const char* name; ftnptr func; } RegType;

    static inline void Register(lua_State *L);
    static inline int push(lua_State *L, T* obj, bool gc=false);
    static inline T* check(lua_State* L, int narg);

    //for calling a C closure with upvalues
    static inline int thunk(lua_State* L);
    //pointer to string
    static inline void tostring(char* buff, void* obj);
    //these are metamethods
    //.__gc
    static inline int gc_T(lua_State* L);
    //.__tostring
    static inline int tostring_T(lua_State* L);
    //.__index
    static inline int index(lua_State* L);
    //.__newindex
    static inline int newindex(lua_State* L);
	
    //gets called from the Register function, right before _regfunctions.
    //If you want to inherit from another class, in the function you would want
    //to call _regfunctions<superclass>, where method is metatable_index - 1. Anything
    //that has the same name in the subclass will be overwrite whatever had the 
    //same name in the superclass.
    //static inline void extra_init(lua_State* L, int metatable_index);
    //Registers methods,getters,and setters to the type
    static inline void _regfunctions(lua_State* L, int meta, int method);
    //Says if it is a reference counted type. If so, then on push and __gc, do reference counting things
    //rather than regular new/delete. Note that it is still up to the user to pass "true" to the push function's
    //third parameter to be able to decrease the reference when Lua garbage collects an object
    //static inline bool is_reference_counted();
private:
    LuaType(); //hide constructor

};


}
}
}

#include "LuaType.inl" //this feels so dirty, but it is the only way I got it to compile in release
#endif