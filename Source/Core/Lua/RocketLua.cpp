#include "precompiled.h"
#include <Rocket/Core/Lua/Interpreter.h>


namespace Rocket {
namespace Core {
namespace Lua {

void Initialise()
{
    Rocket::Core::RegisterPlugin(new Interpreter());
    
}

}
}
}