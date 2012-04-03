#include "precompiled.h"
#include "LuaDocument.h"
#include "lua.hpp"
#include "Interpreter.h"


namespace Rocket {
namespace Core {
namespace Lua {

LuaDocument::LuaDocument(const String& tag) : ElementDocument(tag)
{
}

void LuaDocument::LoadScript(Stream* stream, const String& source_name)
{
    //if it is loaded from a file
    if(source_name != "")
    {
        Interpreter::LoadFile(source_name);
    }
    else
    {
        String buffer = "";
        stream.Read(buffer,stream.Length()); //just do the whole thing
        Interpreter::DoString(buffer);
    }
}

}
}
}