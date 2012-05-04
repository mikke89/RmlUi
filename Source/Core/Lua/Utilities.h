#pragma once
/*
    This file is for free-floating functions that are used across more than one file.
*/
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Core/Variant.h>

namespace Rocket {
namespace Core {
namespace Lua {

//casts the variant to its specific type before pushing it to the stack
void PushVariant(lua_State* L, Variant* var);

}
}
}
