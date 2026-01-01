#include "Context.h"
#include "ContextDocumentsProxy.h"
#include "LuaDataModel.h"
#include "LuaEventListener.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Factory.h>

namespace Rml {
namespace Lua {
typedef ElementDocument Document;
template <>
void ExtraInit<Context>(lua_State* /*L*/, int /*metatable_index*/)
{
	return;
}

// methods
int ContextAddEventListener(lua_State* L, Context* obj)
{
	// need to make an EventListener for Lua before I can do anything else
	const char* evt = luaL_checkstring(L, 1); // event
	Element* element = nullptr;
	bool capturephase = false;
	// get the rest of the stuff needed to construct the listener
	if (lua_gettop(L) > 2)
	{
		if (!lua_isnoneornil(L, 3))
			element = LuaType<Element>::check(L, 3);
		if (!lua_isnoneornil(L, 4))
			capturephase = RMLUI_CHECK_BOOL(L, 4);
	}
	int type = lua_type(L, 2);
	if (type == LUA_TFUNCTION)
	{
		if (element)
			element->AddEventListener(evt, new LuaEventListener(L, 2, element), capturephase);
		else
			obj->AddEventListener(evt, new LuaEventListener(L, 2, nullptr), capturephase);
	}
	else if (type == LUA_TSTRING)
	{
		if (element)
			element->AddEventListener(evt, new LuaEventListener(luaL_checkstring(L, 2), element), capturephase);
		else
			obj->AddEventListener(evt, new LuaEventListener(luaL_checkstring(L, 2), nullptr), capturephase);
	}
	else
	{
		Log::Message(Log::LT_WARNING, "Lua Context:AddEventLisener's 2nd argument can only be a Lua function or a string, you passed in a %s",
			lua_typename(L, type));
	}
	return 0;
}

int ContextCreateDocument(lua_State* L, Context* obj)
{
	const char* tag;
	if (lua_gettop(L) < 1)
		tag = "body";
	else
		tag = luaL_checkstring(L, 1);
	Document* doc = obj->CreateDocument(tag);
	LuaType<Document>::push(L, doc, false);
	return 1;
}

int ContextLoadDocument(lua_State* L, Context* obj)
{
	const char* path = luaL_checkstring(L, 1);
	Document* doc = obj->LoadDocument(path);
	LuaType<Document>::push(L, doc, false);
	return 1;
}

int ContextRender(lua_State* L, Context* obj)
{
	lua_pushboolean(L, obj->Render());
	return 1;
}

int ContextUnloadAllDocuments(lua_State* /*L*/, Context* obj)
{
	obj->UnloadAllDocuments();
	return 0;
}

int ContextUnloadDocument(lua_State* L, Context* obj)
{
	Document* doc = LuaType<Document>::check(L, 1);
	obj->UnloadDocument(doc);
	return 0;
}

int ContextUpdate(lua_State* L, Context* obj)
{
	lua_pushboolean(L, obj->Update());
	return 1;
}

int ContextOpenDataModel(lua_State* L, Context* obj)
{
	if (!OpenLuaDataModel(L, obj, 1, 2))
	{
		// Open fails
		lua_pushboolean(L, false);
	}
	return 1;
}

// input
int ContextProcessMouseMove(lua_State* L, Context* obj)
{
	int x = (int)luaL_checkinteger(L, 1);
	int y = (int)luaL_checkinteger(L, 2);
	int flags = (int)luaL_checkinteger(L, 3);
	lua_pushboolean(L, obj->ProcessMouseMove(x, y, flags));
	return 1;
}

int ContextProcessMouseButtonDown(lua_State* L, Context* obj)
{
	int button_index = (int)luaL_checkinteger(L, 1);
	int key_modifier_state = (int)luaL_checkinteger(L, 2);
	lua_pushboolean(L, obj->ProcessMouseButtonDown(button_index, key_modifier_state));
	return 1;
}

int ContextProcessMouseButtonUp(lua_State* L, Context* obj)
{
	int button_index = (int)luaL_checkinteger(L, 1);
	int key_modifier_state = (int)luaL_checkinteger(L, 2);
	lua_pushboolean(L, obj->ProcessMouseButtonUp(button_index, key_modifier_state));
	return 1;
}

int ContextProcessMouseWheel(lua_State* L, Context* obj)
{
	float wheel_delta = (float)luaL_checknumber(L, 1);
	int key_modifier_state = (int)luaL_checkinteger(L, 2);
	lua_pushboolean(L, obj->ProcessMouseWheel(wheel_delta, key_modifier_state));
	return 1;
}

int ContextProcessMouseLeave(lua_State* L, Context* obj)
{
	lua_pushboolean(L, obj->ProcessMouseLeave());
	return 1;
}

int ContextIsMouseInteracting(lua_State* L, Context* obj)
{
	lua_pushboolean(L, obj->IsMouseInteracting());
	return 1;
}

int ContextProcessKeyDown(lua_State* L, Context* obj)
{
	Rml::Input::KeyIdentifier key_identifier = (Rml::Input::KeyIdentifier)luaL_checkinteger(L, 1);
	int key_modifier_state = (int)luaL_checkinteger(L, 2);
	lua_pushboolean(L, obj->ProcessKeyDown(key_identifier, key_modifier_state));
	return 1;
}

int ContextProcessKeyUp(lua_State* L, Context* obj)
{
	Rml::Input::KeyIdentifier key_identifier = (Rml::Input::KeyIdentifier)luaL_checkinteger(L, 1);
	int key_modifier_state = (int)luaL_checkinteger(L, 2);
	lua_pushboolean(L, obj->ProcessKeyUp(key_identifier, key_modifier_state));
	return 1;
}

int ContextProcessTextInput(lua_State* L, Context* obj)
{
	const char* text = NULL;
	int character = -1;

	if (lua_isstring(L, 1))
		text = lua_tostring(L, 1);
	else
		character = (int)luaL_checkinteger(L, 1);

	if (character > 0)
		lua_pushboolean(L, obj->ProcessTextInput((char)character));
	else if (text != NULL)
		lua_pushboolean(L, obj->ProcessTextInput(text));
	else
		Log::Message(Log::LT_WARNING, "Could not process text input on context '%s'.", obj->GetName().c_str());

	return 1;
}

// getters
int ContextGetAttrdimensions(lua_State* L)
{
	Context* cont = LuaType<Context>::check(L, 1);
	Vector2i* dim = new Vector2i(cont->GetDimensions());
	LuaType<Vector2i>::push(L, dim, true);
	return 1;
}

// returns a table of everything
int ContextGetAttrdocuments(lua_State* L)
{
	Context* cont = LuaType<Context>::check(L, 1);
	RMLUI_CHECK_OBJ(cont);
	ContextDocumentsProxy* cdp = new ContextDocumentsProxy();
	cdp->owner = cont;
	LuaType<ContextDocumentsProxy>::push(L, cdp, true); // does get garbage collected (deleted)
	return 1;
}

int ContextGetAttrdp_ratio(lua_State* L)
{
	Context* cont = LuaType<Context>::check(L, 1);
	float dp_ratio = cont->GetDensityIndependentPixelRatio();
	lua_pushnumber(L, dp_ratio);
	return 1;
}

int ContextGetAttrfocus_element(lua_State* L)
{
	Context* cont = LuaType<Context>::check(L, 1);
	RMLUI_CHECK_OBJ(cont);
	LuaType<Element>::push(L, cont->GetFocusElement());
	return 1;
}

int ContextGetAttrhover_element(lua_State* L)
{
	Context* cont = LuaType<Context>::check(L, 1);
	RMLUI_CHECK_OBJ(cont);
	LuaType<Element>::push(L, cont->GetHoverElement());
	return 1;
}

int ContextGetAttrname(lua_State* L)
{
	Context* cont = LuaType<Context>::check(L, 1);
	RMLUI_CHECK_OBJ(cont);
	lua_pushstring(L, cont->GetName().c_str());
	return 1;
}

int ContextGetAttrroot_element(lua_State* L)
{
	Context* cont = LuaType<Context>::check(L, 1);
	RMLUI_CHECK_OBJ(cont);
	LuaType<Element>::push(L, cont->GetRootElement());
	return 1;
}

// setters
int ContextSetAttrdimensions(lua_State* L)
{
	Context* cont = LuaType<Context>::check(L, 1);
	RMLUI_CHECK_OBJ(cont);
	Vector2i* dim = LuaType<Vector2i>::check(L, 2);
	cont->SetDimensions(*dim);
	return 0;
}

int ContextSetAttrdp_ratio(lua_State* L)
{
	Context* cont = LuaType<Context>::check(L, 1);
	RMLUI_CHECK_OBJ(cont);
	lua_Number dp_ratio = luaL_checknumber(L, 2);
	cont->SetDensityIndependentPixelRatio((float)dp_ratio);
	return 0;
}

RegType<Context> ContextMethods[] = {
	RMLUI_LUAMETHOD(Context, AddEventListener),
	RMLUI_LUAMETHOD(Context, CreateDocument),
	RMLUI_LUAMETHOD(Context, LoadDocument),
	RMLUI_LUAMETHOD(Context, Render),
	RMLUI_LUAMETHOD(Context, UnloadAllDocuments),
	RMLUI_LUAMETHOD(Context, UnloadDocument),
	RMLUI_LUAMETHOD(Context, Update),
	RMLUI_LUAMETHOD(Context, OpenDataModel),
	// todo: CloseDataModel
	RMLUI_LUAMETHOD(Context, ProcessMouseMove),
	RMLUI_LUAMETHOD(Context, ProcessMouseButtonDown),
	RMLUI_LUAMETHOD(Context, ProcessMouseButtonUp),
	RMLUI_LUAMETHOD(Context, ProcessMouseWheel),
	RMLUI_LUAMETHOD(Context, ProcessMouseLeave),
	RMLUI_LUAMETHOD(Context, IsMouseInteracting),
	RMLUI_LUAMETHOD(Context, ProcessKeyDown),
	RMLUI_LUAMETHOD(Context, ProcessKeyUp),
	RMLUI_LUAMETHOD(Context, ProcessTextInput),
	{nullptr, nullptr},
};

luaL_Reg ContextGetters[] = {
	RMLUI_LUAGETTER(Context, dimensions),
	RMLUI_LUAGETTER(Context, documents),
	RMLUI_LUAGETTER(Context, dp_ratio),
	RMLUI_LUAGETTER(Context, focus_element),
	RMLUI_LUAGETTER(Context, hover_element),
	RMLUI_LUAGETTER(Context, name),
	RMLUI_LUAGETTER(Context, root_element),
	{nullptr, nullptr},
};

luaL_Reg ContextSetters[] = {
	RMLUI_LUASETTER(Context, dimensions),
	RMLUI_LUASETTER(Context, dp_ratio),
	{nullptr, nullptr},
};

RMLUI_LUATYPE_DEFINE(Context)
} // namespace Lua
} // namespace Rml
