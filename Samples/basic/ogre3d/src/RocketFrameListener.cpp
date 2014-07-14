/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#include "RocketFrameListener.h"
#include <Rocket/Core/Context.h>
#include <Rocket/Debugger.h>
#include "RocketApplication.h"

RocketFrameListener::RocketFrameListener(Ogre::RenderWindow* window, Ogre::Camera* camera, Rocket::Core::Context* _context) : ExampleFrameListener(window, camera, true, true)
{
	context = _context;
	running = true;

	BuildKeyMaps();

	mMouse->setEventCallback(this);
	mKeyboard->setEventCallback(this);
}

RocketFrameListener::~RocketFrameListener()
{
}

bool RocketFrameListener::frameStarted(const Ogre::FrameEvent& evt)
{
	return ExampleFrameListener::frameStarted(evt) && running;
}

bool RocketFrameListener::mouseMoved(const OIS::MouseEvent& e)
{
	int key_modifier_state = GetKeyModifierState();

	context->ProcessMouseMove(e.state.X.abs, e.state.Y.abs, key_modifier_state);
	if (e.state.Z.rel != 0)
		context->ProcessMouseWheel(e.state.Z.rel / -120, key_modifier_state);

	return true;
}

bool RocketFrameListener::mousePressed(const OIS::MouseEvent& ROCKET_UNUSED_PARAMETER(e), OIS::MouseButtonID id)
{
	ROCKET_UNUSED(e);

	context->ProcessMouseButtonDown((int) id, GetKeyModifierState());
	return true;
}

bool RocketFrameListener::mouseReleased(const OIS::MouseEvent& ROCKET_UNUSED_PARAMETER(e), OIS::MouseButtonID id)
{
	ROCKET_UNUSED(e);

	context->ProcessMouseButtonUp((int) id, GetKeyModifierState());
	return true;
}

bool RocketFrameListener::keyPressed(const OIS::KeyEvent& e)
{
	Rocket::Core::Input::KeyIdentifier key_identifier = key_identifiers[e.key];

	// Toggle the debugger on a shift-~ press.
	if (key_identifier == Rocket::Core::Input::KI_OEM_3 &&
		(GetKeyModifierState() & Rocket::Core::Input::KM_SHIFT))
	{
		Rocket::Debugger::SetVisible(!Rocket::Debugger::IsVisible());
		return true;
	}

	if (key_identifier != Rocket::Core::Input::KI_UNKNOWN)
		context->ProcessKeyDown(key_identifier, GetKeyModifierState());

	// Send through the ASCII value as text input if it is printable.
	if (e.text >= 32)
		context->ProcessTextInput((Rocket::Core::word) e.text);
	else if (key_identifier == Rocket::Core::Input::KI_RETURN)
		context->ProcessTextInput((Rocket::Core::word) '\n');

	return true;
}

bool RocketFrameListener::keyReleased(const OIS::KeyEvent& e)
{
	Rocket::Core::Input::KeyIdentifier key_identifier = key_identifiers[e.key];

	if (key_identifier != Rocket::Core::Input::KI_UNKNOWN)
		context->ProcessKeyUp(key_identifier, GetKeyModifierState());

	if (e.key == OIS::KC_ESCAPE)
		running = false;

	return true;
}

void RocketFrameListener::BuildKeyMaps()
{
	key_identifiers[OIS::KC_UNASSIGNED] = Rocket::Core::Input::KI_UNKNOWN;
	key_identifiers[OIS::KC_ESCAPE] = Rocket::Core::Input::KI_ESCAPE;
	key_identifiers[OIS::KC_1] = Rocket::Core::Input::KI_1;
	key_identifiers[OIS::KC_2] = Rocket::Core::Input::KI_2;
	key_identifiers[OIS::KC_3] = Rocket::Core::Input::KI_3;
	key_identifiers[OIS::KC_4] = Rocket::Core::Input::KI_4;
	key_identifiers[OIS::KC_5] = Rocket::Core::Input::KI_5;
	key_identifiers[OIS::KC_6] = Rocket::Core::Input::KI_6;
	key_identifiers[OIS::KC_7] = Rocket::Core::Input::KI_7;
	key_identifiers[OIS::KC_8] = Rocket::Core::Input::KI_8;
	key_identifiers[OIS::KC_9] = Rocket::Core::Input::KI_9;
	key_identifiers[OIS::KC_0] = Rocket::Core::Input::KI_0;
	key_identifiers[OIS::KC_MINUS] = Rocket::Core::Input::KI_OEM_MINUS;
	key_identifiers[OIS::KC_EQUALS] = Rocket::Core::Input::KI_OEM_PLUS;
	key_identifiers[OIS::KC_BACK] = Rocket::Core::Input::KI_BACK;
	key_identifiers[OIS::KC_TAB] = Rocket::Core::Input::KI_TAB;
	key_identifiers[OIS::KC_Q] = Rocket::Core::Input::KI_Q;
	key_identifiers[OIS::KC_W] = Rocket::Core::Input::KI_W;
	key_identifiers[OIS::KC_E] = Rocket::Core::Input::KI_E;
	key_identifiers[OIS::KC_R] = Rocket::Core::Input::KI_R;
	key_identifiers[OIS::KC_T] = Rocket::Core::Input::KI_T;
	key_identifiers[OIS::KC_Y] = Rocket::Core::Input::KI_Y;
	key_identifiers[OIS::KC_U] = Rocket::Core::Input::KI_U;
	key_identifiers[OIS::KC_I] = Rocket::Core::Input::KI_I;
	key_identifiers[OIS::KC_O] = Rocket::Core::Input::KI_O;
	key_identifiers[OIS::KC_P] = Rocket::Core::Input::KI_P;
	key_identifiers[OIS::KC_LBRACKET] = Rocket::Core::Input::KI_OEM_4;
	key_identifiers[OIS::KC_RBRACKET] = Rocket::Core::Input::KI_OEM_6;
	key_identifiers[OIS::KC_RETURN] = Rocket::Core::Input::KI_RETURN;
	key_identifiers[OIS::KC_LCONTROL] = Rocket::Core::Input::KI_LCONTROL;
	key_identifiers[OIS::KC_A] = Rocket::Core::Input::KI_A;
	key_identifiers[OIS::KC_S] = Rocket::Core::Input::KI_S;
	key_identifiers[OIS::KC_D] = Rocket::Core::Input::KI_D;
	key_identifiers[OIS::KC_F] = Rocket::Core::Input::KI_F;
	key_identifiers[OIS::KC_G] = Rocket::Core::Input::KI_G;
	key_identifiers[OIS::KC_H] = Rocket::Core::Input::KI_H;
	key_identifiers[OIS::KC_J] = Rocket::Core::Input::KI_J;
	key_identifiers[OIS::KC_K] = Rocket::Core::Input::KI_K;
	key_identifiers[OIS::KC_L] = Rocket::Core::Input::KI_L;
	key_identifiers[OIS::KC_SEMICOLON] = Rocket::Core::Input::KI_OEM_1;
	key_identifiers[OIS::KC_APOSTROPHE] = Rocket::Core::Input::KI_OEM_7;
	key_identifiers[OIS::KC_GRAVE] = Rocket::Core::Input::KI_OEM_3;
	key_identifiers[OIS::KC_LSHIFT] = Rocket::Core::Input::KI_LSHIFT;
	key_identifiers[OIS::KC_BACKSLASH] = Rocket::Core::Input::KI_OEM_5;
	key_identifiers[OIS::KC_Z] = Rocket::Core::Input::KI_Z;
	key_identifiers[OIS::KC_X] = Rocket::Core::Input::KI_X;
	key_identifiers[OIS::KC_C] = Rocket::Core::Input::KI_C;
	key_identifiers[OIS::KC_V] = Rocket::Core::Input::KI_V;
	key_identifiers[OIS::KC_B] = Rocket::Core::Input::KI_B;
	key_identifiers[OIS::KC_N] = Rocket::Core::Input::KI_N;
	key_identifiers[OIS::KC_M] = Rocket::Core::Input::KI_M;
	key_identifiers[OIS::KC_COMMA] = Rocket::Core::Input::KI_OEM_COMMA;
	key_identifiers[OIS::KC_PERIOD] = Rocket::Core::Input::KI_OEM_PERIOD;
	key_identifiers[OIS::KC_SLASH] = Rocket::Core::Input::KI_OEM_2;
	key_identifiers[OIS::KC_RSHIFT] = Rocket::Core::Input::KI_RSHIFT;
	key_identifiers[OIS::KC_MULTIPLY] = Rocket::Core::Input::KI_MULTIPLY;
	key_identifiers[OIS::KC_LMENU] = Rocket::Core::Input::KI_LMENU;
	key_identifiers[OIS::KC_SPACE] = Rocket::Core::Input::KI_SPACE;
	key_identifiers[OIS::KC_CAPITAL] = Rocket::Core::Input::KI_CAPITAL;
	key_identifiers[OIS::KC_F1] = Rocket::Core::Input::KI_F1;
	key_identifiers[OIS::KC_F2] = Rocket::Core::Input::KI_F2;
	key_identifiers[OIS::KC_F3] = Rocket::Core::Input::KI_F3;
	key_identifiers[OIS::KC_F4] = Rocket::Core::Input::KI_F4;
	key_identifiers[OIS::KC_F5] = Rocket::Core::Input::KI_F5;
	key_identifiers[OIS::KC_F6] = Rocket::Core::Input::KI_F6;
	key_identifiers[OIS::KC_F7] = Rocket::Core::Input::KI_F7;
	key_identifiers[OIS::KC_F8] = Rocket::Core::Input::KI_F8;
	key_identifiers[OIS::KC_F9] = Rocket::Core::Input::KI_F9;
	key_identifiers[OIS::KC_F10] = Rocket::Core::Input::KI_F10;
	key_identifiers[OIS::KC_NUMLOCK] = Rocket::Core::Input::KI_NUMLOCK;
	key_identifiers[OIS::KC_SCROLL] = Rocket::Core::Input::KI_SCROLL;
	key_identifiers[OIS::KC_NUMPAD7] = Rocket::Core::Input::KI_7;
	key_identifiers[OIS::KC_NUMPAD8] = Rocket::Core::Input::KI_8;
	key_identifiers[OIS::KC_NUMPAD9] = Rocket::Core::Input::KI_9;
	key_identifiers[OIS::KC_SUBTRACT] = Rocket::Core::Input::KI_SUBTRACT;
	key_identifiers[OIS::KC_NUMPAD4] = Rocket::Core::Input::KI_4;
	key_identifiers[OIS::KC_NUMPAD5] = Rocket::Core::Input::KI_5;
	key_identifiers[OIS::KC_NUMPAD6] = Rocket::Core::Input::KI_6;
	key_identifiers[OIS::KC_ADD] = Rocket::Core::Input::KI_ADD;
	key_identifiers[OIS::KC_NUMPAD1] = Rocket::Core::Input::KI_1;
	key_identifiers[OIS::KC_NUMPAD2] = Rocket::Core::Input::KI_2;
	key_identifiers[OIS::KC_NUMPAD3] = Rocket::Core::Input::KI_3;
	key_identifiers[OIS::KC_NUMPAD0] = Rocket::Core::Input::KI_0;
	key_identifiers[OIS::KC_DECIMAL] = Rocket::Core::Input::KI_DECIMAL;
	key_identifiers[OIS::KC_OEM_102] = Rocket::Core::Input::KI_OEM_102;
	key_identifiers[OIS::KC_F11] = Rocket::Core::Input::KI_F11;
	key_identifiers[OIS::KC_F12] = Rocket::Core::Input::KI_F12;
	key_identifiers[OIS::KC_F13] = Rocket::Core::Input::KI_F13;
	key_identifiers[OIS::KC_F14] = Rocket::Core::Input::KI_F14;
	key_identifiers[OIS::KC_F15] = Rocket::Core::Input::KI_F15;
	key_identifiers[OIS::KC_KANA] = Rocket::Core::Input::KI_KANA;
	key_identifiers[OIS::KC_ABNT_C1] = Rocket::Core::Input::KI_UNKNOWN;
	key_identifiers[OIS::KC_CONVERT] = Rocket::Core::Input::KI_CONVERT;
	key_identifiers[OIS::KC_NOCONVERT] = Rocket::Core::Input::KI_NONCONVERT;
	key_identifiers[OIS::KC_YEN] = Rocket::Core::Input::KI_UNKNOWN;
	key_identifiers[OIS::KC_ABNT_C2] = Rocket::Core::Input::KI_UNKNOWN;
	key_identifiers[OIS::KC_NUMPADEQUALS] = Rocket::Core::Input::KI_OEM_NEC_EQUAL;
	key_identifiers[OIS::KC_PREVTRACK] = Rocket::Core::Input::KI_MEDIA_PREV_TRACK;
	key_identifiers[OIS::KC_AT] = Rocket::Core::Input::KI_UNKNOWN;
	key_identifiers[OIS::KC_COLON] = Rocket::Core::Input::KI_OEM_1;
	key_identifiers[OIS::KC_UNDERLINE] = Rocket::Core::Input::KI_OEM_MINUS;
	key_identifiers[OIS::KC_KANJI] = Rocket::Core::Input::KI_KANJI;
	key_identifiers[OIS::KC_STOP] = Rocket::Core::Input::KI_UNKNOWN;
	key_identifiers[OIS::KC_AX] = Rocket::Core::Input::KI_OEM_AX;
	key_identifiers[OIS::KC_UNLABELED] = Rocket::Core::Input::KI_UNKNOWN;
	key_identifiers[OIS::KC_NEXTTRACK] = Rocket::Core::Input::KI_MEDIA_NEXT_TRACK;
	key_identifiers[OIS::KC_NUMPADENTER] = Rocket::Core::Input::KI_NUMPADENTER;
	key_identifiers[OIS::KC_RCONTROL] = Rocket::Core::Input::KI_RCONTROL;
	key_identifiers[OIS::KC_MUTE] = Rocket::Core::Input::KI_VOLUME_MUTE;
	key_identifiers[OIS::KC_CALCULATOR] = Rocket::Core::Input::KI_UNKNOWN;
	key_identifiers[OIS::KC_PLAYPAUSE] = Rocket::Core::Input::KI_MEDIA_PLAY_PAUSE;
	key_identifiers[OIS::KC_MEDIASTOP] = Rocket::Core::Input::KI_MEDIA_STOP;
	key_identifiers[OIS::KC_VOLUMEDOWN] = Rocket::Core::Input::KI_VOLUME_DOWN;
	key_identifiers[OIS::KC_VOLUMEUP] = Rocket::Core::Input::KI_VOLUME_UP;
	key_identifiers[OIS::KC_WEBHOME] = Rocket::Core::Input::KI_BROWSER_HOME;
	key_identifiers[OIS::KC_NUMPADCOMMA] = Rocket::Core::Input::KI_SEPARATOR;
	key_identifiers[OIS::KC_DIVIDE] = Rocket::Core::Input::KI_DIVIDE;
	key_identifiers[OIS::KC_SYSRQ] = Rocket::Core::Input::KI_SNAPSHOT;
	key_identifiers[OIS::KC_RMENU] = Rocket::Core::Input::KI_RMENU;
	key_identifiers[OIS::KC_PAUSE] = Rocket::Core::Input::KI_PAUSE;
	key_identifiers[OIS::KC_HOME] = Rocket::Core::Input::KI_HOME;
	key_identifiers[OIS::KC_UP] = Rocket::Core::Input::KI_UP;
	key_identifiers[OIS::KC_PGUP] = Rocket::Core::Input::KI_PRIOR;
	key_identifiers[OIS::KC_LEFT] = Rocket::Core::Input::KI_LEFT;
	key_identifiers[OIS::KC_RIGHT] = Rocket::Core::Input::KI_RIGHT;
	key_identifiers[OIS::KC_END] = Rocket::Core::Input::KI_END;
	key_identifiers[OIS::KC_DOWN] = Rocket::Core::Input::KI_DOWN;
	key_identifiers[OIS::KC_PGDOWN] = Rocket::Core::Input::KI_NEXT;
	key_identifiers[OIS::KC_INSERT] = Rocket::Core::Input::KI_INSERT;
	key_identifiers[OIS::KC_DELETE] = Rocket::Core::Input::KI_DELETE;
	key_identifiers[OIS::KC_LWIN] = Rocket::Core::Input::KI_LWIN;
	key_identifiers[OIS::KC_RWIN] = Rocket::Core::Input::KI_RWIN;
	key_identifiers[OIS::KC_APPS] = Rocket::Core::Input::KI_APPS;
	key_identifiers[OIS::KC_POWER] = Rocket::Core::Input::KI_POWER;
	key_identifiers[OIS::KC_SLEEP] = Rocket::Core::Input::KI_SLEEP;
	key_identifiers[OIS::KC_WAKE] = Rocket::Core::Input::KI_WAKE;
	key_identifiers[OIS::KC_WEBSEARCH] = Rocket::Core::Input::KI_BROWSER_SEARCH;
	key_identifiers[OIS::KC_WEBFAVORITES] = Rocket::Core::Input::KI_BROWSER_FAVORITES;
	key_identifiers[OIS::KC_WEBREFRESH] = Rocket::Core::Input::KI_BROWSER_REFRESH;
	key_identifiers[OIS::KC_WEBSTOP] = Rocket::Core::Input::KI_BROWSER_STOP;
	key_identifiers[OIS::KC_WEBFORWARD] = Rocket::Core::Input::KI_BROWSER_FORWARD;
	key_identifiers[OIS::KC_WEBBACK] = Rocket::Core::Input::KI_BROWSER_BACK;
	key_identifiers[OIS::KC_MYCOMPUTER] = Rocket::Core::Input::KI_UNKNOWN;
	key_identifiers[OIS::KC_MAIL] = Rocket::Core::Input::KI_LAUNCH_MAIL;
	key_identifiers[OIS::KC_MEDIASELECT] = Rocket::Core::Input::KI_LAUNCH_MEDIA_SELECT;
}

int RocketFrameListener::GetKeyModifierState()
{
	int modifier_state = 0;

	if (mKeyboard->isModifierDown(OIS::Keyboard::Ctrl))
		modifier_state |= Rocket::Core::Input::KM_CTRL;
	if (mKeyboard->isModifierDown(OIS::Keyboard::Shift))
		modifier_state |= Rocket::Core::Input::KM_SHIFT;
	if (mKeyboard->isModifierDown(OIS::Keyboard::Alt))
		modifier_state |= Rocket::Core::Input::KM_ALT;

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32

	if (GetKeyState(VK_CAPITAL) > 0)
		modifier_state |= Rocket::Core::Input::KM_CAPSLOCK;
	if (GetKeyState(VK_NUMLOCK) > 0)
		modifier_state |= Rocket::Core::Input::KM_NUMLOCK;
	if (GetKeyState(VK_SCROLL) > 0)
		modifier_state |= Rocket::Core::Input::KM_SCROLLLOCK;

#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE

	UInt32 key_modifiers = GetCurrentEventKeyModifiers();
	if (key_modifiers & (1 << alphaLockBit))
		modifier_state |= Rocket::Core::Input::KM_CAPSLOCK;

#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX

	XKeyboardState keyboard_state;
	XGetKeyboardControl(DISPLAY!, &keyboard_state);

	if (keyboard_state.led_mask & (1 << 0))
		modifier_state |= Rocket::Core::Input::KM_CAPSLOCK;
	if (keyboard_state.led_mask & (1 << 1))
		modifier_state |= Rocket::Core::Input::KM_NUMLOCK;
	if (keyboard_state.led_mask & (1 << 2))
		modifier_state |= Rocket::Core::Input::KM_SCROLLLOCK;

#endif

	return modifier_state;
}
