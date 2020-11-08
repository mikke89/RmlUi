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

class LuaScalarDef;

struct LuaDataModel {
	DataModelHandle handle;
	lua_State *dataL;
	LuaScalarDef *scalarDef;
};


class LuaScalarDef final : public VariableDefinition {
public:
	LuaScalarDef (const struct LuaDataModel *model) :
		VariableDefinition(DataVariableType::Scalar), model(model) {}
private:
	bool Get(void* ptr, Variant& variant) override {
		lua_State *L = model->dataL;
		if (!L)
			return false;
		int id = int((intptr_t)ptr);
		GetVariant(L, id, &variant);
		return true;
	}
	bool Set(void* ptr, const Rml::Variant& variant) override {
		int id = int((intptr_t)ptr);
		lua_State *L = model->dataL;
		if (!L)
			return false;
		PushVariant(L, &variant);
		lua_replace(L, id);
		return true;
	}

	const struct LuaDataModel *model;
};

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
	if (dataL == nullptr)
		luaL_error(L, "DataModel closed");
	int id = getId(L, dataL);
	lua_xmove(L, dataL, 1);
	lua_replace(dataL, id);
	D->handle.DirtyVariable(lua_tostring(L, 2));
	return 0;
}

// Construct a lua sub thread for LuaDataModel
// stack 1 : { name(string) -> id(integer) }
// stack 2- : values
// For example : build from { str = "Hello", x = 0 }
//	1: { str = 2 , x = 3 }
//	2: "Hello"
//	3: 0
static lua_State *
InitDataModelFromTable(lua_State *L, int index, Rml::DataModelConstructor &ctor, class LuaScalarDef *def) {
	lua_State *dataL = lua_newthread(L);
	lua_newtable(dataL);
	intptr_t id = 2;
	lua_pushnil(L);
	while (lua_next(L, index) != 0) {
		if (!lua_checkstack(dataL, 4)) {
			luaL_error(L, "Memory Error");
		}
		// L top : key value
		lua_xmove(L, dataL, 1);	// move value to dataL with index(id)
		lua_pushvalue(L, -1);	// dup key
		lua_xmove(L, dataL, 1);
		lua_pushinteger(dataL, id);
		lua_rawset(dataL, 1);
		const char *key = lua_tostring(L, -1);
		ctor.BindCustomDataVariable(key, Rml::DataVariable(def, (void *)id));
		++id;
	}
	return dataL;
}

bool
OpenLuaDataModel(lua_State *L, Rml::Context *context, int name_index, int table_index) {
	Rml::String name = luaL_checkstring(L, name_index);
	luaL_checktype(L, table_index, LUA_TTABLE);

	Rml::DataModelConstructor constructor = context->CreateDataModel(name);
	if (!constructor) {
		constructor = context->GetDataModel(name);
		if (!constructor) {
			return false;
		}
	}

	struct LuaDataModel *D = (struct LuaDataModel *)lua_newuserdata(L, sizeof(*D));
	D->dataL = nullptr;
	D->scalarDef = nullptr;
	D->handle = constructor.GetModelHandle();

	D->scalarDef = new LuaScalarDef(D);
	D->dataL = InitDataModelFromTable(L, table_index, constructor, D->scalarDef);
	lua_setuservalue(L, -2);	// set D->dataL into uservalue of D

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
	delete D->scalarDef;
	D->scalarDef = nullptr;
	lua_pushnil(L);
	lua_setuservalue(L, -2);
}

} // namespace Lua
} // namespace Rml




