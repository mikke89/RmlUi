namespace Rml {
namespace Lua {

	template <typename T>
	void LuaTypeMetatable<T, typename std::enable_if<has_custom_rtti<T>::value>::type>::Register(lua_State* L)
	{
		lua_newtable(L);
		lua_pushlightuserdata(L, T::GetStaticClassIdentifier());
		lua_pushvalue(L, -2);
		lua_settable(L, LUA_REGISTRYINDEX);
	}

	template <typename T>
	void LuaTypeMetatable<T, typename std::enable_if<has_custom_rtti<T>::value>::type>::Get(lua_State* L,
			const T& obj)
	{
		lua_pushlightuserdata(L, obj.GetClassIdentifier());
		lua_gettable(L, LUA_REGISTRYINDEX);
	}

	template <typename T>
	void LuaTypeMetatable<T, typename std::enable_if<!has_custom_rtti<T>::value>::type>::Register(lua_State* L)
	{
		luaL_newmetatable(L, GetTClassName<T>());
	}

	template <typename T>
	void LuaTypeMetatable<T, typename std::enable_if<!has_custom_rtti<T>::value>::type>::Get(lua_State* L,
			const T& /*obj*/)
	{
		luaL_getmetatable(L, GetTClassName<T>());
	}

	template <typename T>
	void LuaType<T>::Register(lua_State* L)
	{
		static_assert(is_complete<T>::value, "Cannot register an incomplete type");

		// for annotations, starting at 1, but it is a relative value, not always 1
		lua_newtable(L);             //[1] = table
		int methods = lua_gettop(L); // methods = 1

		LuaTypeMetatable<T>::Register(L); //[2] = metatable referred in here by ClassMT
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

		if constexpr (LuaTypeTraits<T>::lua_owned)
		{
			lua_pushcfunction(L, gc_T);
			lua_setfield(L, metatable, "__gc");
		}

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
	void LuaType<T>::push(lua_State* L, T* obj)
	{
		if (!obj)
		{
			lua_pushnil(L);
			return;
		}

		push(L, *obj);
	}

	template <typename T>
	void LuaType<T>::push(lua_State* L, T& obj)
	{
		using storage_type = typename LuaTypeTraits<T>::storage_type;
		static_assert(is_complete<T>::value, "Cannot push an incomplete type");

		// for annotations, starting at index 1, but it is a relative number, not always 1
		// [1] = empty userdata
		auto ptrHold = reinterpret_cast<storage_type*>(lua_newuserdata(L, sizeof(storage_type))); 

		// Handle failure to allocate
		if (!ptrHold)
			return;

		if constexpr (LuaTypeTraits<T>::lua_owned)
			ptrHold = ::new (ptrHold) storage_type(obj);
		else
			ptrHold = ::new (ptrHold) storage_type(&obj);

		LuaTypeMetatable<T>::Get(L, obj); // lookup metatable in Lua registry ->[2] = metatable of <ClassName>
		if (lua_isnil(L, -1))
		{
			luaL_error(L, "%s missing metatable", GetTClassName<T>());
			return;
		}

		lua_setmetatable(L, -2); // [-2] = ptrHold, [-1] = metatable
	}

	template <typename T>
	void LuaType<T>::push(lua_State* L, ElementPtr&& ptr)
	{
		using storage_type = typename LuaTypeTraits<T>::storage_type;
		static_assert(is_complete<T>::value, "Cannot push an incomplete type");
		static_assert(LuaTypeTraits<T>::use_script_ptr, "Can only push ElementPtr for use_script_ptr types");

		if (!ptr)
		{
			lua_pushnil(L);
			return;
		}

		auto& obj = *ptr;

		// for annotations, starting at index 1, but it is a relative number, not always 1
		// [1] = empty userdata
		auto ptrHold = reinterpret_cast<storage_type*>(lua_newuserdata(L, sizeof(storage_type))); 

		// Handle failure to allocate
		if (!ptrHold)
			return;

		ptrHold = ::new (ptrHold) storage_type(std::move(ptr));

		LuaTypeMetatable<T>::Get(L, obj); // lookup metatable in Lua registry ->[2] = metatable of <ClassName>
		if (lua_isnil(L, -1))
		{
			luaL_error(L, "%s missing metatable", GetTClassName<T>());
			return;
		}

		lua_setmetatable(L, -2); // [-2] = ptrHold, [-1] = metatable
	}

	template <typename T>
	template <typename... Args>
	void LuaType<T>::emplace(lua_State* L, Args&&... args) {
		using storage_type = typename LuaTypeTraits<T>::storage_type;
		static_assert(is_complete<T>::value, "Cannot push an incomplete type");

		// for annotations, starting at index 1, but it is a relative number, not always 1
		// [1] = empty userdata
		auto ptrHold = reinterpret_cast<storage_type*>(lua_newuserdata(L, sizeof(storage_type))); 

		// Handle failure to allocate
		if (!ptrHold)
			return;

		if constexpr (std::is_aggregate_v<storage_type>)
			ptrHold = ::new (ptrHold) storage_type{std::forward<Args>(args)...};
		else
			ptrHold = ::new (ptrHold) storage_type(std::forward<Args>(args)...);

		T* obj;

		if constexpr (LuaTypeTraits<T>::use_script_ptr)
			obj = static_cast<T*>(ptrHold->get());
		else
			obj = static_cast<T*>(ptrHold);

		LuaTypeMetatable<T>::Get(L, *obj); // lookup metatable in Lua registry ->[2] = metatable of <ClassName>
		if (lua_isnil(L, -1))
		{
			luaL_error(L, "%s missing metatable", GetTClassName<T>());
			return;
		}

		lua_setmetatable(L, -2); // [-2] = ptrHold, [-1] = metatable
	}

	template <typename T>
	T* LuaType<T>::check(lua_State* L, int narg)
	{
		using storage_type = typename LuaTypeTraits<T>::storage_type;

		auto ptrHold = reinterpret_cast<storage_type*>(lua_touserdata(L, narg));
		if (ptrHold == nullptr)
			return nullptr;

		if constexpr (LuaTypeTraits<T>::use_script_ptr)
			return static_cast<T*>(ptrHold->get());
		else if constexpr (LuaTypeTraits<T>::lua_owned)
			return static_cast<T*>(ptrHold);
		else
			return *static_cast<T**>(ptrHold);
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
		using storage_type = typename LuaTypeTraits<T>::storage_type;

		if constexpr (LuaTypeTraits<T>::lua_owned)
		{
			auto ptrHold = reinterpret_cast<storage_type*>(lua_touserdata(L, 1));

			if (ptrHold)
			{
				ptrHold->~storage_type();
			}
		}

		return 0;
	}

	template <typename T>
	int LuaType<T>::tostring_T(lua_State* L)
	{
		using storage_type = typename LuaTypeTraits<T>::storage_type;

		char buff[max_pointer_string_size];
		auto ptrHold = reinterpret_cast<storage_type*>(lua_touserdata(L, 1));
		void* obj; 

		if constexpr (LuaTypeTraits<T>::use_script_ptr)
			obj = reinterpret_cast<void*>(ptrHold->get());
		else if constexpr (LuaTypeTraits<T>::lua_owned)
			obj = reinterpret_cast<void*>(ptrHold);
		else
			obj = reinterpret_cast<void*>(*ptrHold);

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
