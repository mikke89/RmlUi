#include "../../Include/RmlUi/Debugger/Debugger.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "DebuggerPlugin.h"

namespace Rml {
namespace Debugger {

bool Initialise(Context* context)
{
	if (DebuggerPlugin::GetInstance() != nullptr)
	{
		Log::Message(Log::LT_WARNING, "Unable to initialise debugger plugin, already initialised!");
		return false;
	}

	DebuggerPlugin* plugin = new DebuggerPlugin();
	if (!plugin->Initialise(context))
	{
		Log::Message(Log::LT_WARNING, "Unable to initialise debugger plugin.");

		delete plugin;
		return false;
	}

	SetContext(context);
	RegisterPlugin(plugin);

	return true;
}

void Shutdown()
{
	DebuggerPlugin* plugin = DebuggerPlugin::GetInstance();
	if (!plugin)
	{
		Log::Message(Log::LT_WARNING, "Unable to shutdown debugger plugin, it was not initialised!");
		return;
	}

	UnregisterPlugin(plugin);
}

bool SetContext(Context* context)
{
	DebuggerPlugin* plugin = DebuggerPlugin::GetInstance();
	if (plugin == nullptr)
		return false;

	plugin->SetContext(context);

	return true;
}

void SetVisible(bool visibility)
{
	DebuggerPlugin* plugin = DebuggerPlugin::GetInstance();
	if (plugin != nullptr)
		plugin->SetVisible(visibility);
}

bool IsVisible()
{
	DebuggerPlugin* plugin = DebuggerPlugin::GetInstance();
	if (plugin == nullptr)
		return false;

	return plugin->IsVisible();
}

} // namespace Debugger
} // namespace Rml
