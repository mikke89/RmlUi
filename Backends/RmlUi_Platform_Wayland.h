#pragma once

#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/SystemInterface.h>
#include <RmlUi/Core/Types.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <xkbcommon/xkbcommon.h>
#include <sys/time.h>

struct xdg_wm_base;
struct zxdg_decoration_manager_v1;

class SystemInterface_Wayland : public Rml::SystemInterface {
public:
	SystemInterface_Wayland(wl_display* display, wl_shm* shm);
	~SystemInterface_Wayland();

	void SetPointer(wl_pointer* pointer);
	void SetCursorSurface(wl_surface* surface);
	void SetPointerSerial(uint32_t serial);
	void ClearPointerSerial();

	double GetElapsedTime() override;
	void SetMouseCursor(const Rml::String& cursor_name) override;
	void SetClipboardText(const Rml::String& text) override;
	void GetClipboardText(Rml::String& text) override;

private:
	void LoadCursorTheme();
	void ApplyCursor(const char* cursor_name);

	wl_display* display = nullptr;
	wl_shm* shm = nullptr;
	wl_pointer* pointer = nullptr;
	wl_surface* cursor_surface = nullptr;
	wl_cursor_theme* cursor_theme = nullptr;
	timeval start_time {};
	uint32_t pointer_serial = 0;
	bool has_pointer_serial = false;
	Rml::String clipboard_text;
};

namespace RmlWayland {

struct Globals {
	wl_compositor* compositor = nullptr;
	wl_shm* shm = nullptr;
	wl_seat* seat = nullptr;
	xdg_wm_base* wm_base = nullptr;
	zxdg_decoration_manager_v1* decoration_manager = nullptr;
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
