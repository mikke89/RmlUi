/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 Nuno Silva
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
#include <Rocket/Core.h>
#include "SystemInterfaceSDL2.h"

Rocket::Core::Input::KeyIdentifier RocketSDL2SystemInterface::TranslateKey(SDL_Keycode sdlkey)
{
    using namespace Rocket::Core::Input;


    switch(sdlkey) {
        case SDLK_UNKNOWN:
            return KI_UNKNOWN;
            break;
        case SDLK_SPACE:
            return KI_SPACE;
            break;
        case SDLK_0:
            return KI_0;
            break;
        case SDLK_1:
            return KI_1;
            break;
        case SDLK_2:
            return KI_2;
            break;
        case SDLK_3:
            return KI_3;
            break;
        case SDLK_4:
            return KI_4;
            break;
        case SDLK_5:
            return KI_5;
            break;
        case SDLK_6:
            return KI_6;
            break;
        case SDLK_7:
            return KI_7;
            break;
        case SDLK_8:
            return KI_8;
            break;
        case SDLK_9:
            return KI_9;
            break;
        case SDLK_a:
            return KI_A;
            break;
        case SDLK_b:
            return KI_B;
            break;
        case SDLK_c:
            return KI_C;
            break;
        case SDLK_d:
            return KI_D;
            break;
        case SDLK_e:
            return KI_E;
            break;
        case SDLK_f:
            return KI_F;
            break;
        case SDLK_g:
            return KI_G;
            break;
        case SDLK_h:
            return KI_H;
            break;
        case SDLK_i:
            return KI_I;
            break;
        case SDLK_j:
            return KI_J;
            break;
        case SDLK_k:
            return KI_K;
            break;
        case SDLK_l:
            return KI_L;
            break;
        case SDLK_m:
            return KI_M;
            break;
        case SDLK_n:
            return KI_N;
            break;
        case SDLK_o:
            return KI_O;
            break;
        case SDLK_p:
            return KI_P;
            break;
        case SDLK_q:
            return KI_Q;
            break;
        case SDLK_r:
            return KI_R;
            break;
        case SDLK_s:
            return KI_S;
            break;
        case SDLK_t:
            return KI_T;
            break;
        case SDLK_u:
            return KI_U;
            break;
        case SDLK_v:
            return KI_V;
            break;
        case SDLK_w:
            return KI_W;
            break;
        case SDLK_x:
            return KI_X;
            break;
        case SDLK_y:
            return KI_Y;
            break;
        case SDLK_z:
            return KI_Z;
            break;
        case SDLK_SEMICOLON:
            return KI_OEM_1;
            break;
        case SDLK_PLUS:
            return KI_OEM_PLUS;
            break;
        case SDLK_COMMA:
            return KI_OEM_COMMA;
            break;
        case SDLK_MINUS:
            return KI_OEM_MINUS;
            break;
        case SDLK_PERIOD:
            return KI_OEM_PERIOD;
            break;
        case SDLK_SLASH:
            return KI_OEM_2;
            break;
        case SDLK_BACKQUOTE:
            return KI_OEM_3;
            break;
        case SDLK_LEFTBRACKET:
            return KI_OEM_4;
            break;
        case SDLK_BACKSLASH:
            return KI_OEM_5;
            break;
        case SDLK_RIGHTBRACKET:
            return KI_OEM_6;
            break;
        case SDLK_QUOTEDBL:
            return KI_OEM_7;
            break;
        case SDLK_KP_0:
            return KI_NUMPAD0;
            break;
        case SDLK_KP_1:
            return KI_NUMPAD1;
            break;
        case SDLK_KP_2:
            return KI_NUMPAD2;
            break;
        case SDLK_KP_3:
            return KI_NUMPAD3;
            break;
        case SDLK_KP_4:
            return KI_NUMPAD4;
            break;
        case SDLK_KP_5:
            return KI_NUMPAD5;
            break;
        case SDLK_KP_6:
            return KI_NUMPAD6;
            break;
        case SDLK_KP_7:
            return KI_NUMPAD7;
            break;
        case SDLK_KP_8:
            return KI_NUMPAD8;
            break;
        case SDLK_KP_9:
            return KI_NUMPAD9;
            break;
        case SDLK_KP_ENTER:
            return KI_NUMPADENTER;
            break;
        case SDLK_KP_MULTIPLY:
            return KI_MULTIPLY;
            break;
        case SDLK_KP_PLUS:
            return KI_ADD;
            break;
        case SDLK_KP_MINUS:
            return KI_SUBTRACT;
            break;
        case SDLK_KP_PERIOD:
            return KI_DECIMAL;
            break;
        case SDLK_KP_DIVIDE:
            return KI_DIVIDE;
            break;
        case SDLK_KP_EQUALS:
            return KI_OEM_NEC_EQUAL;
            break;
        case SDLK_BACKSPACE:
            return KI_BACK;
            break;
        case SDLK_TAB:
            return KI_TAB;
            break;
        case SDLK_CLEAR:
            return KI_CLEAR;
            break;
        case SDLK_RETURN:
            return KI_RETURN;
            break;
        case SDLK_PAUSE:
            return KI_PAUSE;
            break;
        case SDLK_CAPSLOCK:
            return KI_CAPITAL;
            break;
        case SDLK_PAGEUP:
            return KI_PRIOR;
            break;
        case SDLK_PAGEDOWN:
            return KI_NEXT;
            break;
        case SDLK_END:
            return KI_END;
            break;
        case SDLK_HOME:
            return KI_HOME;
            break;
        case SDLK_LEFT:
            return KI_LEFT;
            break;
        case SDLK_UP:
            return KI_UP;
            break;
        case SDLK_RIGHT:
            return KI_RIGHT;
            break;
        case SDLK_DOWN:
            return KI_DOWN;
            break;
        case SDLK_INSERT:
            return KI_INSERT;
            break;
        case SDLK_DELETE:
            return KI_DELETE;
            break;
        case SDLK_HELP:
            return KI_HELP;
            break;
        case SDLK_F1:
            return KI_F1;
            break;
        case SDLK_F2:
            return KI_F2;
            break;
        case SDLK_F3:
            return KI_F3;
            break;
        case SDLK_F4:
            return KI_F4;
            break;
        case SDLK_F5:
            return KI_F5;
            break;
        case SDLK_F6:
            return KI_F6;
            break;
        case SDLK_F7:
            return KI_F7;
            break;
        case SDLK_F8:
            return KI_F8;
            break;
        case SDLK_F9:
            return KI_F9;
            break;
        case SDLK_F10:
            return KI_F10;
            break;
        case SDLK_F11:
            return KI_F11;
            break;
        case SDLK_F12:
            return KI_F12;
            break;
        case SDLK_F13:
            return KI_F13;
            break;
        case SDLK_F14:
            return KI_F14;
            break;
        case SDLK_F15:
            return KI_F15;
            break;
        case SDLK_NUMLOCKCLEAR:
            return KI_NUMLOCK;
            break;
        case SDLK_SCROLLLOCK:
            return KI_SCROLL;
            break;
        case SDLK_LSHIFT:
            return KI_LSHIFT;
            break;
        case SDLK_RSHIFT:
            return KI_RSHIFT;
            break;
        case SDLK_LCTRL:
            return KI_LCONTROL;
            break;
        case SDLK_RCTRL:
            return KI_RCONTROL;
            break;
        case SDLK_LALT:
            return KI_LMENU;
            break;
        case SDLK_RALT:
            return KI_RMENU;
            break;
        case SDLK_LGUI:
            return KI_LMETA;
            break;
        case SDLK_RGUI:
            return KI_RMETA;
            break;
        /*case SDLK_LSUPER:
            return KI_LWIN;
            break;
        case SDLK_RSUPER:
            return KI_RWIN;
            break;*/
        default:
            return KI_UNKNOWN;
            break;
    }
}

int RocketSDL2SystemInterface::TranslateMouseButton(Uint8 button)
{
    switch(button)
    {
        case SDL_BUTTON_LEFT:
            return 0;
        case SDL_BUTTON_RIGHT:
            return 1;
        case SDL_BUTTON_MIDDLE:
            return 2;
        default:
            return 3;
    }
}

int RocketSDL2SystemInterface::GetKeyModifiers()
{
    SDL_Keymod sdlMods = SDL_GetModState();

    int retval = 0;

    if(sdlMods & KMOD_CTRL)
        retval |= Rocket::Core::Input::KM_CTRL;

    if(sdlMods & KMOD_SHIFT)
        retval |= Rocket::Core::Input::KM_SHIFT;

    if(sdlMods & KMOD_ALT)
        retval |= Rocket::Core::Input::KM_ALT;

    return retval;
}

float RocketSDL2SystemInterface::GetElapsedTime()
{
	return SDL_GetTicks() / 1000;
}

bool RocketSDL2SystemInterface::LogMessage(Rocket::Core::Log::Type type, const Rocket::Core::String& message)
{
	std::string Type;

	switch(type)
	{
	case Rocket::Core::Log::LT_ALWAYS:
		Type = "[Always]";
		break;
	case Rocket::Core::Log::LT_ERROR:
		Type = "[Error]";
		break;
	case Rocket::Core::Log::LT_ASSERT:
		Type = "[Assert]";
		break;
	case Rocket::Core::Log::LT_WARNING:
		Type = "[Warning]";
		break;
	case Rocket::Core::Log::LT_INFO:
		Type = "[Info]";
		break;
	case Rocket::Core::Log::LT_DEBUG:
		Type = "[Debug]";
		break;
    case Rocket::Core::Log::LT_MAX:
        break;
	};

	printf("%s - %s\n", Type.c_str(), message.CString());

	return true;
};
