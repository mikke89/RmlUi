#include "precompiled.h"
#include "Interpreter.h"


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