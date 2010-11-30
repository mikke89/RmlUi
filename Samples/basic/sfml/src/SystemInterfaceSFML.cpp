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
#include "SystemInterfaceSFML.h"

int RocketSFMLSystemInterface::GetKeyModifiers(sf::Window *Window)
{
	int Modifiers = 0;

	if(Window->GetInput().IsKeyDown(sf::Key::LShift) ||
		Window->GetInput().IsKeyDown(sf::Key::RShift))
		Modifiers |= Rocket::Core::Input::KM_SHIFT;

	if(Window->GetInput().IsKeyDown(sf::Key::LControl) ||
		Window->GetInput().IsKeyDown(sf::Key::RControl))
		Modifiers |= Rocket::Core::Input::KM_CTRL;

	if(Window->GetInput().IsKeyDown(sf::Key::LAlt) ||
		Window->GetInput().IsKeyDown(sf::Key::RAlt))
		Modifiers |= Rocket::Core::Input::KM_ALT;

	return Modifiers;
};

Rocket::Core::Input::KeyIdentifier RocketSFMLSystemInterface::TranslateKey(sf::Key::Code Key)
{
	switch(Key)
	{
	case sf::Key::A:
		return Rocket::Core::Input::KI_A;
		break;
	case sf::Key::B:
		return Rocket::Core::Input::KI_B;
		break;
	case sf::Key::C:
		return Rocket::Core::Input::KI_C;
		break;
	case sf::Key::D:
		return Rocket::Core::Input::KI_D;
		break;
	case sf::Key::E:
		return Rocket::Core::Input::KI_E;
		break;
	case sf::Key::F:
		return Rocket::Core::Input::KI_F;
		break;
	case sf::Key::G:
		return Rocket::Core::Input::KI_G;
		break;
	case sf::Key::H:
		return Rocket::Core::Input::KI_H;
		break;
	case sf::Key::I:
		return Rocket::Core::Input::KI_I;
		break;
	case sf::Key::J:
		return Rocket::Core::Input::KI_J;
		break;
	case sf::Key::K:
		return Rocket::Core::Input::KI_K;
		break;
	case sf::Key::L:
		return Rocket::Core::Input::KI_L;
		break;
	case sf::Key::M:
		return Rocket::Core::Input::KI_M;
		break;
	case sf::Key::N:
		return Rocket::Core::Input::KI_N;
		break;
	case sf::Key::O:
		return Rocket::Core::Input::KI_O;
		break;
	case sf::Key::P:
		return Rocket::Core::Input::KI_P;
		break;
	case sf::Key::Q:
		return Rocket::Core::Input::KI_Q;
		break;
	case sf::Key::R:
		return Rocket::Core::Input::KI_R;
		break;
	case sf::Key::S:
		return Rocket::Core::Input::KI_S;
		break;
	case sf::Key::T:
		return Rocket::Core::Input::KI_T;
		break;
	case sf::Key::U:
		return Rocket::Core::Input::KI_U;
		break;
	case sf::Key::V:
		return Rocket::Core::Input::KI_V;
		break;
	case sf::Key::W:
		return Rocket::Core::Input::KI_W;
		break;
	case sf::Key::X:
		return Rocket::Core::Input::KI_X;
		break;
	case sf::Key::Y:
		return Rocket::Core::Input::KI_Y;
		break;
	case sf::Key::Z:
		return Rocket::Core::Input::KI_Z;
		break;
	case sf::Key::Num0:
		return Rocket::Core::Input::KI_0;
		break;
	case sf::Key::Num1:
		return Rocket::Core::Input::KI_1;
		break;
	case sf::Key::Num2:
		return Rocket::Core::Input::KI_2;
		break;
	case sf::Key::Num3:
		return Rocket::Core::Input::KI_3;
		break;
	case sf::Key::Num4:
		return Rocket::Core::Input::KI_4;
		break;
	case sf::Key::Num5:
		return Rocket::Core::Input::KI_5;
		break;
	case sf::Key::Num6:
		return Rocket::Core::Input::KI_6;
		break;
	case sf::Key::Num7:
		return Rocket::Core::Input::KI_7;
		break;
	case sf::Key::Num8:
		return Rocket::Core::Input::KI_8;
		break;
	case sf::Key::Num9:
		return Rocket::Core::Input::KI_9;
		break;
	case sf::Key::Numpad0:
		return Rocket::Core::Input::KI_NUMPAD0;
		break;
	case sf::Key::Numpad1:
		return Rocket::Core::Input::KI_NUMPAD1;
		break;
	case sf::Key::Numpad2:
		return Rocket::Core::Input::KI_NUMPAD2;
		break;
	case sf::Key::Numpad3:
		return Rocket::Core::Input::KI_NUMPAD3;
		break;
	case sf::Key::Numpad4:
		return Rocket::Core::Input::KI_NUMPAD4;
		break;
	case sf::Key::Numpad5:
		return Rocket::Core::Input::KI_NUMPAD5;
		break;
	case sf::Key::Numpad6:
		return Rocket::Core::Input::KI_NUMPAD6;
		break;
	case sf::Key::Numpad7:
		return Rocket::Core::Input::KI_NUMPAD7;
		break;
	case sf::Key::Numpad8:
		return Rocket::Core::Input::KI_NUMPAD8;
		break;
	case sf::Key::Numpad9:
		return Rocket::Core::Input::KI_NUMPAD9;
		break;
	case sf::Key::Left:
		return Rocket::Core::Input::KI_LEFT;
		break;
	case sf::Key::Right:
		return Rocket::Core::Input::KI_RIGHT;
		break;
	case sf::Key::Up:
		return Rocket::Core::Input::KI_UP;
		break;
	case sf::Key::Down:
		return Rocket::Core::Input::KI_DOWN;
		break;
	case sf::Key::Add:
		return Rocket::Core::Input::KI_ADD;
		break;
	case sf::Key::Back:
		return Rocket::Core::Input::KI_BACK;
		break;
	case sf::Key::Delete:
		return Rocket::Core::Input::KI_DELETE;
		break;
	case sf::Key::Divide:
		return Rocket::Core::Input::KI_DIVIDE;
		break;
	case sf::Key::End:
		return Rocket::Core::Input::KI_END;
		break;
	case sf::Key::Escape:
		return Rocket::Core::Input::KI_ESCAPE;
		break;
	case sf::Key::F1:
		return Rocket::Core::Input::KI_F1;
		break;
	case sf::Key::F2:
		return Rocket::Core::Input::KI_F2;
		break;
	case sf::Key::F3:
		return Rocket::Core::Input::KI_F3;
		break;
	case sf::Key::F4:
		return Rocket::Core::Input::KI_F4;
		break;
	case sf::Key::F5:
		return Rocket::Core::Input::KI_F5;
		break;
	case sf::Key::F6:
		return Rocket::Core::Input::KI_F6;
		break;
	case sf::Key::F7:
		return Rocket::Core::Input::KI_F7;
		break;
	case sf::Key::F8:
		return Rocket::Core::Input::KI_F8;
		break;
	case sf::Key::F9:
		return Rocket::Core::Input::KI_F9;
		break;
	case sf::Key::F10:
		return Rocket::Core::Input::KI_F10;
		break;
	case sf::Key::F11:
		return Rocket::Core::Input::KI_F11;
		break;
	case sf::Key::F12:
		return Rocket::Core::Input::KI_F12;
		break;
	case sf::Key::F13:
		return Rocket::Core::Input::KI_F13;
		break;
	case sf::Key::F14:
		return Rocket::Core::Input::KI_F14;
		break;
	case sf::Key::F15:
		return Rocket::Core::Input::KI_F15;
		break;
	case sf::Key::Home:
		return Rocket::Core::Input::KI_HOME;
		break;
	case sf::Key::Insert:
		return Rocket::Core::Input::KI_INSERT;
		break;
	case sf::Key::LControl:
		return Rocket::Core::Input::KI_LCONTROL;
		break;
	case sf::Key::LShift:
		return Rocket::Core::Input::KI_LSHIFT;
		break;
	case sf::Key::Multiply:
		return Rocket::Core::Input::KI_MULTIPLY;
		break;
	case sf::Key::Pause:
		return Rocket::Core::Input::KI_PAUSE;
		break;
	case sf::Key::RControl:
		return Rocket::Core::Input::KI_RCONTROL;
		break;
	case sf::Key::Return:
		return Rocket::Core::Input::KI_RETURN;
		break;
	case sf::Key::RShift:
		return Rocket::Core::Input::KI_RSHIFT;
		break;
	case sf::Key::Space:
		return Rocket::Core::Input::KI_SPACE;
		break;
	case sf::Key::Subtract:
		return Rocket::Core::Input::KI_SUBTRACT;
		break;
	case sf::Key::Tab:
		return Rocket::Core::Input::KI_TAB;
		break;
	};

	return Rocket::Core::Input::KI_UNKNOWN;
};

float RocketSFMLSystemInterface::GetElapsedTime()
{
	return timer.GetElapsedTime();
};

bool RocketSFMLSystemInterface::LogMessage(Rocket::Core::Log::Type type, const Rocket::Core::String& message)
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
	};

	printf("%s - %s\n", Type.c_str(), message.CString());

	return true;
};
