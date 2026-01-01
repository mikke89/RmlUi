#pragma once

#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/SystemInterface.h>
#include <RmlUi/Core/Types.h>

#if RMLUI_SDL_VERSION_MAJOR == 3
	#include <SDL3/SDL.h>
#elif RMLUI_SDL_VERSION_MAJOR == 2
	#include <SDL.h>
#else
	#error "Unspecified RMLUI_SDL_VERSION_MAJOR. Please set this definition to the major version of the SDL library being linked to."
#endif

class SystemInterface_SDL : public Rml::SystemInterface {
public:
	SystemInterface_SDL();
	~SystemInterface_SDL();

	// Optionally, provide or change the window to be used for setting the mouse cursors.
	void SetWindow(SDL_Window* window);

	// -- Inherited from Rml::SystemInterface  --

	double GetElapsedTime() override;

	void SetMouseCursor(const Rml::String& cursor_name) override;

	void SetClipboardText(const Rml::String& text) override;
	void GetClipboardText(Rml::String& text) override;

	void ActivateKeyboard(Rml::Vector2f caret_position, float line_height) override;
	void DeactivateKeyboard() override;

private:
	SDL_Window* window = nullptr;

	SDL_Cursor* cursor_default = nullptr;
	SDL_Cursor* cursor_move = nullptr;
	SDL_Cursor* cursor_pointer = nullptr;
	SDL_Cursor* cursor_resize = nullptr;
	SDL_Cursor* cursor_cross = nullptr;
	SDL_Cursor* cursor_text = nullptr;
	SDL_Cursor* cursor_unavailable = nullptr;
};

namespace RmlSDL {

// Applies input on the context based on the given SDL event.
// @return True if the event is still propagating, false if it was handled by the context.
bool InputEventHandler(Rml::Context* context, SDL_Window* window, SDL_Event& ev);

// Converts the SDL key to RmlUi key.
Rml::Input::KeyIdentifier ConvertKey(int sdl_key);

// Converts the SDL mouse button to RmlUi mouse button.
int ConvertMouseButton(int sdl_mouse_button);

// Returns the active RmlUi key modifier state.
int GetKeyModifierState();

} // namespace RmlSDL
