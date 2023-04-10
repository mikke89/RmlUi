/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
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

namespace Rml {
namespace Lua {

	template <typename T>
	void LuaType<T>::Register(lua_State* L)
	{
		// for annotations, starting at 1, but it is a relative value, not always 1
		lua_newtable(L);             //[1] = table
		int methods = lua_gettop(L); // methods = 1

		luaL_newmetatable(L, GetTClassName<T>()); //[2] = metatable named <ClassName>, referred in here by ClassMT
		int metatable = lua_gettop(L);            // metatable = 2

		luaL_newmetatable(L, "DO NOT TRASH"); //[3] = metatable named "DO NOT TRASH"
		lua_pop(L, 1);                        // remove the above metatable -> [-1 = 2]

		// store method table in globals so that scripts can add functions written in Lua
		lua_pushvalue(L, methods);            //[methods = 1] -> [3] = copy (reference) of methods table
		lua_setglobal(L, GetTClassName<T>()); // -> <ClassName> = [3 = 1], pop top [3]

		// hide metatable from Lua getmetatable()
		lua_pushvalue(L, methods);                 //[methods = 1] -> [3] = copy of methods table, including modifications above
		lua_setfield(L, metatable, "__metatable"); //[metatable = 2] -> t[k] = v; t = [2 = ClassMT], k = "__metatable", v = [3 = 1]; pop [3]

		lua_pushcfunction(L, index);           // index = cfunction -> [3] = cfunction
		lua_setfield(L, metatable, "__index"); //[metatable = 2] -> t[k] = v; t = [2], k = "__index", v = cfunc; pop [3]

		lua_pushcfunction(L, newindex);
		lua_setfield(L, metatable, "__newindex");

		lua_pushcfunction(L, gc_T);
		lua_setfield(L, metatable, "__gc");

		lua_pushcfunction(L, tostring_T);
		lua_setfield(L, metatable, "__tostring");

		lua_pushcfunction(L, eq_T);
		lua_setfield(L, metatable, "__eq");

		ExtraInit<T>(L, metatable); // optionally implemented by individual types

		lua_newtable(L);              // for method table -> [3] = this table
		lua_setmetatable(L, methods); //[methods = 1] -> metatable for [1] is [3]; [3] is popped off, top = [2]

		_regfunctions(L, metatable, methods);

		lua_pop(L, 2); // remove the two items from the stack, [1 = methods] and [2 = metatable]
	}

	template <typename T>
	int LuaType<T>::push(lua_State* L, T* obj, bool gc)
	{
		// for annotations, starting at index 1, but it is a relative number, not always 1
		if (!obj)
		{
			lua_pushnil(L);
			return lua_gettop(L);
		}
		luaL_getmetatable(L, GetTClassName<T>()); // lookup metatable in Lua registry ->[1] = metatable of <ClassName>
		if (lua_isnil(L, -1))
			luaL_error(L, "%s missing metatable", GetTClassName<T>());
		int mt = lua_gettop(L);                             // mt = 1
		T** ptrHold = (T**)lua_newuserdata(L, sizeof(T**)); //->[2] = empty userdata
		int ud = lua_gettop(L);                             // ud = 2
		if (ptrHold != nullptr)
		{
			*ptrHold = obj;
			lua_pushvalue(L, mt);    // ->[3] = copy of [1]
			lua_setmetatable(L, -2); //[-2 = 2] -> [2]'s metatable = [3]; pop [3]
			char name[max_pointer_string_size];
			tostring(name, max_pointer_string_size, ptrHold);
			lua_getfield(L, LUA_REGISTRYINDEX, "DO NOT TRASH"); //->[3] = value returned from function
			if (lua_isnil(L, -1))                               // if [3] hasn't been created yet, then create it
			{
				luaL_newmetatable(L, "DO NOT TRASH"); //[4] = the new metatable
				lua_pop(L, 1);                        // pop [4]
			}
			lua_pop(L, 1);                                      // pop [3]
			lua_getfield(L, LUA_REGISTRYINDEX, "DO NOT TRASH"); //->[3] = value returned from function
			if (gc == false)                                    // if we shouldn't garbage collect it, then put the name in to [3]
			{
				lua_pushboolean(L, 1);     // ->[4] = true
				lua_setfield(L, -2, name); // represents t[k] = v, [-2 = 3] = t -> v = [4], k = <ClassName>; pop [4]
			}
			else
			{
				// In case this is an address that has been pushed
				// to lua before, we need to set it to nil
				lua_pushnil(L);            // ->[4] = nil
				lua_setfield(L, -2, name); // represents t[k] = v, [-2 = 3] = t -> v = [4], k = <ClassName>; pop [4]
			}

			lua_pop(L, 1); // -> pop [3]
		}
		lua_settop(L, ud);  //[ud = 2] -> remove everything that is above 2, top = [2]
		lua_replace(L, mt); //[mt = 1] -> move [2] to pos [1], and pop previous [1]
		lua_settop(L, mt);  // remove everything above [1]
		return mt;          // index of userdata containing pointer to T object
	}

	template <typename T>
	T* LuaType<T>::check(lua_State* L, int narg)
	{
		T** ptrHold = static_cast<T**>(lua_touserdata(L, narg));
		if (ptrHold == nullptr)
			return nullptr;
		return (*ptrHold);
	}

	// private members

	template <typename T>
	int LuaType<T>::thunk(lua_State* L)
	{
		// stack has userdata, followed by method args
		T* obj = check(L, 1); // get 'self', or if you prefer, 'this'
		lua_remove(L, 1);     // remove self so member function args start at index 1
		// get member function from upvalue
		RegType* l = static_cast<RegType*>(lua_touserdata(L, lua_upvalueindex(1)));
		// at the moment, there isn't a case where nullptr is acceptable to be used in the function, so check
		// for it here, rather than individually for each function
		if (obj == nullptr)
		{
			lua_pushnil(L);
			return 1;
		}
		else
			return l->func(L, obj); // call member function
	}

	template <typename T>
	void LuaType<T>::tostring(char* buff, size_t buff_size, void* obj)
	{
		snprintf(buff, buff_size, "%p", obj);
	}

	template <typename T>
	int LuaType<T>::gc_T(lua_State* L)
	{
		T* obj = check(L, 1); //[1] = this userdata
		if (obj == nullptr)
			return 0;

		lua_getfield(L, LUA_REGISTRYINDEX, "DO NOT TRASH"); //->[2] = return value from this
		if (lua_istable(L, -1))                             //[-1 = 2], if it is a table
		{
			char name[max_pointer_string_size];
			void* ptrHold = lua_touserdata(L, 1);
			tostring(name, max_pointer_string_size, ptrHold);
			lua_getfield(L, -1, name);  //[-1 = 2] -> [3] = the value returned from if <ClassName> exists in the table to not gc
			if (lua_isnoneornil(L, -1)) //[-1 = 3] if it doesn't exist, then we are free to garbage collect c++ side
			{
				delete obj;
				obj = nullptr;

				// Change the field to not gc the next time we encounter this pointer. This may be necessary in case the
				// just deleted object shared an address with a previously deleted (non-GCed) object, the latter which
				// this function will be called upon later.
				lua_pushboolean(L, 1);     // ->[4] = true
				lua_setfield(L, -3, name); // represents t[k] = v, [-3 = 2] = t -> v = [4], k = <ClassName>; pop [4]
			}
		}
		lua_pop(L, 3); // balance function
		return 0;
	}

	template <typename T>
	int LuaType<T>::tostring_T(lua_State* L)
	{
		char buff[max_pointer_string_size];
		T** ptrHold = (T**)lua_touserdata(L, 1);
		void* obj = static_cast<void*>(*ptrHold);
		snprintf(buff, max_pointer_string_size, "%p", obj);
		lua_pushfstring(L, "%s (%s)", GetTClassName<T>(), buff);
		return 1;
	}

	template <typename T>
	int LuaType<T>::eq_T(lua_State* L)
	{
		T* o1 = check(L, 1);
		RMLUI_CHECK_OBJ(o1);
		T* o2 = check(L, 2);
		RMLUI_CHECK_OBJ(o2);
		lua_pushboolean(L, o1 == o2);
		return 1;
	}

	template <typename T>
	int LuaType<T>::index(lua_State* L)
	{
		const char* class_name = GetTClassName<T>();
		return LuaTypeImpl::index(L, class_name);
	}

	template <typename T>
	int LuaType<T>::newindex(lua_State* L)
	{
		const char* class_name = GetTClassName<T>();
		return LuaTypeImpl::newindex(L, class_name);
	}

	template <typename T>
	void LuaType<T>::_regfunctions(lua_State* L, int /*meta*/, int methods)
	{
		// fill method table with methods.
		for (RegType* m = (RegType*)GetMethodTable<T>(); m->name; m++)
		{
			lua_pushstring(L, m->name);         // ->[1] = name of function Lua side
			lua_pushlightuserdata(L, (void*)m); // ->[2] = pointer to the object containing the name and the function pointer as light userdata
			lua_pushcclosure(L, thunk, 1);      // thunk = function pointer -> pop 1 item from stack, [2] = closure
			lua_settable(L, methods);           // represents t[k] = v, t = [methods] -> pop [2 = closure] to be v, pop [1 = name] to be k
		}

		lua_getfield(L, methods, "__getters"); // -> table[1]
		if (lua_isnoneornil(L, -1))
		{
			lua_pop(L, 1);                         // pop unsuccessful get
			lua_newtable(L);                       // -> table [1]
			lua_setfield(L, methods, "__getters"); // pop [1]
			lua_getfield(L, methods, "__getters"); // -> table [1]
		}
		for (luaL_Reg* m = (luaL_Reg*)GetAttrTable<T>(); m->name; m++)
		{
			lua_pushcfunction(L, m->func); // -> [2] is this function
			lua_setfield(L, -2, m->name);  //[-2 = 1] -> __getters.name = function
		}
		lua_pop(L, 1); // pop __getters

		lua_getfield(L, methods, "__setters"); // -> table[1]
		if (lua_isnoneornil(L, -1))
		{
			lua_pop(L, 1);                         // pop unsuccessful get
			lua_newtable(L);                       // -> table [1]
			lua_setfield(L, methods, "__setters"); // pop [1]
			lua_getfield(L, methods, "__setters"); // -> table [1]
		}
		for (luaL_Reg* m = (luaL_Reg*)SetAttrTable<T>(); m->name; m++)
		{
			lua_pushcfunction(L, m->func); // -> [2] is this function
			lua_setfield(L, -2, m->name);  //[-2 = 1] -> __setters.name = function
		}
		lua_pop(L, 1); // pop __setters
	}

} // namespace Lua
} // namespace Rml
