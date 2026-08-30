#include "RmlUi_Platform_Wayland.h"
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/StringUtilities.h>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <linux/input-event-codes.h>
#include <unistd.h>

#include <cursor-shape-v1-client-protocol.h>

struct CursorSettings {
	Rml::String theme_name;
	Rml::String theme_source;
	int size = 24;
	bool has_size = false;
};

static constexpr const char* DefaultCursorNames[] = {"default", "left_ptr", "arrow"};
static constexpr const char* MoveCursorNames[] = {"move", "fleur", "all-scroll"};
static constexpr const char* PointerCursorNames[] = {"pointer", "hand2", "pointing_hand"};
static constexpr const char* ResizeCursorNames[] = {"se-resize", "bottom_right_corner", "size_fdiag"};
static constexpr const char* CrossCursorNames[] = {"crosshair", "cross", "tcross"};
static constexpr const char* TextCursorNames[] = {"text", "xterm", "ibeam"};
static constexpr const char* UnavailableCursorNames[] = {"not-allowed", "forbidden", "crossed_circle"};

static Rml::String GetEnvironmentValue(const char* name)
{
	if (const char* value = getenv(name))
		return value;
	return {};
}

static bool ParseCursorSize(const Rml::String& string, int& size)
{
	const Rml::String stripped = Rml::StringUtilities::StripWhitespace(string);
	if (stripped.empty())
		return false;

	char* end = nullptr;
	const long parsed_size = strtol(stripped.c_str(), &end, 10);
	if (!end || *end != '\0' || parsed_size <= 0 || parsed_size > 1024)
		return false;

	size = int(parsed_size);
	return true;
}

static bool IsKdeSession()
{
	const Rml::String current_desktop = Rml::StringUtilities::ToLower(GetEnvironmentValue("XDG_CURRENT_DESKTOP"));
	if (current_desktop.find("kde") != Rml::String::npos || current_desktop.find("plasma") != Rml::String::npos)
		return true;

	const Rml::String session_desktop = Rml::StringUtilities::ToLower(GetEnvironmentValue("XDG_SESSION_DESKTOP"));
	if (session_desktop.find("kde") != Rml::String::npos || session_desktop.find("plasma") != Rml::String::npos)
		return true;

	return !GetEnvironmentValue("KDE_FULL_SESSION").empty();
}

static Rml::String MakeHomePath(const char* relative_path)
{
	const Rml::String home = GetEnvironmentValue("HOME");
	if (home.empty())
		return {};

	Rml::String path = home;
	path += "/";
	path += relative_path;
	return path;
}

static void ReadKdeCursorConfig(const Rml::String& path, CursorSettings& settings)
{
	std::ifstream stream(path.c_str());
	if (!stream)
		return;

	bool in_mouse_group = false;
	Rml::String line;
	while (std::getline(stream, line))
	{
		line = Rml::StringUtilities::StripWhitespace(line);
		if (line.empty() || line[0] == '#' || line[0] == ';')
			continue;

		if (line.front() == '[' && line.back() == ']')
		{
			const Rml::String group_name = line.substr(1, line.size() - 2);
			in_mouse_group = (group_name == "Mouse");
			continue;
		}

		if (!in_mouse_group)
			continue;

		const size_t equals = line.find('=');
		if (equals == Rml::String::npos)
			continue;

		const Rml::String key = Rml::StringUtilities::StripWhitespace(line.substr(0, equals));
		const Rml::String value = Rml::StringUtilities::StripWhitespace(line.substr(equals + 1));
		if (value.empty())
			continue;

		if (settings.theme_name.empty() && key == "cursorTheme")
		{
			settings.theme_name = value;
			settings.theme_source = path;
		}
		else if (!settings.has_size && (key == "cursorSize" || key == "CursorSize"))
		{
			int parsed_size = 0;
			if (ParseCursorSize(value, parsed_size))
			{
				settings.size = parsed_size;
				settings.has_size = true;
			}
		}
	}
}

static CursorSettings ResolveCursorSettings()
{
	CursorSettings settings;

	const Rml::String environment_theme = Rml::StringUtilities::StripWhitespace(GetEnvironmentValue("XCURSOR_THEME"));
	if (!environment_theme.empty())
	{
		settings.theme_name = environment_theme;
		settings.theme_source = "XCURSOR_THEME";
	}

	int environment_size = 0;
	if (ParseCursorSize(GetEnvironmentValue("XCURSOR_SIZE"), environment_size))
	{
		settings.size = environment_size;
		settings.has_size = true;
	}

	if (IsKdeSession())
	{
		// XDG_CONFIG_HOME replaces ~/.config; do not fall through to the home path when it is set.
		const Rml::String xdg_config_home = GetEnvironmentValue("XDG_CONFIG_HOME");
		const Rml::String config_home = !xdg_config_home.empty() ? xdg_config_home : MakeHomePath(".config");
		if (!config_home.empty())
		{
			ReadKdeCursorConfig(config_home + "/kcminputrc", settings);
			ReadKdeCursorConfig(config_home + "/kdedefaults/kcminputrc", settings);
		}
	}

	return settings;
}

SystemInterface_Wayland::SystemInterface_Wayland(wl_display* display, wl_shm* shm, wp_cursor_shape_manager_v1* cursor_shape_manager) :
	display(display), shm(shm), cursor_shape_manager(cursor_shape_manager)
{}

SystemInterface_Wayland::~SystemInterface_Wayland()
{
	if (cursor_shape_device)
		wp_cursor_shape_device_v1_destroy(cursor_shape_device);
	if (cursor_theme)
		wl_cursor_theme_destroy(cursor_theme);
}

void SystemInterface_Wayland::SetPointer(wl_pointer* in_pointer)
{
	if (cursor_shape_device)
		wp_cursor_shape_device_v1_destroy(cursor_shape_device);
	cursor_shape_device = nullptr;

	pointer = in_pointer;

	if (pointer && cursor_shape_manager)
		cursor_shape_device = wp_cursor_shape_manager_v1_get_pointer(cursor_shape_manager, pointer);
}

void SystemInterface_Wayland::SetCursorSurface(wl_surface* surface)
{
	cursor_surface = surface;
}

void SystemInterface_Wayland::SetPointerSerial(uint32_t serial)
{
	pointer_serial = serial;
	has_pointer_serial = true;
}

void SystemInterface_Wayland::ClearPointerSerial()
{
	has_pointer_serial = false;
}

void SystemInterface_Wayland::LoadCursorTheme()
{
	// Resolve and attempt load once per instance once shm is available. A failed attempt is not retried on every
	// ApplyCursor; that would re-read env and config files for no gain.
	if (cursor_theme || cursor_theme_load_attempted || !shm)
		return;

	cursor_theme_load_attempted = true;

	const CursorSettings settings = ResolveCursorSettings();
	const int cursor_size = settings.size;
	const char* theme_name = settings.theme_name.empty() ? nullptr : settings.theme_name.c_str();

	if (theme_name)
	{
		if (settings.theme_source.empty())
			Rml::Log::Message(Rml::Log::LT_INFO, "Loading Wayland cursor theme '%s' at size %d.", theme_name, cursor_size);
		else
			Rml::Log::Message(Rml::Log::LT_INFO, "Loading Wayland cursor theme '%s' at size %d from %s.", theme_name, cursor_size,
				settings.theme_source.c_str());

		cursor_theme = wl_cursor_theme_load(theme_name, cursor_size, shm);
	}
	else
	{
		Rml::Log::Message(Rml::Log::LT_INFO, "Loading default Wayland cursor theme at size %d.", cursor_size);
		cursor_theme = wl_cursor_theme_load(nullptr, cursor_size, shm);
	}

	if (!cursor_theme)
		Rml::Log::Message(Rml::Log::LT_WARNING, "Failed to load Wayland cursor theme.");
}

bool SystemInterface_Wayland::ApplyCursorShape(uint32_t shape)
{
	if (cursor_shape_device && has_pointer_serial)
	{
		wp_cursor_shape_device_v1_set_shape(cursor_shape_device, pointer_serial, shape);
		wl_display_flush(display);
		return true;
	}
	return false;
}

void SystemInterface_Wayland::ApplyCursor(const char* const* cursor_names, size_t cursor_name_count)
{
	if (!pointer || !cursor_surface || !has_pointer_serial || !cursor_names || cursor_name_count == 0)
		return;

	LoadCursorTheme();
	if (!cursor_theme)
		return;

	wl_cursor* cursor = nullptr;
	for (size_t i = 0; i < cursor_name_count; ++i)
	{
		if (!cursor_names[i])
			continue;

		cursor = wl_cursor_theme_get_cursor(cursor_theme, cursor_names[i]);
		if (cursor && cursor->image_count > 0)
			break;
	}

	const char* requested_name = cursor_names[0] ? cursor_names[0] : "";
	if (!cursor || cursor->image_count == 0)
	{
		if (warned_missing_cursors.insert(requested_name).second)
		{
			if (cursor_names == DefaultCursorNames)
				Rml::Log::Message(Rml::Log::LT_WARNING, "Failed to load default Wayland cursor.");
			else
				Rml::Log::Message(Rml::Log::LT_WARNING, "Wayland cursor '%s' is unavailable in the loaded theme; using default.", requested_name);
		}

		if (cursor_names != DefaultCursorNames)
		{
			for (const char* fallback_name : DefaultCursorNames)
			{
				cursor = wl_cursor_theme_get_cursor(cursor_theme, fallback_name);
				if (cursor && cursor->image_count > 0)
					break;
			}
		}
	}

	if (!cursor || cursor->image_count == 0)
		return;

	wl_cursor_image* image = cursor->images[0];
	wl_buffer* buffer = wl_cursor_image_get_buffer(image);
	if (!buffer)
		return;

	wl_pointer_set_cursor(pointer, pointer_serial, cursor_surface, int32_t(image->hotspot_x), int32_t(image->hotspot_y));
	wl_surface_attach(cursor_surface, buffer, 0, 0);
	wl_surface_damage(cursor_surface, 0, 0, int32_t(image->width), int32_t(image->height));
	wl_surface_commit(cursor_surface);
	wl_display_flush(display);
}

void SystemInterface_Wayland::SetMouseCursor(const Rml::String& cursor_name)
{
	if (cursor_name.empty() || cursor_name == "arrow")
	{
		if (ApplyCursorShape(WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT))
			return;
		ApplyCursor(DefaultCursorNames, std::size(DefaultCursorNames));
	}
	else if (cursor_name == "move")
	{
		if (ApplyCursorShape(WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_MOVE))
			return;
		ApplyCursor(MoveCursorNames, std::size(MoveCursorNames));
	}
	else if (Rml::StringUtilities::StartsWith(cursor_name, "rmlui-scroll"))
	{
		if (ApplyCursorShape(WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_ALL_SCROLL))
			return;
		ApplyCursor(MoveCursorNames, std::size(MoveCursorNames));
	}
	else if (cursor_name == "pointer")
	{
		if (ApplyCursorShape(WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER))
			return;
		ApplyCursor(PointerCursorNames, std::size(PointerCursorNames));
	}
	else if (cursor_name == "resize")
	{
		if (ApplyCursorShape(WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_SE_RESIZE))
			return;
		ApplyCursor(ResizeCursorNames, std::size(ResizeCursorNames));
	}
	else if (cursor_name == "cross")
	{
		if (ApplyCursorShape(WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_CROSSHAIR))
			return;
		ApplyCursor(CrossCursorNames, std::size(CrossCursorNames));
	}
	else if (cursor_name == "text")
	{
		if (ApplyCursorShape(WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_TEXT))
			return;
		ApplyCursor(TextCursorNames, std::size(TextCursorNames));
	}
	else if (cursor_name == "unavailable")
	{
		if (ApplyCursorShape(WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NOT_ALLOWED))
			return;
		ApplyCursor(UnavailableCursorNames, std::size(UnavailableCursorNames));
	}
	else
	{
		const char* custom_cursor_names[] = {cursor_name.c_str()};
		ApplyCursor(custom_cursor_names, std::size(custom_cursor_names));
	}
}

void SystemInterface_Wayland::SetClipboardText(const Rml::String& text)
{
	clipboard_text = text;
}

void SystemInterface_Wayland::GetClipboardText(Rml::String& text)
{
	text = clipboard_text;
}

namespace RmlWayland {

KeyboardState::KeyboardState()
{
	context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
}

KeyboardState::~KeyboardState()
{
	if (state)
		xkb_state_unref(state);
	if (keymap)
		xkb_keymap_unref(keymap);
	if (context)
		xkb_context_unref(context);
}

void KeyboardState::SetKeymapFromString(const char* keymap_string)
{
	if (!context || !keymap_string)
		return;

	xkb_keymap* new_keymap = xkb_keymap_new_from_string(context, keymap_string, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
	if (!new_keymap)
	{
		Rml::Log::Message(Rml::Log::LT_WARNING, "Failed to load Wayland keyboard map.");
		return;
	}

	xkb_state* new_state = xkb_state_new(new_keymap);
	if (!new_state)
	{
		xkb_keymap_unref(new_keymap);
		Rml::Log::Message(Rml::Log::LT_WARNING, "Failed to create Wayland keyboard state.");
		return;
	}

	if (state)
		xkb_state_unref(state);
	if (keymap)
		xkb_keymap_unref(keymap);

	keymap = new_keymap;
	state = new_state;
	modifiers = 0;
}

void KeyboardState::Reset()
{
	modifiers = 0;

	xkb_state* new_state = nullptr;
	if (keymap)
		new_state = xkb_state_new(keymap);

	if (state)
		xkb_state_unref(state);
	state = new_state;

	if (keymap && !state)
		Rml::Log::Message(Rml::Log::LT_WARNING, "Failed to reset Wayland keyboard state.");
}

void KeyboardState::UpdateModifiers(uint32_t depressed, uint32_t latched, uint32_t locked, uint32_t group)
{
	if (!state)
		return;

	xkb_state_update_mask(state, depressed, latched, locked, 0, 0, group);
	modifiers = ConvertKeyModifiers(state);
}

int ConvertKeyModifiers(xkb_state* state)
{
	int result = 0;
	if (!state)
		return result;

	if (xkb_state_mod_name_is_active(state, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE))
		result |= Rml::Input::KM_SHIFT;
	if (xkb_state_mod_name_is_active(state, XKB_MOD_NAME_CAPS, XKB_STATE_MODS_EFFECTIVE))
		result |= Rml::Input::KM_CAPSLOCK;
	if (xkb_state_mod_name_is_active(state, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE))
		result |= Rml::Input::KM_CTRL;
	if (xkb_state_mod_name_is_active(state, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE))
		result |= Rml::Input::KM_ALT;
	if (xkb_state_mod_name_is_active(state, XKB_MOD_NAME_NUM, XKB_STATE_MODS_EFFECTIVE))
		result |= Rml::Input::KM_NUMLOCK;

	return result;
}

int ConvertMouseButton(uint32_t button)
{
	switch (button)
	{
	case BTN_LEFT: return 0;
	case BTN_RIGHT: return 1;
	case BTN_MIDDLE: return 2;
	default: return -1;
	}
}

Rml::Input::KeyIdentifier ConvertKeySym(xkb_keysym_t sym)
{
	// clang-format off
	switch (sym)
	{
	case XKB_KEY_BackSpace: return Rml::Input::KI_BACK;
	case XKB_KEY_Tab: return Rml::Input::KI_TAB;
	case XKB_KEY_Clear: return Rml::Input::KI_CLEAR;
	case XKB_KEY_Return: return Rml::Input::KI_RETURN;
	case XKB_KEY_Pause: return Rml::Input::KI_PAUSE;
	case XKB_KEY_Scroll_Lock: return Rml::Input::KI_SCROLL;
	case XKB_KEY_Escape: return Rml::Input::KI_ESCAPE;
	case XKB_KEY_Delete: return Rml::Input::KI_DELETE;
	case XKB_KEY_Home: return Rml::Input::KI_HOME;
	case XKB_KEY_Left: return Rml::Input::KI_LEFT;
	case XKB_KEY_Up: return Rml::Input::KI_UP;
	case XKB_KEY_Right: return Rml::Input::KI_RIGHT;
	case XKB_KEY_Down: return Rml::Input::KI_DOWN;
	case XKB_KEY_Page_Up: return Rml::Input::KI_PRIOR;
	case XKB_KEY_Page_Down: return Rml::Input::KI_NEXT;
	case XKB_KEY_End: return Rml::Input::KI_END;
	case XKB_KEY_Insert: return Rml::Input::KI_INSERT;
	case XKB_KEY_Num_Lock: return Rml::Input::KI_NUMLOCK;

	case XKB_KEY_KP_Space: return Rml::Input::KI_SPACE;
	case XKB_KEY_KP_Tab: return Rml::Input::KI_TAB;
	case XKB_KEY_KP_Enter: return Rml::Input::KI_NUMPADENTER;
	case XKB_KEY_KP_Home: return Rml::Input::KI_NUMPAD7;
	case XKB_KEY_KP_Left: return Rml::Input::KI_NUMPAD4;
	case XKB_KEY_KP_Up: return Rml::Input::KI_NUMPAD8;
	case XKB_KEY_KP_Right: return Rml::Input::KI_NUMPAD6;
	case XKB_KEY_KP_Down: return Rml::Input::KI_NUMPAD2;
	case XKB_KEY_KP_Page_Up: return Rml::Input::KI_NUMPAD9;
	case XKB_KEY_KP_Page_Down: return Rml::Input::KI_NUMPAD3;
	case XKB_KEY_KP_End: return Rml::Input::KI_NUMPAD1;
	case XKB_KEY_KP_Begin: return Rml::Input::KI_NUMPAD5;
	case XKB_KEY_KP_Insert: return Rml::Input::KI_NUMPAD0;
	case XKB_KEY_KP_Delete: return Rml::Input::KI_DECIMAL;
	case XKB_KEY_KP_Multiply: return Rml::Input::KI_MULTIPLY;
	case XKB_KEY_KP_Add: return Rml::Input::KI_ADD;
	case XKB_KEY_KP_Separator: return Rml::Input::KI_SEPARATOR;
	case XKB_KEY_KP_Subtract: return Rml::Input::KI_SUBTRACT;
	case XKB_KEY_KP_Decimal: return Rml::Input::KI_DECIMAL;
	case XKB_KEY_KP_Divide: return Rml::Input::KI_DIVIDE;

	case XKB_KEY_F1: return Rml::Input::KI_F1;
	case XKB_KEY_F2: return Rml::Input::KI_F2;
	case XKB_KEY_F3: return Rml::Input::KI_F3;
	case XKB_KEY_F4: return Rml::Input::KI_F4;
	case XKB_KEY_F5: return Rml::Input::KI_F5;
	case XKB_KEY_F6: return Rml::Input::KI_F6;
	case XKB_KEY_F7: return Rml::Input::KI_F7;
	case XKB_KEY_F8: return Rml::Input::KI_F8;
	case XKB_KEY_F9: return Rml::Input::KI_F9;
	case XKB_KEY_F10: return Rml::Input::KI_F10;
	case XKB_KEY_F11: return Rml::Input::KI_F11;
	case XKB_KEY_F12: return Rml::Input::KI_F12;
	case XKB_KEY_F13: return Rml::Input::KI_F13;
	case XKB_KEY_F14: return Rml::Input::KI_F14;
	case XKB_KEY_F15: return Rml::Input::KI_F15;
	case XKB_KEY_F16: return Rml::Input::KI_F16;
	case XKB_KEY_F17: return Rml::Input::KI_F17;
	case XKB_KEY_F18: return Rml::Input::KI_F18;
	case XKB_KEY_F19: return Rml::Input::KI_F19;
	case XKB_KEY_F20: return Rml::Input::KI_F20;
	case XKB_KEY_F21: return Rml::Input::KI_F21;
	case XKB_KEY_F22: return Rml::Input::KI_F22;
	case XKB_KEY_F23: return Rml::Input::KI_F23;
	case XKB_KEY_F24: return Rml::Input::KI_F24;

	case XKB_KEY_Shift_L: return Rml::Input::KI_LSHIFT;
	case XKB_KEY_Shift_R: return Rml::Input::KI_RSHIFT;
	case XKB_KEY_Control_L: return Rml::Input::KI_LCONTROL;
	case XKB_KEY_Control_R: return Rml::Input::KI_RCONTROL;
	case XKB_KEY_Caps_Lock: return Rml::Input::KI_CAPITAL;
	case XKB_KEY_Alt_L: return Rml::Input::KI_LMENU;
	case XKB_KEY_Alt_R: return Rml::Input::KI_RMENU;
	case XKB_KEY_Super_L: return Rml::Input::KI_LWIN;
	case XKB_KEY_Super_R: return Rml::Input::KI_RWIN;

	case XKB_KEY_space: return Rml::Input::KI_SPACE;
	case XKB_KEY_apostrophe: return Rml::Input::KI_OEM_7;
	case XKB_KEY_comma: return Rml::Input::KI_OEM_COMMA;
	case XKB_KEY_minus: return Rml::Input::KI_OEM_MINUS;
	case XKB_KEY_period: return Rml::Input::KI_OEM_PERIOD;
	case XKB_KEY_slash: return Rml::Input::KI_OEM_2;
	case XKB_KEY_0: return Rml::Input::KI_0;
	case XKB_KEY_1: return Rml::Input::KI_1;
	case XKB_KEY_2: return Rml::Input::KI_2;
	case XKB_KEY_3: return Rml::Input::KI_3;
	case XKB_KEY_4: return Rml::Input::KI_4;
	case XKB_KEY_5: return Rml::Input::KI_5;
	case XKB_KEY_6: return Rml::Input::KI_6;
	case XKB_KEY_7: return Rml::Input::KI_7;
	case XKB_KEY_8: return Rml::Input::KI_8;
	case XKB_KEY_9: return Rml::Input::KI_9;
	case XKB_KEY_semicolon: return Rml::Input::KI_OEM_1;
	case XKB_KEY_equal: return Rml::Input::KI_OEM_PLUS;
	case XKB_KEY_bracketleft: return Rml::Input::KI_OEM_4;
	case XKB_KEY_backslash: return Rml::Input::KI_OEM_5;
	case XKB_KEY_bracketright: return Rml::Input::KI_OEM_6;
	case XKB_KEY_grave: return Rml::Input::KI_OEM_3;
	case XKB_KEY_a: case XKB_KEY_A: return Rml::Input::KI_A;
	case XKB_KEY_b: case XKB_KEY_B: return Rml::Input::KI_B;
	case XKB_KEY_c: case XKB_KEY_C: return Rml::Input::KI_C;
	case XKB_KEY_d: case XKB_KEY_D: return Rml::Input::KI_D;
	case XKB_KEY_e: case XKB_KEY_E: return Rml::Input::KI_E;
	case XKB_KEY_f: case XKB_KEY_F: return Rml::Input::KI_F;
	case XKB_KEY_g: case XKB_KEY_G: return Rml::Input::KI_G;
	case XKB_KEY_h: case XKB_KEY_H: return Rml::Input::KI_H;
	case XKB_KEY_i: case XKB_KEY_I: return Rml::Input::KI_I;
	case XKB_KEY_j: case XKB_KEY_J: return Rml::Input::KI_J;
	case XKB_KEY_k: case XKB_KEY_K: return Rml::Input::KI_K;
	case XKB_KEY_l: case XKB_KEY_L: return Rml::Input::KI_L;
	case XKB_KEY_m: case XKB_KEY_M: return Rml::Input::KI_M;
	case XKB_KEY_n: case XKB_KEY_N: return Rml::Input::KI_N;
	case XKB_KEY_o: case XKB_KEY_O: return Rml::Input::KI_O;
	case XKB_KEY_p: case XKB_KEY_P: return Rml::Input::KI_P;
	case XKB_KEY_q: case XKB_KEY_Q: return Rml::Input::KI_Q;
	case XKB_KEY_r: case XKB_KEY_R: return Rml::Input::KI_R;
	case XKB_KEY_s: case XKB_KEY_S: return Rml::Input::KI_S;
	case XKB_KEY_t: case XKB_KEY_T: return Rml::Input::KI_T;
	case XKB_KEY_u: case XKB_KEY_U: return Rml::Input::KI_U;
	case XKB_KEY_v: case XKB_KEY_V: return Rml::Input::KI_V;
	case XKB_KEY_w: case XKB_KEY_W: return Rml::Input::KI_W;
	case XKB_KEY_x: case XKB_KEY_X: return Rml::Input::KI_X;
	case XKB_KEY_y: case XKB_KEY_Y: return Rml::Input::KI_Y;
	case XKB_KEY_z: case XKB_KEY_Z: return Rml::Input::KI_Z;
	default: break;
	}
	// clang-format on

	return Rml::Input::KI_UNKNOWN;
}

} // namespace RmlWayland
