/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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
 
#include "LuaDataModel.h"
#include <RmlUi/Lua/Utilities.h>
#include <RmlUi/Core/DataVariable.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>

#define RMLDATAMODEL "RMLDATAMODEL"

namespace Rml {
namespace Lua {

namespace luabind {
#if LUA_VERSION_NUM < 503
	static void lua_reverse(lua_State* L, int a, int b) {
		for (; a < b; ++a, --b) {
			lua_pushvalue(L, a);
			lua_pushvalue(L, b);
			lua_replace(L, a);
			lua_replace(L, b);
		}
	}
	void lua_rotate(lua_State* L, int idx, int n) {
		int n_elems = 0;
		idx = lua_absindex(L, idx);
		n_elems = lua_gettop(L) - idx + 1;
		if (n < 0) {
			n += n_elems;
		}
		if (n > 0 && n < n_elems) {
			luaL_checkstack(L, 2, "not enough stack slots available");
			n = n_elems - n;
			lua_reverse(L, idx, idx + n - 1);
			lua_reverse(L, idx + n, idx + n_elems - 1);
			lua_reverse(L, idx, idx + n_elems - 1);
		}
	}
#endif
	using call_t = Rml::Function<void(void)>;
	inline int errhandler(lua_State* L) {
		const char* msg = lua_tostring(L, 1);
		if (msg == NULL) {
			if (luaL_callmeta(L, 1, "__tostring") && lua_type(L, -1) == LUA_TSTRING)
				return 1;
			else
				msg = lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, 1));
		}
		luaL_traceback(L, L, msg, 1);
		return 1;
	}
	inline void errfunc(const char* msg) {
		Log::Message(Log::LT_WARNING, "%s", msg);
	}
	inline int function_call(lua_State* L) {
		call_t& f = *(call_t*)lua_touserdata(L, 1);
		f();
		return 0;
	}
	inline bool invoke(lua_State* L, call_t f, int argn = 0) {
		if (!lua_checkstack(L, 3)) {
			errfunc("stack overflow");
			lua_pop(L, argn);
			return false;
		}
		lua_pushcfunction(L, errhandler);
		lua_pushcfunction(L, function_call);
		lua_pushlightuserdata(L, &f);
		lua_rotate(L, -argn - 3, 3);
		if (lua_pcall(L, 1 + argn, 0, lua_gettop(L) - argn - 2) != LUA_OK) {
			errfunc(lua_tostring(L, -1));
			lua_pop(L, 2);
			return false;
		}
		lua_pop(L, 1);
		return true;
	}
}

class LuaScalarDef;
class LuaTableDef;

struct LuaDataModel {
	DataModelConstructor constructor;
	DataModelHandle handle;
	lua_State *dataL;
	LuaScalarDef *scalarDef;
	LuaTableDef *tableDef;
	int top;
};

class LuaTableDef : public VariableDefinition {
public:
	LuaTableDef(const struct LuaDataModel* model);
	bool Get(void* ptr, Variant& variant) override;
	bool Set(void* ptr, const Variant& variant) override;
	int Size(void* ptr) override;
	DataVariable Child(void* ptr, const DataAddressEntry& address) override;
protected:
	const struct LuaDataModel* model;
};

class LuaScalarDef final : public LuaTableDef {
public:
	LuaScalarDef(const struct LuaDataModel* model);
	DataVariable Child(void* ptr, const DataAddressEntry& address) override;
};

LuaTableDef::LuaTableDef(const struct LuaDataModel *model)
	: VariableDefinition(DataVariableType::Scalar)
	, model(model)
{}

bool LuaTableDef::Get(void* ptr, Variant& variant) {
	lua_State *L = model->dataL;
	if (!L)
		return false;
	int id = (int)(intptr_t)ptr;
	GetVariant(L, id, &variant);
	return true;
}

bool LuaTableDef::Set(void* ptr, const Variant& variant) {
	int id = (int)(intptr_t)ptr;
	lua_State *L = model->dataL;
	if (!L)
		return false;
	PushVariant(L, &variant);
	lua_replace(L, id);
	return true;
}


static int
lLuaTableDefSize(lua_State* L) {
	lua_pushinteger(L, luaL_len(L, 1));
	return 1;
}

static int
lLuaTableDefChild(lua_State* L) {
	lua_gettable(L, 1);
	return 1;
}

int LuaTableDef::Size(void* ptr) {
	lua_State* L = model->dataL;
	if (!L)
		return 0;
	int id = (int)(intptr_t)ptr;
	if (lua_type(L, id) != LUA_TTABLE) {
		return 0;
	}
	if (!lua_checkstack(L, 4)) {
		return 0;
	}
	lua_pushcfunction(L, lLuaTableDefSize);
	lua_pushvalue(L, id);
	if (LUA_OK != lua_pcall(L, 1, 1, 0)) {
		lua_pop(L, 1);
		return 0;
	}
	int size = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);
	return size;
}

DataVariable LuaTableDef::Child(void* ptr, const DataAddressEntry& address) {
	lua_State* L = model->dataL;
	if (!L)
		return DataVariable{};
	int id = (int)(intptr_t)ptr;
	if (lua_type(L, id) != LUA_TTABLE) {
		return DataVariable{};
	}
	if (!lua_checkstack(L, 4)) {
		return DataVariable{};
	}
	lua_pushcfunction(L, lLuaTableDefChild);
	lua_pushvalue(L, id);
	if (address.index == -1) {
		lua_pushlstring(L, address.name.data(), address.name.size());
	}
	else {
		lua_pushinteger(L, (lua_Integer)address.index + 1);
	}
	if (LUA_OK != lua_pcall(L, 2, 1, 0)) {
		lua_pop(L, 1);
		return DataVariable{};
	}
	return DataVariable(model->tableDef, (void*)(intptr_t)lua_gettop(L));
}

LuaScalarDef::LuaScalarDef(const struct LuaDataModel* model)
	: LuaTableDef(model)
{}

DataVariable LuaScalarDef::Child(void* ptr, const DataAddressEntry& address) {
	lua_State* L = model->dataL;
	if (!L)
		return DataVariable{};
	lua_settop(L, model->top);
	return LuaTableDef::Child(ptr, address);
}

static void
BindVariable(struct LuaDataModel* D, lua_State* L) {
	lua_State* dataL = D->dataL;
	if (!lua_checkstack(dataL, 4)) {
		luaL_error(L, "Memory Error");
	}
	int id = lua_gettop(dataL) + 1;
	D->top = id;
	// L top : key value
	lua_xmove(L, dataL, 1);	// move value to dataL with index(id)
	lua_pushvalue(L, -1);	// dup key
	lua_xmove(L, dataL, 1);
	lua_pushinteger(dataL, id);
	lua_rawset(dataL, 1);
	const char* key = lua_tostring(L, -1);
	if (lua_type(dataL, D->top) == LUA_TFUNCTION) {
		D->constructor.BindEventCallback(key, [=](DataModelHandle, Event& event, const VariantList& varlist) {
			lua_pushvalue(dataL, id);
			lua_xmove(dataL, L, 1);
			luabind::invoke(L, [&](){
				LuaType<Event>::push(L,&event,false);
				for (auto const& variant : varlist) {
					PushVariant(L, &variant);
				}
				lua_call(L, (int)varlist.size() + 1, 0);
			}, 1);
		});
	}
	else {
		D->constructor.BindCustomDataVariable(key,
			DataVariable(D->scalarDef, (void*)(intptr_t)id)
		);
	}
}

static int
getId(lua_State *L, lua_State *dataL) {
	lua_pushvalue(dataL, 1);
	lua_xmove(dataL, L, 1);
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);
	if (lua_type(L, -1) != LUA_TNUMBER) {
		luaL_error(L, "DataModel has no key : %s", lua_tostring(L, 2));
	}
	int id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	return id;
}

static int
lDataModelGet(lua_State *L) {
	struct LuaDataModel *D = (struct LuaDataModel *)lua_touserdata(L, 1);
	lua_State *dataL = D->dataL;
	if (dataL == nullptr)
		luaL_error(L, "DataModel closed");
	int id = getId(L, dataL);
	lua_pushvalue(dataL, id);
	lua_xmove(dataL, L, 1);
	return 1;
}

static int
lDataModelSet(lua_State *L) {
	struct LuaDataModel *D = (struct LuaDataModel *)lua_touserdata(L, 1);
	lua_State *dataL = D->dataL;
	if (dataL == NULL)
		luaL_error(L, "DataModel released");
	lua_settop(dataL, D->top);

	lua_pushvalue(L, 2);
	lua_xmove(L, dataL, 1);
	lua_rawget(dataL, 1);
	if (lua_type(dataL, -1) == LUA_TNUMBER) {
		int id = (int)lua_tointeger(dataL, -1);
		lua_pop(dataL, 1);
		lua_xmove(L, dataL, 1);
		lua_replace(dataL, id);
		D->handle.DirtyVariable(lua_tostring(L, 2));
		return 0;
	}
	lua_pop(dataL, 1);
	BindVariable(D, L);
	return 0;
}

bool
OpenLuaDataModel(lua_State *L, Context *context, int name_index, int table_index) {
	String name = luaL_checkstring(L, name_index);
	luaL_checktype(L, table_index, LUA_TTABLE);

	DataModelConstructor constructor = context->CreateDataModel(name);
	if (!constructor) {
		constructor = context->GetDataModel(name);
		if (!constructor) {
			return false;
		}
	}

	struct LuaDataModel *D = (struct LuaDataModel *)lua_newuserdata(L, sizeof(*D));
	D->dataL = nullptr;
	D->scalarDef = nullptr;
	D->tableDef = nullptr;
	D->constructor = constructor;
	D->handle = constructor.GetModelHandle();

	D->scalarDef = new LuaScalarDef(D);
	D->tableDef = new LuaTableDef(D);
	D->dataL = lua_newthread(L);
	D->top = 1;
	lua_newtable(D->dataL);
	lua_pushnil(L);
	while (lua_next(L, table_index) != 0) {
		BindVariable(D, L);
	}
	lua_setuservalue(L, -2);

	if (luaL_newmetatable(L, RMLDATAMODEL)) {
		luaL_Reg l[] = {
			{ "__index", lDataModelGet },
			{ "__newindex", lDataModelSet },
			{ nullptr, nullptr },
		};
		luaL_setfuncs(L, l, 0);
	}
	lua_setmetatable(L, -2);

	return true;
}

// If you create all the Data Models from lua, you can store these LuaDataModel objects in a table,
// and call CloseLuaDataModel for each after Context released.
// We don't put it in __gc, becuase LuaDataModel can be free before DataModel if you are not careful.
// scalarDef will free by CloseLuaDataModel, but DataModel need it.
void
CloseLuaDataModel(lua_State *L) {
	luaL_checkudata(L, -1, RMLDATAMODEL);
	struct LuaDataModel *D = (struct LuaDataModel *)lua_touserdata(L, -1);
	D->dataL = nullptr;
	D->top = 0;
	delete D->scalarDef;
	D->scalarDef = nullptr;
	delete D->tableDef;
	D->tableDef = nullptr;
	lua_pushnil(L);
	lua_setuservalue(L, -2);
}

} // namespace Lua
} // namespace Rml




