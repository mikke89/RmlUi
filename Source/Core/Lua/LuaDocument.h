#pragma once
/*
    This class is an ElementDocument that overrides the LoadScript function
*/
#include <Rocket/Core/ElementDocument.h>

namespace Rocket {
namespace Core {
namespace Lua {

class LuaDocument : public ElementDocument
{
public:
    LuaDocument(const String& tag);
    virtual void LoadScript(Stream& stream, const String& source_name);
};

}
}
}