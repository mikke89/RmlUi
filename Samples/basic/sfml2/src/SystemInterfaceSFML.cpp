/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 Nuno Silva
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
#include <RmlUi/Core.h>
#include "SystemInterfaceSFML.h"

int RmlUiSFMLSystemInterface::GetKeyModifiers(sf::Window *Window)
{
	int Modifiers = 0;

	if(sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
		sf::Keyboard::isKeyPressed(sf::Keyboard::RShift))
		Modifiers |= Rml::Core::Input::KM_SHIFT;

	if(sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ||
		sf::Keyboard::isKeyPressed(sf::Keyboard::RControl))
		Modifiers |= Rml::Core::Input::KM_CTRL;

	if(sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) ||
		sf::Keyboard::isKeyPressed(sf::Keyboard::RAlt))
		Modifiers |= Rml::Core::Input::KM_ALT;

	return Modifiers;
};

Rml::Core::Input::KeyIdentifier RmlUiSFMLSystemInterface::TranslateKey(sf::Keyboard::Key Key)
{
	switch(Key)
	{
	case sf::Keyboard::A:
		return Rml::Core::Input::KI_A;
		break;
	case sf::Keyboard::B:
		return Rml::Core::Input::KI_B;
		break;
	case sf::Keyboard::C:
		return Rml::Core::Input::KI_C;
		break;
	case sf::Keyboard::D:
		return Rml::Core::Input::KI_D;
		break;
	case sf::Keyboard::E:
		return Rml::Core::Input::KI_E;
		break;
	case sf::Keyboard::F:
		return Rml::Core::Input::KI_F;
		break;
	case sf::Keyboard::G:
		return Rml::Core::Input::KI_G;
		break;
	case sf::Keyboard::H:
		return Rml::Core::Input::KI_H;
		break;
	case sf::Keyboard::I:
		return Rml::Core::Input::KI_I;
		break;
	case sf::Keyboard::J:
		return Rml::Core::Input::KI_J;
		break;
	case sf::Keyboard::K:
		return Rml::Core::Input::KI_K;
		break;
	case sf::Keyboard::L:
		return Rml::Core::Input::KI_L;
		break;
	case sf::Keyboard::M:
		return Rml::Core::Input::KI_M;
		break;
	case sf::Keyboard::N:
		return Rml::Core::Input::KI_N;
		break;
	case sf::Keyboard::O:
		return Rml::Core::Input::KI_O;
		break;
	case sf::Keyboard::P:
		return Rml::Core::Input::KI_P;
		break;
	case sf::Keyboard::Q:
		return Rml::Core::Input::KI_Q;
		break;
	case sf::Keyboard::R:
		return Rml::Core::Input::KI_R;
		break;
	case sf::Keyboard::S:
		return Rml::Core::Input::KI_S;
		break;
	case sf::Keyboard::T:
		return Rml::Core::Input::KI_T;
		break;
	case sf::Keyboard::U:
		return Rml::Core::Input::KI_U;
		break;
	case sf::Keyboard::V:
		return Rml::Core::Input::KI_V;
		break;
	case sf::Keyboard::W:
		return Rml::Core::Input::KI_W;
		break;
	case sf::Keyboard::X:
		return Rml::Core::Input::KI_X;
		break;
	case sf::Keyboard::Y:
		return Rml::Core::Input::KI_Y;
		break;
	case sf::Keyboard::Z:
		return Rml::Core::Input::KI_Z;
		break;
	case sf::Keyboard::Num0:
		return Rml::Core::Input::KI_0;
		break;
	case sf::Keyboard::Num1:
		return Rml::Core::Input::KI_1;
		break;
	case sf::Keyboard::Num2:
		return Rml::Core::Input::KI_2;
		break;
	case sf::Keyboard::Num3:
		return Rml::Core::Input::KI_3;
		break;
	case sf::Keyboard::Num4:
		return Rml::Core::Input::KI_4;
		break;
	case sf::Keyboard::Num5:
		return Rml::Core::Input::KI_5;
		break;
	case sf::Keyboard::Num6:
		return Rml::Core::Input::KI_6;
		break;
	case sf::Keyboard::Num7:
		return Rml::Core::Input::KI_7;
		break;
	case sf::Keyboard::Num8:
		return Rml::Core::Input::KI_8;
		break;
	case sf::Keyboard::Num9:
		return Rml::Core::Input::KI_9;
		break;
	case sf::Keyboard::Numpad0:
		return Rml::Core::Input::KI_NUMPAD0;
		break;
	case sf::Keyboard::Numpad1:
		return Rml::Core::Input::KI_NUMPAD1;
		break;
	case sf::Keyboard::Numpad2:
		return Rml::Core::Input::KI_NUMPAD2;
		break;
	case sf::Keyboard::Numpad3:
		return Rml::Core::Input::KI_NUMPAD3;
		break;
	case sf::Keyboard::Numpad4:
		return Rml::Core::Input::KI_NUMPAD4;
		break;
	case sf::Keyboard::Numpad5:
		return Rml::Core::Input::KI_NUMPAD5;
		break;
	case sf::Keyboard::Numpad6:
		return Rml::Core::Input::KI_NUMPAD6;
		break;
	case sf::Keyboard::Numpad7:
		return Rml::Core::Input::KI_NUMPAD7;
		break;
	case sf::Keyboard::Numpad8:
		return Rml::Core::Input::KI_NUMPAD8;
		break;
	case sf::Keyboard::Numpad9:
		return Rml::Core::Input::KI_NUMPAD9;
		break;
	case sf::Keyboard::Left:
		return Rml::Core::Input::KI_LEFT;
		break;
	case sf::Keyboard::Right:
		return Rml::Core::Input::KI_RIGHT;
		break;
	case sf::Keyboard::Up:
		return Rml::Core::Input::KI_UP;
		break;
	case sf::Keyboard::Down:
		return Rml::Core::Input::KI_DOWN;
		break;
	case sf::Keyboard::Add:
		return Rml::Core::Input::KI_ADD;
		break;
	case sf::Keyboard::BackSpace:
		return Rml::Core::Input::KI_BACK;
		break;
	case sf::Keyboard::Delete:
		return Rml::Core::Input::KI_DELETE;
		break;
	case sf::Keyboard::Divide:
		return Rml::Core::Input::KI_DIVIDE;
		break;
	case sf::Keyboard::End:
		return Rml::Core::Input::KI_END;
		break;
	case sf::Keyboard::Escape:
		return Rml::Core::Input::KI_ESCAPE;
		break;
	case sf::Keyboard::F1:
		return Rml::Core::Input::KI_F1;
		break;
	case sf::Keyboard::F2:
		return Rml::Core::Input::KI_F2;
		break;
	case sf::Keyboard::F3:
		return Rml::Core::Input::KI_F3;
		break;
	case sf::Keyboard::F4:
		return Rml::Core::Input::KI_F4;
		break;
	case sf::Keyboard::F5:
		return Rml::Core::Input::KI_F5;
		break;
	case sf::Keyboard::F6:
		return Rml::Core::Input::KI_F6;
		break;
	case sf::Keyboard::F7:
		return Rml::Core::Input::KI_F7;
		break;
	case sf::Keyboard::F8:
		return Rml::Core::Input::KI_F8;
		break;
	case sf::Keyboard::F9:
		return Rml::Core::Input::KI_F9;
		break;
	case sf::Keyboard::F10:
		return Rml::Core::Input::KI_F10;
		break;
	case sf::Keyboard::F11:
		return Rml::Core::Input::KI_F11;
		break;
	case sf::Keyboard::F12:
		return Rml::Core::Input::KI_F12;
		break;
	case sf::Keyboard::F13:
		return Rml::Core::Input::KI_F13;
		break;
	case sf::Keyboard::F14:
		return Rml::Core::Input::KI_F14;
		break;
	case sf::Keyboard::F15:
		return Rml::Core::Input::KI_F15;
		break;
	case sf::Keyboard::Home:
		return Rml::Core::Input::KI_HOME;
		break;
	case sf::Keyboard::Insert:
		return Rml::Core::Input::KI_INSERT;
		break;
	case sf::Keyboard::LControl:
		return Rml::Core::Input::KI_LCONTROL;
		break;
	case sf::Keyboard::LShift:
		return Rml::Core::Input::KI_LSHIFT;
		break;
	case sf::Keyboard::Multiply:
		return Rml::Core::Input::KI_MULTIPLY;
		break;
	case sf::Keyboard::Pause:
		return Rml::Core::Input::KI_PAUSE;
		break;
	case sf::Keyboard::RControl:
		return Rml::Core::Input::KI_RCONTROL;
		break;
	case sf::Keyboard::Return:
		return Rml::Core::Input::KI_RETURN;
		break;
	case sf::Keyboard::RShift:
		return Rml::Core::Input::KI_RSHIFT;
		break;
	case sf::Keyboard::Space:
		return Rml::Core::Input::KI_SPACE;
		break;
	case sf::Keyboard::Subtract:
		return Rml::Core::Input::KI_SUBTRACT;
		break;
	case sf::Keyboard::Tab:
		return Rml::Core::Input::KI_TAB;
		break;
	default:
		break;
	};

	return Rml::Core::Input::KI_UNKNOWN;
};

double RmlUiSFMLSystemInterface::GetElapsedTime()
{
	return timer.getElapsedTime().asSeconds();
};

bool RmlUiSFMLSystemInterface::LogMessage(Rml::Core::Log::Type type, const Rml::Core::String& message)
{
	std::string Type;

	switch(type)
	{
	case Rml::Core::Log::LT_ALWAYS:
		Type = "[Always]";
		break;
	case Rml::Core::Log::LT_ERROR:
		Type = "[Error]";
		break;
	case Rml::Core::Log::LT_ASSERT:
		Type = "[Assert]";
		break;
	case Rml::Core::Log::LT_WARNING:
		Type = "[Warning]";
		break;
	case Rml::Core::Log::LT_INFO:
		Type = "[Info]";
		break;
	case Rml::Core::Log::LT_DEBUG:
		Type = "[Debug]";
		break;
	default:
		break;
	};

	printf("%s - %s\n", Type.c_str(), message.c_str());

	return true;
};
