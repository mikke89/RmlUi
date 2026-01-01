#pragma once

typedef struct lua_State lua_State;

namespace Rml {
namespace Lua {
void OverrideLuaGlobalFunctions(lua_State* L);
// overrides pairs and ipairs to respect __pairs and __ipairs metamethods
// overrdes print to print to the console
} // namespace Lua
} // namespace Rml
