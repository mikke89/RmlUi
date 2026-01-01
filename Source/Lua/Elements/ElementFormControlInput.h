#pragma once

#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {

// methods
int ElementFormControlInputSelect(lua_State* L, ElementFormControlInput* obj);
int ElementFormControlInputSetSelection(lua_State* L, ElementFormControlInput* obj);
int ElementFormControlInputGetSelection(lua_State* L, ElementFormControlInput* obj);

// getters
int ElementFormControlInputGetAttrchecked(lua_State* L);
int ElementFormControlInputGetAttrmaxlength(lua_State* L);
int ElementFormControlInputGetAttrsize(lua_State* L);
int ElementFormControlInputGetAttrmax(lua_State* L);
int ElementFormControlInputGetAttrmin(lua_State* L);
int ElementFormControlInputGetAttrstep(lua_State* L);

// setters
int ElementFormControlInputSetAttrchecked(lua_State* L);
int ElementFormControlInputSetAttrmaxlength(lua_State* L);
int ElementFormControlInputSetAttrsize(lua_State* L);
int ElementFormControlInputSetAttrmax(lua_State* L);
int ElementFormControlInputSetAttrmin(lua_State* L);
int ElementFormControlInputSetAttrstep(lua_State* L);

extern RegType<ElementFormControlInput> ElementFormControlInputMethods[];
extern luaL_Reg ElementFormControlInputGetters[];
extern luaL_Reg ElementFormControlInputSetters[];

// inherits from ElementFormControl which inherits from Element
template <>
void ExtraInit<ElementFormControlInput>(lua_State* L, int metatable_index);
RMLUI_LUATYPE_DECLARE(ElementFormControlInput)
} // namespace Lua
} // namespace Rml
