#pragma once

#include <RmlUi/Core/Types.h>

Rml::String LoadFile(const Rml::String& path);
bool SaveFile(const Rml::String& path, const Rml::String& content);

Rml::UnorderedMap<Rml::String, Rml::String> LoadOptions(const Rml::String& path);
void SaveOptions(const Rml::String& path, const Rml::UnorderedMap<Rml::String, Rml::String>& options);
