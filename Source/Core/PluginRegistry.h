#pragma once

#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class Context;
class Element;
class ElementDocument;
class Plugin;

class PluginRegistry {
public:
	static void RegisterPlugin(Plugin* plugin);
	static void UnregisterPlugin(Plugin* plugin);

	/// Calls OnInitialise() on all plugins.
	static void NotifyInitialise();
	/// Calls OnShutdown() on all plugins.
	static void NotifyShutdown();

	/// Calls OnContextCreate() on all plugins.
	static void NotifyContextCreate(Context* context);
	/// Calls OnContextDestroy() on all plugins.
	static void NotifyContextDestroy(Context* context);

	/// Calls OnDocumentOpen() on all plugins.
	static void NotifyDocumentOpen(Context* context, const String& document_path);
	/// Calls OnDocumentLoad() on all plugins.
	static void NotifyDocumentLoad(ElementDocument* document);
	/// Calls OnDocumentUnload() on all plugins.
	static void NotifyDocumentUnload(ElementDocument* document);

	/// Calls OnElementCreate() on all plugins.
	static void NotifyElementCreate(Element* element);
	/// Calls OnElementDestroy() on all plugins.
	static void NotifyElementDestroy(Element* element);

private:
	PluginRegistry() = delete;
};

} // namespace Rml
