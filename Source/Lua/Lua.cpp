#include "LuaPlugin.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Lua/Lua.h>

namespace Rml {
namespace Lua {

void Initialise()
{
	::Rml::Lua::Initialise(nullptr);
}

void Initialise(lua_State* lua_state, const Options& options)
{
	::Rml::RegisterPlugin(new LuaPlugin(lua_state, options));
}

} // namespace Lua
} // namespace Rml
