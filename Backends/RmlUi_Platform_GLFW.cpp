/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
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

#include "RmlUi_Platform_GLFW.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/StringUtilities.h>
#include <RmlUi/Core/SystemInterface.h>
#include <GLFW/glfw3.h>

SystemInterface_GLFW::SystemInterface_GLFW()
{
	cursor_pointer = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
	cursor_cross = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
	cursor_text = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
}

SystemInterface_GLFW::~SystemInterface_GLFW()
{
	glfwDestroyCursor(cursor_pointer);
	glfwDestroyCursor(cursor_cross);
	glfwDestroyCursor(cursor_text);
}

void SystemInterface_GLFW::SetWindow(GLFWwindow* in_window)
{
	window = in_window;
}

double SystemInterface_GLFW::GetElapsedTime()
{
	return glfwGetTime();
}

void SystemInterface_GLFW::SetMouseCursor(const Rml::String& cursor_name)
{
	GLFWcursor* cursor = nullptr;

	if (cursor_name.empty() || cursor_name == "arrow")
		cursor = nullptr;
	else if (cursor_name == "move")
		cursor = cursor_pointer;
	else if (cursor_name == "pointer")
		cursor = cursor_pointer;
	else if (cursor_name == "resize")
		cursor = cursor_pointer;
	else if (cursor_name == "cross")
		cursor = cursor_cross;
	else if (cursor_name == "text")
		cursor = cursor_text;
	else if (cursor_name == "unavailable")
		cursor = nullptr;
	else if (Rml::StringUtilities::StartsWith(cursor_name, "rmlui-scroll"))
		cursor = cursor_pointer;

	if (window)
		glfwSetCursor(window, cursor);
}

void SystemInterface_GLFW::SetClipboardText(const Rml::String& text_utf8)
{
	if (window)
		glfwSetClipboardString(window, text_utf8.c_str());
}

void SystemInterface_GLFW::GetClipboardText(Rml::String& text)
{
	if (window)
		text = Rml::String(glfwGetClipboardString(window));
}

bool RmlGLFW::ProcessKeyCallback(Rml::Context* context, int key, int action, int mods)
{
	if (!context)
		return true;

	bool result = true;

	switch (action)
	{
	case GLFW_PRESS:
	case GLFW_REPEAT:
		result = context->ProcessKeyDown(RmlGLFW::ConvertKey(key), RmlGLFW::ConvertKeyModifiers(mods));
		if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER)
			result &= context->ProcessTextInput('\n');
		break;
	case GLFW_RELEASE: result = context->ProcessKeyUp(RmlGLFW::ConvertKey(key), RmlGLFW::ConvertKeyModifiers(mods)); break;
	}

	return result;
}
bool RmlGLFW::ProcessCharCallback(Rml::Context* context, unsigned int codepoint)
{
	if (!context)
		return true;

	bool result = context->ProcessTextInput((Rml::Character)codepoint);
	return result;
}

bool RmlGLFW::ProcessCursorEnterCallback(Rml::Context* context, int entered)
{
	if (!context)
		return true;

	bool result = true;
	if (!entered)
		result = context->ProcessMouseLeave();
	return result;
}

bool RmlGLFW::ProcessCursorPosCallback(Rml::Context* context, double xpos, double ypos, int mods)
{
	if (!context)
		return true;

	bool result = context->ProcessMouseMove(int(xpos), int(ypos), RmlGLFW::ConvertKeyModifiers(mods));
	return result;
}

bool RmlGLFW::ProcessMouseButtonCallback(Rml::Context* context, int button, int action, int mods)
{
	if (!context)
		return true;

	bool result = true;

	switch (action)
	{
	case GLFW_PRESS: result = context->ProcessMouseButtonDown(button, RmlGLFW::ConvertKeyModifiers(mods)); break;
	case GLFW_RELEASE: result = context->ProcessMouseButtonUp(button, RmlGLFW::ConvertKeyModifiers(mods)); break;
	}
	return result;
}

bool RmlGLFW::ProcessScrollCallback(Rml::Context* context, double yoffset, int mods)
{
	if (!context)
		return true;

	bool result = context->ProcessMouseWheel(-float(yoffset), RmlGLFW::ConvertKeyModifiers(mods));
	return result;
}

void RmlGLFW::ProcessFramebufferSizeCallback(Rml::Context* context, int width, int height)
{
	if (context)
		context->SetDimensions(Rml::Vector2i(width, height));
}

void RmlGLFW::ProcessContentScaleCallback(Rml::Context* context, float xscale)
{
	if (context)
		context->SetDensityIndependentPixelRatio(xscale);
}

int RmlGLFW::ConvertKeyModifiers(int glfw_mods)
{
	int key_modifier_state = 0;

	if (GLFW_MOD_SHIFT & glfw_mods)
		key_modifier_state |= Rml::Input::KM_SHIFT;

	if (GLFW_MOD_CONTROL & glfw_mods)
		key_modifier_state |= Rml::Input::KM_CTRL;

	if (GLFW_MOD_ALT & glfw_mods)
		key_modifier_state |= Rml::Input::KM_ALT;

	if (GLFW_MOD_CAPS_LOCK & glfw_mods)
		key_modifier_state |= Rml::Input::KM_SCROLLLOCK;

	if (GLFW_MOD_NUM_LOCK & glfw_mods)
		key_modifier_state |= Rml::Input::KM_NUMLOCK;

	return key_modifier_state;
}

Rml::Input::KeyIdentifier RmlGLFW::ConvertKey(int glfw_key)
{
	// clang-format off
	switch (glfw_key)
	{
	case GLFW_KEY_A:             return Rml::Input::KI_A;
	case GLFW_KEY_B:             return Rml::Input::KI_B;
	case GLFW_KEY_C:             return Rml::Input::KI_C;
	case GLFW_KEY_D:             return Rml::Input::KI_D;
	case GLFW_KEY_E:             return Rml::Input::KI_E;
	case GLFW_KEY_F:             return Rml::Input::KI_F;
	case GLFW_KEY_G:             return Rml::Input::KI_G;
	case GLFW_KEY_H:             return Rml::Input::KI_H;
	case GLFW_KEY_I:             return Rml::Input::KI_I;
	case GLFW_KEY_J:             return Rml::Input::KI_J;
	case GLFW_KEY_K:             return Rml::Input::KI_K;
	case GLFW_KEY_L:             return Rml::Input::KI_L;
	case GLFW_KEY_M:             return Rml::Input::KI_M;
	case GLFW_KEY_N:             return Rml::Input::KI_N;
	case GLFW_KEY_O:             return Rml::Input::KI_O;
	case GLFW_KEY_P:             return Rml::Input::KI_P;
	case GLFW_KEY_Q:             return Rml::Input::KI_Q;
	case GLFW_KEY_R:             return Rml::Input::KI_R;
	case GLFW_KEY_S:             return Rml::Input::KI_S;
	case GLFW_KEY_T:             return Rml::Input::KI_T;
	case GLFW_KEY_U:             return Rml::Input::KI_U;
	case GLFW_KEY_V:             return Rml::Input::KI_V;
	case GLFW_KEY_W:             return Rml::Input::KI_W;
	case GLFW_KEY_X:             return Rml::Input::KI_X;
	case GLFW_KEY_Y:             return Rml::Input::KI_Y;
	case GLFW_KEY_Z:             return Rml::Input::KI_Z;

	case GLFW_KEY_0:             return Rml::Input::KI_0;
	case GLFW_KEY_1:             return Rml::Input::KI_1;
	case GLFW_KEY_2:             return Rml::Input::KI_2;
	case GLFW_KEY_3:             return Rml::Input::KI_3;
	case GLFW_KEY_4:             return Rml::Input::KI_4;
	case GLFW_KEY_5:             return Rml::Input::KI_5;
	case GLFW_KEY_6:             return Rml::Input::KI_6;
	case GLFW_KEY_7:             return Rml::Input::KI_7;
	case GLFW_KEY_8:             return Rml::Input::KI_8;
	case GLFW_KEY_9:             return Rml::Input::KI_9;

	case GLFW_KEY_BACKSPACE:     return Rml::Input::KI_BACK;
	case GLFW_KEY_TAB:           return Rml::Input::KI_TAB;

	case GLFW_KEY_ENTER:         return Rml::Input::KI_RETURN;

	case GLFW_KEY_PAUSE:         return Rml::Input::KI_PAUSE;
	case GLFW_KEY_CAPS_LOCK:     return Rml::Input::KI_CAPITAL;

	case GLFW_KEY_ESCAPE:        return Rml::Input::KI_ESCAPE;

	case GLFW_KEY_SPACE:         return Rml::Input::KI_SPACE;
	case GLFW_KEY_PAGE_UP:       return Rml::Input::KI_PRIOR;
	case GLFW_KEY_PAGE_DOWN:     return Rml::Input::KI_NEXT;
	case GLFW_KEY_END:           return Rml::Input::KI_END;
	case GLFW_KEY_HOME:          return Rml::Input::KI_HOME;
	case GLFW_KEY_LEFT:          return Rml::Input::KI_LEFT;
	case GLFW_KEY_UP:            return Rml::Input::KI_UP;
	case GLFW_KEY_RIGHT:         return Rml::Input::KI_RIGHT;
	case GLFW_KEY_DOWN:          return Rml::Input::KI_DOWN;
	case GLFW_KEY_PRINT_SCREEN:  return Rml::Input::KI_SNAPSHOT;
	case GLFW_KEY_INSERT:        return Rml::Input::KI_INSERT;
	case GLFW_KEY_DELETE:        return Rml::Input::KI_DELETE;

	case GLFW_KEY_LEFT_SUPER:    return Rml::Input::KI_LWIN;
	case GLFW_KEY_RIGHT_SUPER:   return Rml::Input::KI_RWIN;

	case GLFW_KEY_KP_0:          return Rml::Input::KI_NUMPAD0;
	case GLFW_KEY_KP_1:          return Rml::Input::KI_NUMPAD1;
	case GLFW_KEY_KP_2:          return Rml::Input::KI_NUMPAD2;
	case GLFW_KEY_KP_3:          return Rml::Input::KI_NUMPAD3;
	case GLFW_KEY_KP_4:          return Rml::Input::KI_NUMPAD4;
	case GLFW_KEY_KP_5:          return Rml::Input::KI_NUMPAD5;
	case GLFW_KEY_KP_6:          return Rml::Input::KI_NUMPAD6;
	case GLFW_KEY_KP_7:          return Rml::Input::KI_NUMPAD7;
	case GLFW_KEY_KP_8:          return Rml::Input::KI_NUMPAD8;
	case GLFW_KEY_KP_9:          return Rml::Input::KI_NUMPAD9;
	case GLFW_KEY_KP_ENTER:      return Rml::Input::KI_NUMPADENTER;
	case GLFW_KEY_KP_MULTIPLY:   return Rml::Input::KI_MULTIPLY;
	case GLFW_KEY_KP_ADD:        return Rml::Input::KI_ADD;
	case GLFW_KEY_KP_SUBTRACT:   return Rml::Input::KI_SUBTRACT;
	case GLFW_KEY_KP_DECIMAL:    return Rml::Input::KI_DECIMAL;
	case GLFW_KEY_KP_DIVIDE:     return Rml::Input::KI_DIVIDE;

	case GLFW_KEY_F1:            return Rml::Input::KI_F1;
	case GLFW_KEY_F2:            return Rml::Input::KI_F2;
	case GLFW_KEY_F3:            return Rml::Input::KI_F3;
	case GLFW_KEY_F4:            return Rml::Input::KI_F4;
	case GLFW_KEY_F5:            return Rml::Input::KI_F5;
	case GLFW_KEY_F6:            return Rml::Input::KI_F6;
	case GLFW_KEY_F7:            return Rml::Input::KI_F7;
	case GLFW_KEY_F8:            return Rml::Input::KI_F8;
	case GLFW_KEY_F9:            return Rml::Input::KI_F9;
	case GLFW_KEY_F10:           return Rml::Input::KI_F10;
	case GLFW_KEY_F11:           return Rml::Input::KI_F11;
	case GLFW_KEY_F12:           return Rml::Input::KI_F12;
	case GLFW_KEY_F13:           return Rml::Input::KI_F13;
	case GLFW_KEY_F14:           return Rml::Input::KI_F14;
	case GLFW_KEY_F15:           return Rml::Input::KI_F15;
	case GLFW_KEY_F16:           return Rml::Input::KI_F16;
	case GLFW_KEY_F17:           return Rml::Input::KI_F17;
	case GLFW_KEY_F18:           return Rml::Input::KI_F18;
	case GLFW_KEY_F19:           return Rml::Input::KI_F19;
	case GLFW_KEY_F20:           return Rml::Input::KI_F20;
	case GLFW_KEY_F21:           return Rml::Input::KI_F21;
	case GLFW_KEY_F22:           return Rml::Input::KI_F22;
	case GLFW_KEY_F23:           return Rml::Input::KI_F23;
	case GLFW_KEY_F24:           return Rml::Input::KI_F24;

	case GLFW_KEY_NUM_LOCK:      return Rml::Input::KI_NUMLOCK;
	case GLFW_KEY_SCROLL_LOCK:   return Rml::Input::KI_SCROLL;

	case GLFW_KEY_LEFT_SHIFT:    return Rml::Input::KI_LSHIFT;
	case GLFW_KEY_LEFT_CONTROL:  return Rml::Input::KI_LCONTROL;
	case GLFW_KEY_RIGHT_SHIFT:   return Rml::Input::KI_RSHIFT;
	case GLFW_KEY_RIGHT_CONTROL: return Rml::Input::KI_RCONTROL;
	case GLFW_KEY_MENU:          return Rml::Input::KI_LMENU;

	case GLFW_KEY_KP_EQUAL:      return Rml::Input::KI_OEM_NEC_EQUAL;
	default: break;
	}
	// clang-format on

	return Rml::Input::KI_UNKNOWN;
}
