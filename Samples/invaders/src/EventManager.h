#pragma once

#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Types.h>

class EventHandler;

class EventManager {
public:
	/// Releases all event handlers registered with the manager.
	static void Shutdown();

	/// Registers a new event handler with the manager.
	/// @param[in] handler_name The name of the handler; this must be the same as the window it is handling events for.
	/// @param[in] handler The event handler.
	static void RegisterEventHandler(const Rml::String& handler_name, Rml::UniquePtr<EventHandler> handler);

	/// Processes an event coming through from RmlUi.
	/// @param[in] event The RmlUi event that spawned the application event.
	/// @param[in] value The application-specific event value.
	static void ProcessEvent(Rml::Event& event, const Rml::String& value);
	/// Loads a window and binds the event handler for it.
	/// @param[in] window_name The name of the window to load.
	static Rml::ElementDocument* LoadWindow(const Rml::String& window_name);

private:
	EventManager();
	~EventManager();
};
