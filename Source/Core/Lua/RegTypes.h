#pragma once
#include "LuaType.h"
//Macro to eliminate a bunch of repetitive typing. It declares all of the templated functions that are needed to
//bind a C++ class to Lua. You have to actually implement the functions later on.


namespace Rocket {
namespace Core {
namespace Lua {

    ROCKETREGTYPE(rocket)
}
}
}