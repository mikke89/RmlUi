#ifndef ROCKETCORELUALUADATAFORMATTER_H
#define ROCKETCORELUALUADATAFORMATTER_H
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Controls/DataFormatter.h>

using Rocket::Controls::DataFormatter;
namespace Rocket {
namespace Core {
namespace Lua {

class LuaDataFormatter : public Rocket::Controls::DataFormatter
{
public:
    LuaDataFormatter(const String& name = "");
    ~LuaDataFormatter();

    virtual void FormatData(Rocket::Core::String& formatted_data, const Rocket::Core::StringList& raw_data);

    //Helper function used to push on to the stack the table where the function ref should be stored
    static void PushDataFormatterFunctionTable(lua_State* L);

    int ref_FormatData; //the lua reference to the FormatData function
};

}
}
}
#endif
