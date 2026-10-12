#include "PluginRegistry.h"
#include "../../Include/RmlUi/Core/Plugin.h"
#include "ControlledLifetimeResource.h"
#include <algorithm>

namespace Rml {

struct PluginVectors {
	Vector<Plugin*> basic;
	Vector<Plugin*> document;
	Vector<Plugin*> element;
	Vector<Plugin*> data_model;
};

static ControlledLifetimeResource<PluginVectors> plugin_vectors;

static void EnsurePluginVectorsInitialized()
{
	if (!plugin_vectors)
	{
		plugin_vectors.Initialize();
	}
}

void PluginRegistry::RegisterPlugin(Plugin* plugin)
{
	EnsurePluginVectorsInitialized();

	int event_classes = plugin->GetEventClasses();

	if (event_classes & Plugin::EVT_BASIC)
		plugin_vectors->basic.push_back(plugin);
	if (event_classes & Plugin::EVT_DOCUMENT)
		plugin_vectors->document.push_back(plugin);
	if (event_classes & Plugin::EVT_ELEMENT)
		plugin_vectors->element.push_back(plugin);
	if (event_classes & Plugin::EVT_DATA_MODEL)
		plugin_vectors->data_model.push_back(plugin);
}

void PluginRegistry::UnregisterPlugin(Plugin* plugin)
{
	// Safe to call after NotifyShutdown() (e.g. from a plugin destructor): the backing storage has been released
	// and there is nothing left to unregister.
	if (!plugin_vectors)
		return;

	auto erase_value = [](Vector<Plugin*>& container, Plugin* value) {
		container.erase(std::remove(container.begin(), container.end(), value), container.end());
	};

	int event_classes = plugin->GetEventClasses();
	if (event_classes & Plugin::EVT_BASIC)
		erase_value(plugin_vectors->basic, plugin);
	if (event_classes & Plugin::EVT_DOCUMENT)
		erase_value(plugin_vectors->document, plugin);
	if (event_classes & Plugin::EVT_ELEMENT)
		erase_value(plugin_vectors->element, plugin);
	if (event_classes & Plugin::EVT_DATA_MODEL)
		erase_value(plugin_vectors->data_model, plugin);
}

void PluginRegistry::NotifyInitialise()
{
	EnsurePluginVectorsInitialized();

	for (Plugin* plugin : plugin_vectors->basic)
		plugin->OnInitialise();
}

void PluginRegistry::NotifyShutdown()
{
	while (!plugin_vectors->basic.empty())
	{
		Plugin* plugin = plugin_vectors->basic.back();
		PluginRegistry::UnregisterPlugin(plugin);
		plugin->OnShutdown();
	}

	plugin_vectors.Shutdown();
}

void PluginRegistry::NotifyContextCreate(Context* context)
{
	for (Plugin* plugin : plugin_vectors->basic)
		plugin->OnContextCreate(context);
}

void PluginRegistry::NotifyContextDestroy(Context* context)
{
	for (Plugin* plugin : plugin_vectors->basic)
		plugin->OnContextDestroy(context);
}

void PluginRegistry::NotifyDocumentOpen(Context* context, const String& document_path)
{
	for (Plugin* plugin : plugin_vectors->document)
		plugin->OnDocumentOpen(context, document_path);
}

void PluginRegistry::NotifyDocumentLoad(ElementDocument* document)
{
	for (Plugin* plugin : plugin_vectors->document)
		plugin->OnDocumentLoad(document);
}

void PluginRegistry::NotifyDocumentUnload(ElementDocument* document)
{
	for (Plugin* plugin : plugin_vectors->document)
		plugin->OnDocumentUnload(document);
}

void PluginRegistry::NotifyElementCreate(Element* element)
{
	for (Plugin* plugin : plugin_vectors->element)
		plugin->OnElementCreate(element);
}

void PluginRegistry::NotifyElementDestroy(Element* element)
{
	for (Plugin* plugin : plugin_vectors->element)
		plugin->OnElementDestroy(element);
}

void PluginRegistry::NotifyDataModelCreate(Context* context, const String& name)
{
	for (Plugin* plugin : plugin_vectors->data_model)
		plugin->OnDataModelCreate(context, name);
}

void PluginRegistry::NotifyDataModelDestroy(Context* context, const String& name)
{
	for (Plugin* plugin : plugin_vectors->data_model)
		plugin->OnDataModelDestroy(context, name);
}

} // namespace Rml
