#pragma once

#include "RmlUi_Include_Windows.h"
#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/StringUtilities.h>
#include <RmlUi/Core/SystemInterface.h>
#include <RmlUi/Core/TextInputHandler.h>
#include <RmlUi/Core/Types.h>
#include <string>

class SystemInterface_Win32 : public Rml::SystemInterface {
public:
	SystemInterface_Win32();
	~SystemInterface_Win32();

	// Optionally, provide or change the window to be used for setting the mouse cursor, clipboard text and IME position.
	void SetWindow(HWND window_handle);

	// -- Inherited from Rml::SystemInterface  --

	double GetElapsedTime() override;

	void SetMouseCursor(const Rml::String& cursor_name) override;

	void SetClipboardText(const Rml::String& text) override;
	void GetClipboardText(Rml::String& text) override;

	void ActivateKeyboard(Rml::Vector2f caret_position, float line_height) override;

private:
	HWND window_handle = nullptr;

	double time_frequency = {};
	LARGE_INTEGER time_startup = {};

	HCURSOR cursor_default = nullptr;
	HCURSOR cursor_move = nullptr;
	HCURSOR cursor_pointer = nullptr;
	HCURSOR cursor_resize = nullptr;
	HCURSOR cursor_cross = nullptr;
	HCURSOR cursor_text = nullptr;
	HCURSOR cursor_unavailable = nullptr;
};

class TextInputMethodEditor_Win32;

/**
    Optional helper functions for the Win32 plaform.
 */
namespace RmlWin32 {

// Convenience helpers for converting between RmlUi strings (UTF-8) and Windows strings (UTF-16).
Rml::String ConvertToUTF8(const std::wstring& wstr);
std::wstring ConvertToUTF16(const Rml::String& str);

// Window event handler to submit default input behavior to the context.
// @return True if the event is still propagating, false if it was handled by the context.
bool WindowProcedure(Rml::Context* context, TextInputMethodEditor_Win32& text_input_method_editor, HWND window_handle, UINT message, WPARAM w_param,
	LPARAM l_param);

// Converts the key from Win32 key code to RmlUi key.
Rml::Input::KeyIdentifier ConvertKey(int win32_key_code);

// Returns the active RmlUi key modifier state.
int GetKeyModifierState();

} // namespace RmlWin32

/**
    Custom backend implementation of TextInputHandler to handle the system's Input Method Editor (IME).
    This version supports only one active text input context.
 */
class TextInputMethodEditor_Win32 final : public Rml::TextInputHandler {
public:
	TextInputMethodEditor_Win32();

	void OnActivate(Rml::TextInputContext* input_context) override;
	void OnDeactivate(Rml::TextInputContext* input_context) override;
	void OnDestroy(Rml::TextInputContext* input_context) override;

	/// Check that a composition is currently active.
	/// @return True if we are composing, false otherwise.
	bool IsComposing() const;

	void StartComposition();
	void CancelComposition();

	/// Set the composition string.
	/// @param[in] composition A string to be set.
	void SetComposition(Rml::StringView composition);

	/// End the current composition by confirming the composition string.
	/// @param[in] composition A string to confirm.
	void ConfirmComposition(Rml::StringView composition);

	/// Set the cursor position within the composition.
	/// @param[in] cursor_pos A character position of the cursor within the composition string.
	/// @param[in] update Update the cursor position within active input contexts.
	void SetCursorPosition(int cursor_pos, bool update);

private:
	void EndComposition();
	void SetCompositionString(Rml::StringView composition);

	void UpdateCursorPosition();

private:
	// An actively used text input method context.
	Rml::TextInputContext* input_context;

	// A flag to mark a composition is currently active.
	bool composing;
	// Character position of the cursor in the composition string.
	int cursor_pos;

	// Composition range (character position) relative to the text input value.
	int composition_range_start;
	int composition_range_end;
};
