#pragma once

#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/SystemInterface.h>
#include <RmlUi/Core/Types.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <xkbcommon/xkbcommon.h>

struct wp_cursor_shape_device_v1;
struct wp_cursor_shape_manager_v1;

class SystemInterface_Wayland : public Rml::SystemInterface {
public:
	SystemInterface_Wayland(wl_display* display, wl_shm* shm, wp_cursor_shape_manager_v1* cursor_shape_manager);
	~SystemInterface_Wayland();

	void SetPointer(wl_pointer* pointer);
	void SetCursorSurface(wl_surface* surface);
	void SetPointerSerial(uint32_t serial);
	void ClearPointerSerial();

	void SetMouseCursor(const Rml::String& cursor_name) override;
	void SetClipboardText(const Rml::String& text) override;
	void GetClipboardText(Rml::String& text) override;

private:
	void LoadCursorTheme();
	bool ApplyCursorShape(uint32_t shape);
	void ApplyCursor(const char* const* cursor_names, size_t cursor_name_count);

	wl_display* display = nullptr;
	wl_shm* shm = nullptr;
	wl_pointer* pointer = nullptr;
	wl_surface* cursor_surface = nullptr;
	wp_cursor_shape_manager_v1* cursor_shape_manager = nullptr;
	wp_cursor_shape_device_v1* cursor_shape_device = nullptr;
	wl_cursor_theme* cursor_theme = nullptr;
	bool cursor_theme_load_attempted = false;
	Rml::UnorderedSet<Rml::String> warned_missing_cursors;
	uint32_t pointer_serial = 0;
	bool has_pointer_serial = false;
	Rml::String clipboard_text;
};

namespace RmlWayland {

struct Globals {
	wl_compositor* compositor = nullptr;
	wl_shm* shm = nullptr;
	wl_seat* seat = nullptr;
	wp_cursor_shape_manager_v1* cursor_shape_manager = nullptr;
};

struct KeyboardState {
	xkb_context* context = nullptr;
	xkb_keymap* keymap = nullptr;
	xkb_state* state = nullptr;
	int modifiers = 0;

	KeyboardState();
	~KeyboardState();
	void SetKeymapFromString(const char* keymap_string);
	void Reset();
	void UpdateModifiers(uint32_t depressed, uint32_t latched, uint32_t locked, uint32_t group);
};

Rml::Input::KeyIdentifier ConvertKeySym(xkb_keysym_t sym);
int ConvertKeyModifiers(xkb_state* state);
int ConvertMouseButton(uint32_t button);

} // namespace RmlWayland
