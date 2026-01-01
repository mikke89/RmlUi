#pragma once

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/Types.h>

/**
    Provides common functionality required for the built-in RmlUi samples.
 */
namespace Shell {

// Initializes and sets a custom file interface used for locating the included RmlUi asset files.
bool Initialize();
// Destroys all resources constructed by the shell.
void Shutdown();

// Loads the fonts included with the RmlUi samples.
void LoadFonts();

// Process key down events to handle shortcuts common to all samples.
// @return True if the event is still propagating, false if it was handled here.
bool ProcessKeyDownShortcuts(Rml::Context* context, Rml::Input::KeyIdentifier key, int key_modifier, float native_dp_ratio, bool priority);

} // namespace Shell
