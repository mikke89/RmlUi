#include "precompiled.h"
#include "ElementForm.h"
#include <Rocket/Core/Element.h>
#include <Rocket/Controls/ElementForm.h>

namespace Rocket {
namespace Core {
namespace Lua {
//method
int ElementFormSubmit(lua_State* L, ElementForm* obj)
{
    const char* name = luaL_checkstring(L,1);
    const char* value = luaL_checkstring(L,2);
    obj->Submit(name,value);
    return 0;
}

RegType<ElementForm> ElementFormMethods[] =
{
    LUAMETHOD(ElementForm,Submit)
    { NULL, NULL },
};

luaL_reg ElementFormGetters[] =
{
    { NULL, NULL },
};

luaL_reg ElementFormSetters[] =
{
    { NULL, NULL },
};


}
}
}