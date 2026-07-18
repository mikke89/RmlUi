#include "LuaDataModel.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/DataVariable.h>
#include <RmlUi/Lua/Utilities.h>

#define RMLDATAMODEL "RMLDATAMODEL"
#define RMLTABLEPROXY "RMLTABLEPROXY"

namespace Rml {
namespace Lua {

namespace luabind {
#if LUA_VERSION_NUM < 503
static void lua_reverse(lua_State* L, int a, int b)
{
	for (; a < b; ++a, --b)
	{
		lua_pushvalue(L, a);
		lua_pushvalue(L, b);
		lua_replace(L, a);
		lua_replace(L, b);
	}
}
void lua_rotate(lua_State* L, int idx, int n)
{
	int n_elems = 0;
	idx = lua_absindex(L, idx);
	n_elems = lua_gettop(L) - idx + 1;
	if (n < 0)
	{
		n += n_elems;
	}
	if (n > 0 && n < n_elems)
	{
		luaL_checkstack(L, 2, "not enough stack slots available");
		n = n_elems - n;
		lua_reverse(L, idx, idx + n - 1);
		lua_reverse(L, idx + n, idx + n_elems - 1);
		lua_reverse(L, idx, idx + n_elems - 1);
	}
}
#endif
using call_t = Rml::Function<void(void)>;
inline int errhandler(lua_State* L)
{
	const char* msg = lua_tostring(L, 1);
	if (msg == NULL)
	{
		if (luaL_callmeta(L, 1, "__tostring") && lua_type(L, -1) == LUA_TSTRING)
			return 1;
		else
			msg = lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, 1));
	}
	luaL_traceback(L, L, msg, 1);
	return 1;
}
inline void errfunc(const char* msg)
{
	Log::Message(Log::LT_WARNING, "%s", msg);
}
inline int function_call(lua_State* L)
{
	call_t& f = *(call_t*)lua_touserdata(L, 1);
	f();
	return 0;
}
inline bool invoke(lua_State* L, call_t f, int argn = 0)
{
	if (!lua_checkstack(L, 3))
	{
		errfunc("stack overflow");
		lua_pop(L, argn);
		return false;
	}
	lua_pushcfunction(L, errhandler);
	lua_pushcfunction(L, function_call);
	lua_pushlightuserdata(L, &f);
	lua_rotate(L, -argn - 3, 3);
	if (lua_pcall(L, 1 + argn, 0, lua_gettop(L) - argn - 2) != LUA_OK)
	{
		errfunc(lua_tostring(L, -1));
		lua_pop(L, 2);
		return false;
	}
	lua_pop(L, 1);
	return true;
}
} // namespace luabind

class LuaObjectDef;

struct LuaDataModel {
	DataModelConstructor constructor;
	DataModelHandle handle;
	lua_State* L;
	LuaObjectDef* objectDef;
	int model_ref = LUA_NOREF;
	int valuestore_ref = LUA_NOREF;
	int tablestore_ref = LUA_NOREF;
	int keystore_ref = LUA_NOREF;
};

struct TableProxy {
	LuaDataModel* model;
	int table_ref = LUA_NOREF;
	int key_ref = LUA_NOREF;
};

static struct TableProxy* NewTableProxy(struct LuaDataModel* model, int key_ref);
static struct TableProxy* ToTableProxy(lua_State* L, int index)
{
	return (struct TableProxy*)luaL_testudata(L, index, RMLTABLEPROXY);
}

class LuaObjectDef : public VariableDefinition {
public:
	LuaObjectDef(struct LuaDataModel* model);
	bool Get(void* ptr, Variant& variant) override;
	bool Set(void* ptr, const Variant& variant) override;
	int Size(void* ptr) override;
	DataVariable Child(void* ptr, const DataAddressEntry& address) override;

protected:
	struct LuaDataModel* model;
};

void PushValue(struct LuaDataModel* model, int ref)
{
	lua_State* L = model->L;
	lua_rawgeti(L, LUA_REGISTRYINDEX, model->valuestore_ref);
	lua_rawgeti(L, -1, ref);
	lua_replace(L, -2);
}

void SetValue(struct LuaDataModel* model, int ref)
{
	// -1 = value
	lua_State* L = model->L;
	lua_rawgeti(L, LUA_REGISTRYINDEX, model->valuestore_ref);
	lua_pushvalue(L, -2);
	lua_rawseti(L, -2, ref);
	lua_pop(L, 2);
}

int ChildRef(struct LuaDataModel* model, int ref)
{
	// -1 = key
	lua_State* L = model->L;
	lua_rawgeti(L, LUA_REGISTRYINDEX, model->tablestore_ref);
	// -1 = tablestore, -2 = key
	lua_rawgeti(L, -1, ref);
	// -1 = tablestore entry / nil, -2 = tablestore, -3 = key
	lua_replace(L, -2);
	// -1 = tablestore entry / nil, -2 = key

	if (lua_type(L, -1) != LUA_TTABLE)
	{
		lua_pop(L, 1);
		// -1 = key;
		return LUA_REFNIL;
	}

	lua_pushvalue(L, -2);
	// -1 = key, -2 = tablestore entry, -3 = key
	lua_rawget(L, -2);
	// -1 = child ref / nil, -2 = tablestore entry, -3 = key

	if (lua_isnil(L, -1))
	{
		lua_pop(L, 2);
		// -1 = key;
		return LUA_REFNIL;
	}

	int child_ref = (int)lua_tonumber(L, -1);
	lua_pop(L, 2);
	// -1 = key;
	return child_ref;
}

LuaObjectDef::LuaObjectDef(struct LuaDataModel* model) : VariableDefinition(DataVariableType::Scalar), model(model) {}

bool LuaObjectDef::Get(void* ptr, Variant& variant)
{
	lua_State* L = model->L;
	if (!L)
		return false;
	int ref = (int)(intptr_t)ptr;
	PushValue(model, ref);
	GetVariant(L, -1, &variant);
	lua_pop(L, 1);
	return true;
}

bool LuaObjectDef::Set(void* ptr, const Variant& variant)
{
	int ref = (int)(intptr_t)ptr;
	lua_State* L = model->L;
	if (!L || ref == LUA_REFNIL)
		return false;
	PushVariant(L, &variant);
	SetValue(model, ref);
	return true;
}

int LuaObjectDef::Size(void* ptr)
{
	lua_State* L = model->L;
	if (!L)
		return 0;
	int ref = (int)(intptr_t)ptr;
	PushValue(model, ref);

	struct TableProxy* proxy = ToTableProxy(L, -1);
	lua_pop(L, 1);

	if (proxy == nullptr || proxy->model != model)
		return 1;

	PushValue(model, proxy->table_ref);
	int size = (int)luaL_len(L, -1);
	lua_pop(L, 1);
	return size;
}

DataVariable LuaObjectDef::Child(void* ptr, const DataAddressEntry& address)
{
	lua_State* L = model->L;
	if (!L)
		return DataVariable{};
	int ref = (int)(intptr_t)ptr;

	PushValue(model, ref);
	// -1 = this table

	struct TableProxy* proxy = ToTableProxy(L, -1);

	if (proxy == nullptr || proxy->model != model)
	{
		lua_pop(L, 1);
		return DataVariable{};
	}

	if (address.index == -1)
		lua_pushlstring(L, address.name.data(), address.name.size());
	else
		lua_pushinteger(L, (lua_Integer)address.index + 1);
	// -1 = key, -2 = this table

	int child_ref = ChildRef(model, proxy->table_ref);

	if (child_ref != LUA_REFNIL)
	{
		lua_pop(L, 2);
		return DataVariable(model->objectDef, (void*)(intptr_t)child_ref);
	}

	lua_pushvalue(L, -1);
	// -1 = key, -2 = key, -3 = this table
	lua_gettable(L, -3);
	// -1 = value, -2 = key, -3 = this table
	lua_remove(L, -3);
	// -1 = value, -2 = key

	if (lua_type(L, -1) == LUA_TTABLE)
		NewTableProxy(model, proxy->key_ref);
	else if (lua_isnil(L, -1))
	{
		lua_pop(L, 2);
		return DataVariable{};
	}

	lua_rawgeti(L, LUA_REGISTRYINDEX, model->valuestore_ref);
	// -1 = valuestore, -2 = value, -3 = key
	lua_pushvalue(L, -2);
	// -1 = value, -2 = valuestore, -3 = value, -4 = key
	child_ref = luaL_ref(L, -2);
	// -1 = valuestore, -2 = value, -3 = key
	lua_pop(L, 2);
	// -1 = key

	lua_rawgeti(L, LUA_REGISTRYINDEX, model->tablestore_ref);
	// -1 = tablestore, -2 = key
	lua_rawgeti(L, -1, proxy->table_ref);
	// -1 = tablestore entry / nil, -2 = tablestore, -3 = key

	if (lua_type(L, -1) != LUA_TTABLE)
	{
		lua_pop(L, 1);
		// -1 = tablestore, -2 = key
		lua_newtable(L);
		// -1 = tablestore entry, -2 = tablestore, -3 = key
		lua_pushvalue(L, -1);
		// -1 = tablestore entry, -2 = tablestore entry, -3 = tablestore, -4 = key
		lua_rawseti(L, -3, proxy->table_ref);
		// -1 = tablestore entry, -2 = tablestore, -3 = key
	}

	lua_replace(L, -2);
	// -1 = tablestore entry, -2 = key

	lua_pushvalue(L, -2);
	// -1 = key, -2 = tablestore entry, -3 = key
	lua_pushnumber(L, child_ref);
	// -1 = child ref, -2 = key, -3 = tablestore entry, -4 = key
	lua_rawset(L, -3);
	// -1 = tablestore entry, -2 = key
	lua_pop(L, 2);

	return DataVariable(model->objectDef, (void*)(intptr_t)child_ref);
}

static int TableProxyGet(lua_State* L)
{
	// 1 = table proxy, 2 = key
	struct TableProxy* proxy = (struct TableProxy*)lua_touserdata(L, 1);

	lua_rawgeti(L, LUA_REGISTRYINDEX, proxy->model->tablestore_ref);
	// -1 = tablestore
	lua_rawgeti(L, -1, proxy->table_ref);
	// -1 = tablestore entry, -2 = tablestore
	lua_replace(L, -2);
	// -1 = tablestore entry
	lua_pushvalue(L, 2);
	// -1 = key, -2 = tablestore entry
	lua_rawget(L, -2);
	// -1 = child ref / nil, -2 = tablestore entry

	int child_ref = LUA_REFNIL;

	if (lua_isnil(L, -1))
	{
		lua_pop(L, 1);
		// -1 = tablestore entry
		lua_rawgeti(L, LUA_REGISTRYINDEX, proxy->model->valuestore_ref);
		// -1 = valuestore, -2 = tablestore entry
		lua_rawgeti(L, -1, proxy->table_ref);
		// -1 = table, -2 = valuestore, -3 = tablestore entry
		lua_replace(L, -2);
		// -1 = table, -2 = tablestore entry
		lua_pushvalue(L, 2);
		// -1 = key, -2 = table, -3 = tablestore entry
		lua_gettable(L, -2);
		// -1 = value, -2 = table, -3 = tablestore entry
		lua_replace(L, -2);
		// -1 = value, -2 = tablestore entry
		if (lua_isnil(L, -1))
			return 1; // Just return nil; avoid setting LUA_REFNIL

		lua_rawgeti(L, LUA_REGISTRYINDEX, proxy->model->valuestore_ref);
		// -1 = valuestore, -2 = value, -3 = tablestore entry
		lua_pushvalue(L, -2);
		// -1 = value, -2 = valuestore, -3 = value, -4 = tablestore entry
		child_ref = luaL_ref(L, -2);
		// -1 = valuestore, -2 = value, -3 = tablestore entry
		lua_pushvalue(L, 2);
		// -1 = key, -2 = valuestore, -3 = value, -4 = tablestore entry
		lua_replace(L, -2);
		// -1 = key, -2 = value, -3 = tablestore entry
		lua_pushnumber(L, child_ref);
		// -1 = child ref, -2 = key, -3 = value, -4 = tablestore entry
		lua_rawset(L, -4);
		// -1 = value, -2 = tablestore entry
		lua_replace(L, -2);
		// -1 = value
	}
	else
	{
		child_ref = (int)lua_tonumber(L, -1);
		// -1 = child ref, -2 = tablestore entry
		lua_pop(L, 2);

		lua_rawgeti(L, LUA_REGISTRYINDEX, proxy->model->valuestore_ref);
		// -1 = valuestore
		lua_rawgeti(L, -1, child_ref);
		// -1 = value, -2 = valuestore
		lua_replace(L, -2);
		// -1 = value
	}

	if (lua_type(L, -1) == LUA_TTABLE)
	{
		NewTableProxy(proxy->model, proxy->key_ref);
		// -1 = value
		lua_pushvalue(L, -1);
		// -1 = value, -2 = value
		SetValue(proxy->model, child_ref);
		// -1 = value
	}

	return 1;
}

static int TableProxySet(lua_State* L)
{
	// 1 = table proxy, 2 = key, 3 = value
	struct TableProxy* proxy = (struct TableProxy*)lua_touserdata(L, 1);

	struct TableProxy* value_proxy = ToTableProxy(L, 3);
	if (value_proxy != nullptr)
	{
		PushValue(proxy->model, value_proxy->table_ref);
		lua_replace(L, 3);
	}

	lua_rawgeti(L, LUA_REGISTRYINDEX, proxy->model->valuestore_ref);
	// -1 = valuestore
	lua_rawgeti(L, LUA_REGISTRYINDEX, proxy->model->tablestore_ref);
	// -1 = tablestore, -2 = valuestore
	lua_rawgeti(L, -1, proxy->table_ref);
	// -1 = tablestore entry, -2 = tablestore, -3 = valuestore
	lua_replace(L, -2);
	// -1 = tablestore entry, -2 = valuestore
	lua_pushvalue(L, 2);
	// -1 = key, -2 = tablestore entry, -3 = valuestore
	lua_rawget(L, -2);
	// -1 = child ref, -2 = tablestore entry, -3 = valuestore

	if (lua_isnil(L, -1))
	{
		if (lua_isnil(L, 3))
			return 0;

		lua_pop(L, 1);
		// -1 = tablestore entry, -2 = valuestore
		lua_pushvalue(L, 3);
		// -1 = value, -2 = tablestore entry, -3 = valuestore
		if (lua_type(L, -1) == LUA_TTABLE)
			NewTableProxy(proxy->model, proxy->key_ref);
		int child_ref = luaL_ref(L, -3);
		// -1 = tablestore entry, -2 = valuestore
		lua_pushvalue(L, 2);
		// -1 = key, -2 = tablestore entry, -3 = valuestore
		lua_pushnumber(L, child_ref);
		// -1 = child ref, -2 = key, -3 = tablestore entry, -4 = valuestore
		lua_rawset(L, -3);
		// -1 = tablestore entry, -2 = valuestore
		return 0;
	}

	int child_ref = (int)lua_tonumber(L, -1);
	lua_pop(L, 3);

	PushValue(proxy->model, child_ref);
	// -1 = prev value

	if (lua_rawequal(L, -1, 3))
		return 0;

	lua_pop(L, 1);

	lua_pushvalue(L, 3);
	// -1 = value
	if (lua_type(L, -1) == LUA_TTABLE)
		NewTableProxy(proxy->model, proxy->key_ref);
	SetValue(proxy->model, child_ref);

	lua_rawgeti(L, LUA_REGISTRYINDEX, proxy->model->keystore_ref);
	// -1 = keystore
	lua_rawgeti(L, -1, proxy->key_ref);
	// -1 = root key, -2 = keystore
	proxy->model->handle.DirtyVariable(lua_tostring(L, -1));
	lua_pop(L, 2);

	return 0;
}

static int TableProxyGc(lua_State* L)
{
	struct TableProxy* proxy = (struct TableProxy*)lua_touserdata(L, 1);

	if (proxy->model->valuestore_ref != LUA_NOREF)
	{
		lua_rawgeti(L, LUA_REGISTRYINDEX, proxy->model->valuestore_ref);
		lua_pushnil(L);
		lua_rawseti(L, -2, proxy->table_ref);
		lua_pop(L, 1);
	}

	if (proxy->model->tablestore_ref != LUA_NOREF)
	{
		lua_rawgeti(L, LUA_REGISTRYINDEX, proxy->model->tablestore_ref);
		lua_pushnil(L);
		lua_rawseti(L, -2, proxy->table_ref);
		lua_pop(L, 1);
	}

	proxy->table_ref = LUA_NOREF;

	return 0;
}

static struct TableProxy* NewTableProxy(struct LuaDataModel* model, int key_ref)
{
	// -1 = table
	lua_State* L = model->L;

	lua_rawgeti(L, LUA_REGISTRYINDEX, model->valuestore_ref);
	lua_pushvalue(L, -2);
	int table_ref = luaL_ref(L, -2);
	lua_pop(L, 2);

	lua_rawgeti(L, LUA_REGISTRYINDEX, model->tablestore_ref);
	lua_newtable(L);
	lua_rawseti(L, -2, table_ref);
	lua_pop(L, 1);

	struct TableProxy* proxy = (struct TableProxy*)lua_newuserdata(L, sizeof(*proxy));
	// -1 = proxy
	proxy->model = model;
	proxy->table_ref = table_ref;
	proxy->key_ref = key_ref;

	if (luaL_newmetatable(L, RMLTABLEPROXY))
	{
		luaL_Reg l[] = {
			{"__index", TableProxyGet},
			{"__newindex", TableProxySet},
			{"__gc", TableProxyGc},
			{nullptr, nullptr},
		};
		luaL_setfuncs(L, l, 0);
	}
	lua_setmetatable(L, -2);

	return proxy;
}

static void BindVariable(struct LuaDataModel* model)
{
	// -1 = value, -2 = key
	lua_State* L = model->L;

	struct TableProxy* value_proxy = ToTableProxy(L, -1);
	if (value_proxy != nullptr)
	{
		lua_pop(L, 1);
		PushValue(model, value_proxy->table_ref);
	}

	lua_rawgeti(L, LUA_REGISTRYINDEX, model->valuestore_ref);
	// -1 = valuestore, -2 = value, -3 = key
	lua_pushvalue(L, -2);
	// -1 = value, -2 = valuestore, -3 = value, -4 = key
	value_proxy = lua_type(L, -1) == LUA_TTABLE ? NewTableProxy(model, LUA_REFNIL) : nullptr;
	int ref = luaL_ref(L, -2);
	if (value_proxy != nullptr)
		value_proxy->key_ref = ref;
	// -1 = valuestore, -2 = value, -3 = key
	lua_pop(L, 1);
	// -1 = value, -2 = key

	lua_rawgeti(L, LUA_REGISTRYINDEX, model->tablestore_ref);
	// -1 = tablestore, -2 = value, -3 = key
	lua_rawgeti(L, -1, model->model_ref);
	// -1 = tablestore entry, -2 = tablestore, -2 = value, -3 = key
	lua_replace(L, -2);
	// -1 = tablestore entry, -2 = value, -3 = key
	lua_pushvalue(L, -3);
	// -1 = key, -2 = tablestore entry, -3 = value, -4 = key
	lua_pushnumber(L, ref);
	// -1 = ref, -2 = key, -3 = tablestore entry, -4 = value, -5 = key
	lua_rawset(L, -3);
	// -1 = tablestore entry, -2 = value, -3 = key
	lua_pop(L, 1);
	// -1 = value, -2 = key

	lua_rawgeti(L, LUA_REGISTRYINDEX, model->keystore_ref);
	// -1 = keystore, -2 = value, -3 = key
	lua_pushvalue(L, -3);
	// -1 = key, -2 = keystore, -3 = value, -4 = key
	lua_rawseti(L, -2, ref);
	// -1 = keystore, -2 = value, -3 = key
	lua_pop(L, 1);
	// -1 = value, -2 = key

	// Root data model variable names are assumed to be strings. But lua_tostring
	// actually converts the value on the stack to a string, which can cause a
	// problem for lua_next. So call it on a copy, just to be safe.
	lua_pushvalue(L, -2);
	const char* key = lua_tostring(L, -1);
	lua_pop(L, 1);

	if (lua_type(L, -1) == LUA_TFUNCTION)
	{
		model->constructor.BindEventCallback(key, [=](DataModelHandle, Event& event, const VariantList& varlist) {
			PushValue(model, ref);
			luabind::invoke(
				L,
				[&]() {
					LuaType<Event>::push(L, &event, false);
					for (const auto& variant : varlist)
					{
						PushVariant(L, &variant);
					}
					lua_call(L, (int)varlist.size() + 1, 0);
				},
				1);
		});
	}
	else
	{
		model->constructor.BindCustomDataVariable(key, DataVariable(model->objectDef, (void*)(intptr_t)ref));
	}
}

static int lDataModelGet(lua_State* L)
{
	// 1 = data model, 2 = key
	struct LuaDataModel* model = (struct LuaDataModel*)lua_touserdata(L, 1);
	if (model->model_ref == LUA_NOREF)
	{
		luaL_error(L, "DataModel closed");
		return 0;
	}

	lua_pushvalue(L, 2);
	int child_ref = ChildRef(model, model->model_ref);
	lua_pop(L, 1);

	if (child_ref == LUA_REFNIL)
	{
		lua_pushnil(L);
		return 1;
	}

	lua_rawgeti(L, LUA_REGISTRYINDEX, model->valuestore_ref);
	lua_rawgeti(L, -1, child_ref);

	if (lua_type(L, -1) == LUA_TTABLE)
	{
		NewTableProxy(model, child_ref);
		lua_pushvalue(L, -1);
		lua_rawseti(L, -3, child_ref);
	}

	return 1;
}

static int lDataModelSet(lua_State* L)
{
	// 1 = data model, 2 = key, 3 = value
	struct LuaDataModel* model = (struct LuaDataModel*)lua_touserdata(L, 1);
	if (model->model_ref == LUA_NOREF)
	{
		luaL_error(L, "DataModel closed");
		return 0;
	}

	lua_pushvalue(L, 2);
	int child_ref = ChildRef(model, model->model_ref);
	lua_pop(L, 1);

	if (child_ref == LUA_REFNIL)
	{
		BindVariable(model);
		return 0;
	}

	lua_pushvalue(L, 3);
	struct TableProxy* value_proxy = ToTableProxy(L, -1);
	if (value_proxy != nullptr)
	{
		lua_pop(L, 1);
		PushValue(model, value_proxy->table_ref);
	}

	if (lua_type(L, -1) == LUA_TTABLE)
		NewTableProxy(model, child_ref);
	SetValue(model, child_ref);

	lua_pushvalue(L, 2);
	model->handle.DirtyVariable(lua_tostring(L, -1));
	lua_pop(L, 1);

	return 0;
}

void PushDataModelsTable(lua_State* L)
{
	static constexpr auto datamodels_key = "datamodels";

	lua_pushstring(L, datamodels_key);
	lua_rawget(L, LUA_REGISTRYINDEX);

	if (lua_type(L, -1) == LUA_TTABLE)
		return;

	lua_pop(L, 1);
	lua_newtable(L);
	lua_pushstring(L, datamodels_key);
	lua_pushvalue(L, -2);
	lua_rawset(L, LUA_REGISTRYINDEX);
}

bool OpenLuaDataModel(lua_State* L, Context* context, int name_index, int table_index)
{
	String name = luaL_checkstring(L, name_index);
	luaL_checktype(L, table_index, LUA_TTABLE);

	DataModelConstructor constructor = context->CreateDataModel(name);
	if (!constructor)
	{
		constructor = context->GetDataModel(name);
		if (!constructor)
		{
			return false;
		}
	}

	struct LuaDataModel* model = (struct LuaDataModel*)lua_newuserdata(L, sizeof(*model));
	model->L = L;
	model->constructor = constructor;
	model->handle = constructor.GetModelHandle();
	model->objectDef = new LuaObjectDef(model);

	lua_newtable(L);
	lua_pushvalue(L, -2);
	model->model_ref = luaL_ref(L, -2);
	model->valuestore_ref = luaL_ref(L, LUA_REGISTRYINDEX);

	lua_newtable(L);
	lua_newtable(L);
	lua_rawseti(L, -2, model->model_ref);
	model->tablestore_ref = luaL_ref(L, LUA_REGISTRYINDEX);

	lua_newtable(L);
	model->keystore_ref = luaL_ref(L, LUA_REGISTRYINDEX);

	lua_pushnil(L);
	while (lua_next(L, table_index) != 0)
	{
		BindVariable(model);
		lua_pop(L, 1);
	}

	if (luaL_newmetatable(L, RMLDATAMODEL))
	{
		luaL_Reg l[] = {
			{"__index", lDataModelGet},
			{"__newindex", lDataModelSet},
			{nullptr, nullptr},
		};
		luaL_setfuncs(L, l, 0);
	}
	lua_setmetatable(L, -2);
	// -1 = data model

	PushDataModelsTable(L);
	// -1 = data models table, -2 = data model
	lua_pushvalue(L, name_index);
	// -1 = name, -2 = data models table, -3 = data model
	lua_pushvalue(L, -3);
	// -1 = data model, -2 = name, -3 = data models table, -4 = data model
	lua_rawset(L, -3);
	// -1 = data models table, -2 = data model
	lua_pop(L, 1);
	// -1 = data model

	return true;
}

void CloseLuaDataModel(lua_State* L, Context* context, int name_index)
{
	String name = luaL_checkstring(L, name_index);

	PushDataModelsTable(L);
	// -1 = data models table
	lua_pushvalue(L, name_index);
	// -1 = name, -2 = data models table
	lua_rawget(L, -2);
	// -1 = data model / nil, -2 = data models table
	struct LuaDataModel* model = (struct LuaDataModel*)luaL_testudata(L, -1, RMLDATAMODEL);
	lua_pop(L, 2);

	if (model == nullptr)
		return;

	lua_pushnil(L);
	lua_rawseti(L, LUA_REGISTRYINDEX, model->valuestore_ref);
	model->valuestore_ref = LUA_NOREF;

	lua_pushnil(L);
	lua_rawseti(L, LUA_REGISTRYINDEX, model->tablestore_ref);
	model->tablestore_ref = LUA_NOREF;

	lua_pushnil(L);
	lua_rawseti(L, LUA_REGISTRYINDEX, model->keystore_ref);
	model->keystore_ref = LUA_NOREF;

	model->L = nullptr;
	delete model->objectDef;
	model->objectDef = nullptr;

	model->model_ref = LUA_NOREF;

	PushDataModelsTable(L);
	// -1 = data models table
	lua_pushvalue(L, name_index);
	// -1 = name, -2 = data models table
	lua_pushnil(L);
	// -1 = nil, -2 = name, -3 = data models table
	lua_rawset(L, -3);
	// -1 = data models table
	lua_pop(L, 1);

	context->RemoveDataModel(name);
}

} // namespace Lua
} // namespace Rml
