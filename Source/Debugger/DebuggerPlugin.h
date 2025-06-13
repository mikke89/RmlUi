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

#ifndef RMLUI_DEBUGGER_DEBUGGERPLUGIN_H
#define RMLUI_DEBUGGER_DEBUGGERPLUGIN_H

#include "../../Include/RmlUi/Core/EventListener.h"
#include "../../Include/RmlUi/Core/Plugin.h"

namespace Rml {

class ElementDocument;
class SystemInterface;

namespace Debugger {

class ElementLog;
class ElementInfo;
class ElementContextHook;
class DebuggerSystemInterface;

/**
    RmlUi plugin interface for the debugger.

    @author Robert Curry
 */

class DebuggerPlugin : public Rml::Plugin, public Rml::EventListener {
public:
	DebuggerPlugin();
	~DebuggerPlugin();

	/// Initialises the debugging tools into the given context.
	/// @param[in] context The context to load the tools into.
	/// @return True on success, false if an error occured.
	bool Initialise(Context* context);

	/// Sets the context to be debugged.
	/// @param[in] context The context to be debugged.
	/// @return True if the debugger is initialised and the context was switched, false otherwise.
	bool SetContext(Context* context);

	/// Sets the visibility of the debugger.
	/// @param[in] visibility True to show the debugger, false to hide it.
	void SetVisible(bool visibility);
	/// Returns the visibility of the debugger.
	/// @return True if the debugger is visible, false if not.
	bool IsVisible();

	/// Renders any debug elements in the debug context.
	void Render();

	/// Called when RmlUi shuts down.
	void OnShutdown() override;

	/// Called whenever a RmlUi context is destroyed.
	/// @param[in] context The destroyed context.
	void OnContextDestroy(Context* context) override;

	/// Called whenever an element is destroyed.
	/// @param[in] element The destroyed element.
	void OnElementDestroy(Element* element) override;

	/// Event handler for events from the debugger elements.
	/// @param[in] event The event to process.
	void ProcessEvent(Event& event) override;

	/// Access the singleton instance of the debugger
	/// @return nullptr or an instance of the plugin
	static DebuggerPlugin* GetInstance();

private:
	bool LoadFont();
	bool LoadMenuElement();
	bool LoadInfoElement();
	bool LoadLogElement();

	void SetupInfoListeners(Rml::Context* new_context);

	// Release all loaded elements
	void ReleaseElements();

	// The context hosting the debug documents.
	Context* host_context;
	// The context we're debugging.
	Context* debug_context;

	// The debug elements.
	ElementDocument* menu_element;
	ElementInfo* info_element;
	ElementLog* log_element;
	ElementContextHook* hook_element;

	Rml::SystemInterface* application_interface;
	UniquePtr<DebuggerSystemInterface> log_interface;

	UniquePtr<ElementInstancer> hook_element_instancer, debug_document_instancer, info_element_instancer, log_element_instancer;

	bool render_outlines;

	// Singleton instance
	static DebuggerPlugin* instance;
};

} // namespace Debugger
} // namespace Rml

#endif
