#ifndef ROCKETCORELUAELEMENTTEXT_H
#define ROCKETCORELUAELEMENTTEXT_H
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Core/ElementText.h>

namespace Rocket {
namespace Core {
namespace Lua {
//will inherit from Element
template<> void LuaType<ElementText>::extra_init(lua_State* L, int metatable_index);
template<> bool LuaType<ElementText>::is_reference_counted();

int ElementTextGetAttrtext(lua_State* L);
int ElementTextSetAttrtext(lua_State* L);

RegType<ElementText> ElementTextMethods[];
luaL_reg ElementTextGetters[];
luaL_reg ElementTextSetters[];

}
}
}
#endif