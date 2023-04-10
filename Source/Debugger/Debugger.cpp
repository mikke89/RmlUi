/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

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
