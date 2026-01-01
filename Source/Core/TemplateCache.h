#pragma once

#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class Template;

/**
    Manages requests for loading templates, caching as it goes.
 */

class TemplateCache {
public:
	/// Initialisation and Shutdown
	static bool Initialise();
	static void Shutdown();

	/// Load the named template from the given path, if its already loaded get the cached copy
	static Template* LoadTemplate(const String& path);
	/// Get the template by id
	static Template* GetTemplate(const String& id);

	/// Clear the template cache.
	static void Clear();

private:
	TemplateCache();
	~TemplateCache();

	using Templates = UnorderedMap<String, Template*>;
	Templates templates;
	Templates template_ids;
};

} // namespace Rml
