#pragma once

#include <RmlUi/Core/Elements/ElementFormControlTextArea.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {

// methods
int ElementFormControlTextAreaSelect(lua_State* L, ElementFormControlTextArea* obj);
int ElementFormControlTextAreaSetSelection(lua_State* L, ElementFormControlTextArea* obj);
int ElementFormControlTextAreaGetSelection(lua_State* L, ElementFormControlTextArea* obj);

// getters
int ElementFormControlTextAreaGetAttrcols(lua_State* L);
int ElementFormControlTextAreaGetAttrmaxlength(lua_State* L);
int ElementFormControlTextAreaGetAttrrows(lua_State* L);
int ElementFormControlTextAreaGetAttrwordwrap(lua_State* L);

// setters
int ElementFormControlTextAreaSetAttrcols(lua_State* L);
int ElementFormControlTextAreaSetAttrmaxlength(lua_State* L);
int ElementFormControlTextAreaSetAttrrows(lua_State* L);
int ElementFormControlTextAreaSetAttrwordwrap(lua_State* L);

extern RegType<ElementFormControlTextArea> ElementFormControlTextAreaMethods[];
extern luaL_Reg ElementFormControlTextAreaGetters[];
extern luaL_Reg ElementFormControlTextAreaSetters[];

// inherits from ElementFormControl which inherits from Element
template <>
void ExtraInit<ElementFormControlTextArea>(lua_State* L, int metatable_index);
RMLUI_LUATYPE_DECLARE(ElementFormControlTextArea)
} // namespace Lua
} // namespace Rml
