#pragma once

#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/SystemInterface.h>
#include <RmlUi/Core/Types.h>

using KeyDownCallback = bool (*)(Rml::Context* context, Rml::Input::KeyIdentifier key, int key_modifier, float native_dp_ratio, bool priority);

/**
    This interface serves as a basic abstraction over the various backends included with RmlUi. It is mainly intended as an example to get something
    simple up and running, and provides just enough functionality for the included samples.

    This interface may be used directly for simple applications and testing. However, for anything more advanced we recommend to use the backend as a
    starting point and copy relevant parts into the main loop of your application. On the other hand, the underlying platform and renderer used by the
    backend are intended to be re-usable as is.
 */
namespace Backend {

// Initializes the backend, including the custom system and render interfaces, and opens a window for rendering the RmlUi context.
bool Initialize(const char* window_name, int width, int height, bool allow_resize);
// Closes the window and release all resources owned by the backend, including the system and render interfaces.
void Shutdown();

// Returns a pointer to the custom system interface which should be provided to RmlUi.
Rml::SystemInterface* GetSystemInterface();
// Returns a pointer to the custom render interface which should be provided to RmlUi.
Rml::RenderInterface* GetRenderInterface();

// Polls and processes events from the current platform, and applies any relevant events to the provided RmlUi context and the key down callback.
// @return False to indicate that the application should be closed.
bool ProcessEvents(Rml::Context* context, KeyDownCallback key_down_callback = nullptr, bool power_save = false);
// Request application closure during the next event processing call.
void RequestExit();

// Prepares the render state to accept rendering commands from RmlUi, call before rendering the RmlUi context.
void BeginFrame();
// Presents the rendered frame to the screen, call after rendering the RmlUi context.
void PresentFrame();

} // namespace Backend
