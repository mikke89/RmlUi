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

#include <macosx/InputMacOSX.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Input.h>
#include <RmlUi/Debugger.h>
#include <Shell.h>

// Defines for Carbon key modifiers.
#define KEY_ALT 256
#define KEY_SHIFT 512
#define KEY_CAPS 1024
#define KEY_OPTION 2048
#define KEY_CTRL 4096

static void InitialiseKeymap();
static int GetKeyModifierState(EventRef event);

static const int KEYMAP_SIZE = 256;
static Rml::Core::Input::KeyIdentifier key_identifier_map[KEYMAP_SIZE];

bool InputMacOSX::Initialise()
{
	InitialiseKeymap();
	return true;
}

void InputMacOSX::Shutdown()
{
}

OSStatus InputMacOSX::EventHandler(EventHandlerCallRef next_handler, EventRef event, void* p)
{
	// Process all mouse and keyboard events
	switch (GetEventClass(event))
	{
		case kEventClassMouse:
		{
			switch (GetEventKind(event))
			{
				case kEventMouseDown:
				{
					EventMouseButton mouse_button;
					if (GetEventParameter(event, kEventParamMouseButton, typeMouseButton, nullptr, sizeof(EventMouseButton), nullptr, &mouse_button) == noErr)
						context->ProcessMouseButtonDown(mouse_button - 1, GetKeyModifierState(event));
				}
				break;
				
				case kEventMouseUp:
				{
					EventMouseButton mouse_button;
					if (GetEventParameter(event, kEventParamMouseButton, typeMouseButton, nullptr, sizeof(EventMouseButton), nullptr, &mouse_button) == noErr)
						context->ProcessMouseButtonUp(mouse_button - 1, GetKeyModifierState(event));
				}
				break;

				case kEventMouseWheelMoved:
				{
					EventMouseWheelAxis axis;
					SInt32 delta;

					if (GetEventParameter(event, kEventParamMouseWheelAxis, typeMouseWheelAxis, nullptr, sizeof(EventMouseWheelAxis), nullptr, &axis) == noErr &&
						GetEventParameter(event, kEventParamMouseWheelDelta, typeLongInteger, nullptr, sizeof(SInt32), nullptr, &delta) == noErr)
					{
						if (axis == kEventMouseWheelAxisY)
							context->ProcessMouseWheel(-delta, GetKeyModifierState(event));
					}
				}
				break;

				case kEventMouseMoved:
				case kEventMouseDragged:
				{
					HIPoint position;
					if (GetEventParameter(event, kEventParamWindowMouseLocation, typeHIPoint, nullptr, sizeof(HIPoint), nullptr, &position) == noErr)
						context->ProcessMouseMove(position.x, position.y - 22, GetKeyModifierState(event));
				}
				break;
			}
		}
		break;

		case kEventClassKeyboard:
		{
			switch (GetEventKind(event))
			{
				case kEventRawKeyDown:
				{
					UInt32 key_code;
					if (GetEventParameter(event, kEventParamKeyCode, typeUInt32, nullptr, sizeof(UInt32), nullptr, &key_code) == noErr)
					{
						Rml::Core::Input::KeyIdentifier key_identifier = key_identifier_map[key_code & 0xFF];
						int key_modifier_state = GetKeyModifierState(event);

						// Check for a shift-~ to toggle the debugger.
						if (key_identifier == Rml::Core::Input::KI_OEM_3 &&
							key_modifier_state & Rml::Core::Input::KM_SHIFT)
						{
							Rml::Debugger::SetVisible(!Rml::Debugger::IsVisible());
							break;
						}

						if (key_identifier != Rml::Core::Input::KI_UNKNOWN)
							context->ProcessKeyDown(key_identifier, key_modifier_state);

						Rml::Core::Character character = GetCharacterCode(key_identifier, key_modifier_state);
						if (character != Rml::Core::Character::Null)
							context->ProcessTextInput(character);
					}
				}
				break;

				case kEventRawKeyUp:
				{
					UInt32 key_code;
					if (GetEventParameter(event, kEventParamKeyCode, typeUInt32, nullptr, sizeof(UInt32), nullptr, &key_code) == noErr)
					{
						Rml::Core::Input::KeyIdentifier key_identifier = key_identifier_map[key_code & 0xFF];
						int key_modifier_state = GetKeyModifierState(event);

						if (key_identifier != Rml::Core::Input::KI_UNKNOWN)
							context->ProcessKeyUp(key_identifier, key_modifier_state);
					}
				}
				break;
			}
		}
		break;
	}

	return CallNextEventHandler(next_handler, event);
}

static int GetKeyModifierState(EventRef event)
{
	int key_modifier_state = 0;

	UInt32 carbon_key_modifier_state;
	if (GetEventParameter(event, kEventParamKeyModifiers, typeUInt32, nullptr, sizeof(UInt32), nullptr, &carbon_key_modifier_state) == noErr)
	{
		if (carbon_key_modifier_state & KEY_ALT)
			key_modifier_state |= Rml::Core::Input::KM_ALT;
		if (carbon_key_modifier_state & KEY_SHIFT)
			key_modifier_state |= Rml::Core::Input::KM_SHIFT;
		if (carbon_key_modifier_state & KEY_CAPS)
			key_modifier_state |= Rml::Core::Input::KM_CAPSLOCK;
		if (carbon_key_modifier_state & KEY_OPTION)
			key_modifier_state |= Rml::Core::Input::KM_META;
		if (carbon_key_modifier_state & KEY_CTRL)
			key_modifier_state |= Rml::Core::Input::KM_CTRL;
	}

	return key_modifier_state;
}

static void InitialiseKeymap()
{
	// Initialise the key map with default values.
	memset(key_identifier_map, sizeof(key_identifier_map), 0);

	key_identifier_map[0x00] = Rml::Core::Input::KI_A;
	key_identifier_map[0x01] = Rml::Core::Input::KI_S;
	key_identifier_map[0x02] = Rml::Core::Input::KI_D;
	key_identifier_map[0x03] = Rml::Core::Input::KI_F;
	key_identifier_map[0x04] = Rml::Core::Input::KI_H;
	key_identifier_map[0x05] = Rml::Core::Input::KI_G;
	key_identifier_map[0x06] = Rml::Core::Input::KI_Z;
	key_identifier_map[0x07] = Rml::Core::Input::KI_X;
	key_identifier_map[0x08] = Rml::Core::Input::KI_C;
	key_identifier_map[0x09] = Rml::Core::Input::KI_V;
	key_identifier_map[0x0B] = Rml::Core::Input::KI_B;
	key_identifier_map[0x0C] = Rml::Core::Input::KI_Q;
	key_identifier_map[0x0D] = Rml::Core::Input::KI_W;
	key_identifier_map[0x0E] = Rml::Core::Input::KI_E;
	key_identifier_map[0x0F] = Rml::Core::Input::KI_R;
	key_identifier_map[0x10] = Rml::Core::Input::KI_Y;
	key_identifier_map[0x11] = Rml::Core::Input::KI_T;
	key_identifier_map[0x12] = Rml::Core::Input::KI_1;
	key_identifier_map[0x13] = Rml::Core::Input::KI_2;
	key_identifier_map[0x14] = Rml::Core::Input::KI_3;
	key_identifier_map[0x15] = Rml::Core::Input::KI_4;
	key_identifier_map[0x16] = Rml::Core::Input::KI_6;
	key_identifier_map[0x17] = Rml::Core::Input::KI_5;
	key_identifier_map[0x18] = Rml::Core::Input::KI_OEM_PLUS;
	key_identifier_map[0x19] = Rml::Core::Input::KI_9;
	key_identifier_map[0x1A] = Rml::Core::Input::KI_7;
	key_identifier_map[0x1B] = Rml::Core::Input::KI_OEM_MINUS;
	key_identifier_map[0x1C] = Rml::Core::Input::KI_8;
	key_identifier_map[0x1D] = Rml::Core::Input::KI_0;
	key_identifier_map[0x1E] = Rml::Core::Input::KI_OEM_6;
	key_identifier_map[0x1F] = Rml::Core::Input::KI_O;
	key_identifier_map[0x20] = Rml::Core::Input::KI_U;
	key_identifier_map[0x21] = Rml::Core::Input::KI_OEM_4;
	key_identifier_map[0x22] = Rml::Core::Input::KI_I;
	key_identifier_map[0x23] = Rml::Core::Input::KI_P;
	key_identifier_map[0x24] = Rml::Core::Input::KI_RETURN;
	key_identifier_map[0x25] = Rml::Core::Input::KI_L;
	key_identifier_map[0x26] = Rml::Core::Input::KI_J;
	key_identifier_map[0x27] = Rml::Core::Input::KI_OEM_7;
	key_identifier_map[0x28] = Rml::Core::Input::KI_K;
	key_identifier_map[0x29] = Rml::Core::Input::KI_OEM_1;
	key_identifier_map[0x2A] = Rml::Core::Input::KI_OEM_5;
	key_identifier_map[0x2B] = Rml::Core::Input::KI_OEM_COMMA;
	key_identifier_map[0x2C] = Rml::Core::Input::KI_OEM_2;
	key_identifier_map[0x2D] = Rml::Core::Input::KI_N;
	key_identifier_map[0x2E] = Rml::Core::Input::KI_M;
	key_identifier_map[0x2F] = Rml::Core::Input::KI_OEM_PERIOD;
	key_identifier_map[0x30] = Rml::Core::Input::KI_TAB;
	key_identifier_map[0x31] = Rml::Core::Input::KI_SPACE;
	key_identifier_map[0x32] = Rml::Core::Input::KI_OEM_3;
	key_identifier_map[0x33] = Rml::Core::Input::KI_BACK;
	key_identifier_map[0x35] = Rml::Core::Input::KI_ESCAPE;
	key_identifier_map[0x37] = Rml::Core::Input::KI_LMETA;
	key_identifier_map[0x38] = Rml::Core::Input::KI_LSHIFT;
	key_identifier_map[0x39] = Rml::Core::Input::KI_CAPITAL;
	key_identifier_map[0x3A] = Rml::Core::Input::KI_LMENU;
	key_identifier_map[0x3B] = Rml::Core::Input::KI_LCONTROL;
	key_identifier_map[0x41] = Rml::Core::Input::KI_DECIMAL;
	key_identifier_map[0x43] = Rml::Core::Input::KI_MULTIPLY;
	key_identifier_map[0x45] = Rml::Core::Input::KI_ADD;
	key_identifier_map[0x4B] = Rml::Core::Input::KI_DIVIDE;
	key_identifier_map[0x4C] = Rml::Core::Input::KI_NUMPADENTER;
	key_identifier_map[0x4E] = Rml::Core::Input::KI_SUBTRACT;
	key_identifier_map[0x51] = Rml::Core::Input::KI_OEM_PLUS;
	key_identifier_map[0x52] = Rml::Core::Input::KI_NUMPAD0;
	key_identifier_map[0x53] = Rml::Core::Input::KI_NUMPAD1;
	key_identifier_map[0x54] = Rml::Core::Input::KI_NUMPAD2;
	key_identifier_map[0x55] = Rml::Core::Input::KI_NUMPAD3;
	key_identifier_map[0x56] = Rml::Core::Input::KI_NUMPAD4;
	key_identifier_map[0x57] = Rml::Core::Input::KI_NUMPAD5;
	key_identifier_map[0x58] = Rml::Core::Input::KI_NUMPAD6;
	key_identifier_map[0x59] = Rml::Core::Input::KI_NUMPAD7;
	key_identifier_map[0x5B] = Rml::Core::Input::KI_NUMPAD8;
	key_identifier_map[0x5C] = Rml::Core::Input::KI_NUMPAD9;
	key_identifier_map[0x60] = Rml::Core::Input::KI_F5;
	key_identifier_map[0x61] = Rml::Core::Input::KI_F6;
	key_identifier_map[0x62] = Rml::Core::Input::KI_F7;
	key_identifier_map[0x63] = Rml::Core::Input::KI_F3;
	key_identifier_map[0x64] = Rml::Core::Input::KI_F8;
	key_identifier_map[0x65] = Rml::Core::Input::KI_F9;
	key_identifier_map[0x67] = Rml::Core::Input::KI_F11;
	key_identifier_map[0x69] = Rml::Core::Input::KI_F13;
	key_identifier_map[0x6B] = Rml::Core::Input::KI_F14;
	key_identifier_map[0x6D] = Rml::Core::Input::KI_F10;
	key_identifier_map[0x6F] = Rml::Core::Input::KI_F12;
	key_identifier_map[0x71] = Rml::Core::Input::KI_F15;
	key_identifier_map[0x73] = Rml::Core::Input::KI_HOME;
	key_identifier_map[0x74] = Rml::Core::Input::KI_PRIOR;
	key_identifier_map[0x75] = Rml::Core::Input::KI_DELETE;
	key_identifier_map[0x76] = Rml::Core::Input::KI_F4;
	key_identifier_map[0x77] = Rml::Core::Input::KI_END;
	key_identifier_map[0x78] = Rml::Core::Input::KI_F2;
	key_identifier_map[0x79] = Rml::Core::Input::KI_NEXT;
	key_identifier_map[0x7A] = Rml::Core::Input::KI_F1;
	key_identifier_map[0x7B] = Rml::Core::Input::KI_LEFT;
	key_identifier_map[0x7C] = Rml::Core::Input::KI_RIGHT;
	key_identifier_map[0x7D] = Rml::Core::Input::KI_DOWN;
	key_identifier_map[0x7E] = Rml::Core::Input::KI_UP;
}
