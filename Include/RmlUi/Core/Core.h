/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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

#ifndef RMLUICORECORE_H
#define RMLUICORECORE_H

#include "Header.h"
#include "Types.h"
#include "Event.h"
#include "ComputedValues.h"

namespace Rml {
namespace Core {

class Plugin;
class Context;
class FileInterface;
class FontEngineInterface;
class RenderInterface;
class SystemInterface;
enum class DefaultActionPhase;


/**
	RmlUi library core API.

	@author Peter Curry
 */

/// Initialises RmlUi.
RMLUICORE_API bool Initialise();
/// Shutdown RmlUi.
RMLUICORE_API void Shutdown();

/// Returns the version of this RmlUi library.
/// @return The version number.
RMLUICORE_API String GetVersion();

/// Sets the interface through which all system requests are made. This must be called before Initialise().
/// @param[in] system_interface A non-owning pointer to the application-specified logging interface.
/// @lifetime The interface must be kept alive until after the call to Core::Shutdown.
RMLUICORE_API void SetSystemInterface(SystemInterface* system_interface);
/// Returns RmlUi's system interface.
RMLUICORE_API SystemInterface* GetSystemInterface();

/// Sets the interface through which all rendering requests are made. This is not required to be called, but if it is
/// it must be called before Initialise(). If no render interface is specified, then all contexts must have a custom
/// render interface.
/// @param[in] render_interface A non-owning pointer to the render interface implementation.
/// @lifetime The interface must be kept alive until after the call to Core::Shutdown.
RMLUICORE_API void SetRenderInterface(RenderInterface* render_interface);
/// Returns RmlUi's default's render interface.
RMLUICORE_API RenderInterface* GetRenderInterface();

/// Sets the interface through which all file I/O requests are made. This is not required to be called, but if it is it
/// must be called before Initialise().
/// @param[in] file_interface A non-owning pointer to the application-specified file interface.
/// @lifetime The interface must be kept alive until after the call to Core::Shutdown.
RMLUICORE_API void SetFileInterface(FileInterface* file_interface);
/// Returns RmlUi's file interface.
RMLUICORE_API FileInterface* GetFileInterface();

/// Sets the interface through which all font requests are made. This is not required to be called, but if it is
/// it must be called before Initialise().
/// @param[in] font_interface A non-owning pointer to the application-specified font engine interface.
/// @lifetime The interface must be kept alive until after the call to Core::Shutdown.
RMLUICORE_API void SetFontEngineInterface(FontEngineInterface* font_interface);
/// Returns RmlUi's font interface.
RMLUICORE_API FontEngineInterface* GetFontEngineInterface();
	
/// Creates a new element context.
/// @param[in] name The new name of the context. This must be unique.
/// @param[in] dimensions The initial dimensions of the new context.
/// @param[in] render_interface The custom render interface to use, or nullptr to use the default.
/// @lifetime If specified, the render interface must be kept alive until after the context is destroyed or the call to Core::Shutdown.
/// @return A non-owning pointer to the new context, or nullptr if the context could not be created.
RMLUICORE_API Context* CreateContext(const String& name, const Vector2i& dimensions, RenderInterface* render_interface = nullptr);
/// Removes and destroys a context.
/// @param[in] name The name of the context to remove.
/// @return True if name is a valid context, false otherwise.
RMLUICORE_API bool RemoveContext(const String& name);
/// Fetches a previously constructed context by name.
/// @param[in] name The name of the desired context.
/// @return The desired context, or nullptr if no context exists with the given name.
RMLUICORE_API Context* GetContext(const String& name);
/// Fetches a context by index.
/// @param[in] index The index of the desired context. If this is outside of the valid range of contexts, it will be clamped.
/// @return The requested context, or nullptr if no contexts exist.
RMLUICORE_API Context* GetContext(int index);
/// Returns the number of active contexts.
/// @return The total number of active RmlUi contexts.
RMLUICORE_API int GetNumContexts();

/// Adds a new font face to the font engine. The face's family, style and weight will be determined from the face itself.
/// @param[in] file_name The file to load the face from.
/// @param[in] fallback_face True to use this font face for unknown characters in other font faces.
/// @return True if the face was loaded successfully, false otherwise.
RMLUICORE_API bool LoadFontFace(const String& file_name, bool fallback_face = false);
/// Adds a new font face from memory to the font engine. The face's family, style and weight is given by the parameters.
/// @param[in] data A pointer to the data.
/// @param[in] data_size Size of the data in bytes.
/// @param[in] family The family to register the font as.
/// @param[in] style The style to register the font as.
/// @param[in] weight The weight to register the font as.
/// @param[in] fallback_face True to use this font face for unknown characters in other font faces.
/// @return True if the face was loaded successfully, false otherwise.
RMLUICORE_API bool LoadFontFace(const byte* data, int data_size, const String& font_family, Style::FontStyle style, Style::FontWeight weight, bool fallback_face = false);

/// Registers a generic RmlUi plugin.
RMLUICORE_API void RegisterPlugin(Plugin* plugin);

/// Registers a new event type. If the type already exists, it will replace custom event types, but not internal types.
/// @param[in] type The new event type.
/// @param[in] interruptible Whether the event can be interrupted during dispatch.
/// @param[in] bubbles Whether the event executes the bubble phase. If false, only capture and target phase is executed.
/// @param[in] default_action_phase Defines during which phase(s) the 'Element::ProcessDefaultAction' method is called.
/// @return The EventId of the newly created type, or existing type if 'type' is an internal type.
RMLUICORE_API EventId RegisterEventType(const String& type, bool interruptible, bool bubbles, DefaultActionPhase default_action_phase = DefaultActionPhase::None);

/// Forces all texture handles loaded and generated by RmlUi to be released.
RMLUICORE_API void ReleaseTextures();
/// Forces all compiled geometry handles generated by RmlUi to be released.
RMLUICORE_API void ReleaseCompiledGeometry();

}
}

#endif
