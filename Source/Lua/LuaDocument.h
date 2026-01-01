#pragma once
/*
    This class is an ElementDocument that overrides the LoadInlineScript and LoadExternalScript function
*/
#include <RmlUi/Core/ElementDocument.h>

namespace Rml {
namespace Lua {

class LuaDocument : public ::Rml::ElementDocument {
public:
	LuaDocument(const String& tag);
	void LoadInlineScript(const String& content, const String& source_path, int source_line) override;
	void LoadExternalScript(const String& source_path) override;
};

} // namespace Lua
} // namespace Rml
