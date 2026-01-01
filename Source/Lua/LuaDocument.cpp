#include "LuaDocument.h"
#include <RmlUi/Core/Stream.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/Interpreter.h>

namespace Rml {
namespace Lua {

LuaDocument::LuaDocument(const String& tag) : ElementDocument(tag) {}

void LuaDocument::LoadInlineScript(const String& context, const String& source_path, int source_line)
{
	String buffer;
	buffer += "--";
	buffer += source_path;
	buffer += ":";
	buffer += Rml::ToString(source_line);
	buffer += "\n";
	buffer += context;
	Interpreter::DoString(buffer, buffer);
}

void LuaDocument::LoadExternalScript(const String& source_path)
{
	Interpreter::LoadFile(source_path);
}

} // namespace Lua
} // namespace Rml
