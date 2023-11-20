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

#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/Context.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/FileInterface.h"
#include "../../Include/RmlUi/Core/FontEngineInterface.h"
#include "../../Include/RmlUi/Core/Plugin.h"
#include "../../Include/RmlUi/Core/RenderInterface.h"
#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "../../Include/RmlUi/Core/SystemInterface.h"
#include "../../Include/RmlUi/Core/Types.h"
#include "EventSpecification.h"
#include "FileInterfaceDefault.h"
#include "GeometryDatabase.h"
#include "PluginRegistry.h"
#include "StyleSheetFactory.h"
#include "StyleSheetParser.h"
#include "TemplateCache.h"
#include "TextureDatabase.h"

#ifndef RMLUI_NO_FONT_INTERFACE_DEFAULT
	#include "FontEngineDefault/FontEngineInterfaceDefault.h"
#endif

#ifdef RMLUI_ENABLE_LOTTIE_PLUGIN
	#include "../Lottie/LottiePlugin.h"
#endif

#ifdef RMLUI_ENABLE_SVG_PLUGIN
	#include "../SVG/SVGPlugin.h"
#endif

#include "Pool.h"

namespace Rml {

// RmlUi's renderer interface.
static RenderInterface* render_interface = nullptr;
/// RmlUi's system interface.
static SystemInterface* system_interface = nullptr;
// RmlUi's file I/O interface.
static FileInterface* file_interface = nullptr;
// RmlUi's font engine interface.
static FontEngineInterface* font_interface = nullptr;

// Default interfaces should be created and destroyed on Initialise and Shutdown, respectively.
static UniquePtr<FileInterface> default_file_interface;
static UniquePtr<FontEngineInterface> default_font_interface;

static bool initialised = false;

using ContextMap = UnorderedMap<String, ContextPtr>;
static ContextMap contexts;

// The ObserverPtrBlock pool
extern Pool<ObserverPtrBlock>* observerPtrBlockPool;

#ifndef RMLUI_VERSION
	#define RMLUI_VERSION "custom"
#endif

bool Initialise()
{
	RMLUI_ASSERTMSG(!initialised, "Rml::Initialise() called, but RmlUi is already initialised!");

	Log::Initialise();

	// Check for valid interfaces, or install default interfaces as appropriate.
	if (!system_interface)
	{
		Log::Message(Log::LT_ERROR, "No system interface set!");
		return false;
	}
	if (!render_interface)
	{
		Log::Message(Log::LT_ERROR, "No render interface set!");
		return false;
	}

	if (!file_interface)
	{
#ifndef RMLUI_NO_FILE_INTERFACE_DEFAULT
		default_file_interface = MakeUnique<FileInterfaceDefault>();
		file_interface = default_file_interface.get();
#else
		Log::Message(Log::LT_ERROR, "No file interface set!");
		return false;
#endif
	}

	EventSpecificationInterface::Initialize();

	TextureDatabase::Initialise();

	if (!font_interface)
	{
#ifndef RMLUI_NO_FONT_INTERFACE_DEFAULT
		default_font_interface = MakeUnique<FontEngineInterfaceDefault>();
		font_interface = default_font_interface.get();
#else
		Log::Message(Log::LT_ERROR, "No font engine interface set!");
		return false;
#endif
	}

	StyleSheetSpecification::Initialise();
	StyleSheetParser::Initialise();
	StyleSheetFactory::Initialise();

	TemplateCache::Initialise();

	Factory::Initialise();

	// Initialise plugins integrated with Core.
#ifdef RMLUI_ENABLE_LOTTIE_PLUGIN
	Lottie::Initialise();
#endif
#ifdef RMLUI_ENABLE_SVG_PLUGIN
	SVG::Initialise();
#endif

	// Notify all plugins we're starting up.
	PluginRegistry::NotifyInitialise();

	initialised = true;

	return true;
}

void Shutdown()
{
	RMLUI_ASSERTMSG(initialised, "Rml::Shutdown() called, but RmlUi is not initialised!");

	// Clear out all contexts, which should also clean up all attached elements.
	contexts.clear();

	// Notify all plugins we're being shutdown.
	PluginRegistry::NotifyShutdown();

	Factory::Shutdown();
	TemplateCache::Shutdown();
	StyleSheetFactory::Shutdown();
	StyleSheetParser::Shutdown();
	StyleSheetSpecification::Shutdown();

	font_interface = nullptr;
	default_font_interface.reset();

	TextureDatabase::Shutdown();

	initialised = false;

	render_interface = nullptr;
	file_interface = nullptr;
	system_interface = nullptr;

	default_file_interface.reset();

	Log::Shutdown();

	// Release any memory pools
	ReleaseMemoryPools();
}

String GetVersion()
{
	return RMLUI_VERSION;
}

void SetSystemInterface(SystemInterface* _system_interface)
{
	system_interface = _system_interface;
}

SystemInterface* GetSystemInterface()
{
	return system_interface;
}

void SetRenderInterface(RenderInterface* _render_interface)
{
	if (initialised)
	{
		Log::Message(Log::LT_ERROR, "The render interface is not allowed to be set or changed after RmlUi has been initialised.");
		return;
	}

	render_interface = _render_interface;
}

RenderInterface* GetRenderInterface()
{
	return render_interface;
}

void SetFileInterface(FileInterface* _file_interface)
{
	file_interface = _file_interface;
}

FileInterface* GetFileInterface()
{
	return file_interface;
}

void SetFontEngineInterface(FontEngineInterface* _font_interface)
{
	font_interface = _font_interface;
}

FontEngineInterface* GetFontEngineInterface()
{
	return font_interface;
}

Context* CreateContext(const String& name, const Vector2i dimensions)
{
	if (!initialised)
		return nullptr;

	if (GetContext(name))
	{
		Log::Message(Log::LT_WARNING, "Failed to create context '%s', context already exists.", name.c_str());
		return nullptr;
	}

	ContextPtr new_context = Factory::InstanceContext(name);
	if (!new_context)
	{
		Log::Message(Log::LT_WARNING, "Failed to instance context '%s', instancer returned nullptr.", name.c_str());
		return nullptr;
	}

	new_context->SetDimensions(dimensions);

	Context* new_context_raw = new_context.get();
	contexts[name] = std::move(new_context);

	PluginRegistry::NotifyContextCreate(new_context_raw);

	return new_context_raw;
}

bool RemoveContext(const String& name)
{
	auto it = contexts.find(name);
	if (it != contexts.end())
	{
		contexts.erase(it);
		return true;
	}
	return false;
}

Context* GetContext(const String& name)
{
	ContextMap::iterator i = contexts.find(name);
	if (i == contexts.end())
		return nullptr;

	return i->second.get();
}

Context* GetContext(int index)
{
	ContextMap::iterator i = contexts.begin();
	int count = 0;

	if (index < 0 || index >= GetNumContexts())
		return nullptr;

	while (count < index)
	{
		++i;
		++count;
	}

	if (i == contexts.end())
		return nullptr;

	return i->second.get();
}

int GetNumContexts()
{
	return (int)contexts.size();
}

bool LoadFontFace(const String& file_path, bool fallback_face, Style::FontWeight weight)
{
	return font_interface->LoadFontFace(file_path, fallback_face, weight);
}

bool LoadFontFace(const byte* data, int data_size, const String& font_family, Style::FontStyle style, Style::FontWeight weight, bool fallback_face)
{
	return font_interface->LoadFontFace(data, data_size, font_family, style, weight, fallback_face);
}

void RegisterPlugin(Plugin* plugin)
{
	if (initialised)
		plugin->OnInitialise();

	PluginRegistry::RegisterPlugin(plugin);
}

void UnregisterPlugin(Plugin* plugin)
{
	PluginRegistry::UnregisterPlugin(plugin);

	if (initialised)
		plugin->OnShutdown();
}

EventId RegisterEventType(const String& type, bool interruptible, bool bubbles, DefaultActionPhase default_action_phase)
{
	return EventSpecificationInterface::InsertOrReplaceCustom(type, interruptible, bubbles, default_action_phase);
}

StringList GetTextureSourceList()
{
	return TextureDatabase::GetSourceList();
}

void ReleaseTextures()
{
	TextureDatabase::ReleaseTextures();
}

bool ReleaseTexture(const String& source)
{
	return TextureDatabase::ReleaseTexture(source);
}

void ReleaseCompiledGeometry()
{
	return GeometryDatabase::ReleaseAll();
}

void ReleaseMemoryPools()
{
	if (observerPtrBlockPool && observerPtrBlockPool->GetNumAllocatedObjects() <= 0)
	{
		delete observerPtrBlockPool;
		observerPtrBlockPool = nullptr;
	}
}

void ReleaseFontResources()
{
	if (font_interface)
	{
		for (const auto& name_context : contexts)
			name_context.second->GetRootElement()->DirtyFontFaceRecursive();

		font_interface->ReleaseFontResources();

		for (const auto& name_context : contexts)
			name_context.second->Update();
	}
}

} // namespace Rml
