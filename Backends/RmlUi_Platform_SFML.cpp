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

#include "RmlUi_Platform_SFML.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/StringUtilities.h>
#include <RmlUi/Core/SystemInterface.h>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>

SystemInterface_SFML::SystemInterface_SFML()
{
	cursors_valid = true;
	cursors_valid &= cursor_default.loadFromSystem(sf::Cursor::Arrow);
	cursors_valid &= cursor_move.loadFromSystem(sf::Cursor::SizeAll);
	cursors_valid &= cursor_pointer.loadFromSystem(sf::Cursor::Hand);
	cursors_valid &= cursor_resize.loadFromSystem(sf::Cursor::SizeTopLeftBottomRight) || cursor_resize.loadFromSystem(sf::Cursor::SizeAll);
	cursors_valid &= cursor_cross.loadFromSystem(sf::Cursor::Cross);
	cursors_valid &= cursor_text.loadFromSystem(sf::Cursor::Text);
	cursors_valid &= cursor_unavailable.loadFromSystem(sf::Cursor::NotAllowed);
}

void SystemInterface_SFML::SetWindow(sf::RenderWindow* in_window)
{
	window = in_window;
}

double SystemInterface_SFML::GetElapsedTime()
{
	return static_cast<double>(timer.getElapsedTime().asMicroseconds()) / 1'000'000.0;
}

void SystemInterface_SFML::SetMouseCursor(const Rml::String& cursor_name)
{
	if (cursors_valid && window)
	{
		sf::Cursor* cursor = nullptr;
		if (cursor_name.empty() || cursor_name == "arrow")
			cursor = &cursor_default;
		else if (cursor_name == "move")
			cursor = &cursor_move;
		else if (cursor_name == "pointer")
			cursor = &cursor_pointer;
		else if (cursor_name == "resize")
			cursor = &cursor_resize;
		else if (cursor_name == "cross")
			cursor = &cursor_cross;
		else if (cursor_name == "text")
			cursor = &cursor_text;
		else if (cursor_name == "unavailable")
			cursor = &cursor_unavailable;
		else if (Rml::StringUtilities::StartsWith(cursor_name, "rmlui-scroll"))
			cursor = &cursor_move;

		if (cursor)
			window->setMouseCursor(*cursor);
	}
}

void SystemInterface_SFML::SetClipboardText(const Rml::String& text_utf8)
{
	sf::Clipboard::setString(text_utf8);
}

void SystemInterface_SFML::GetClipboardText(Rml::String& text)
{
	text = sf::Clipboard::getString();
}

bool RmlSFML::InputHandler(Rml::Context* context, sf::Event& ev)
{
	bool result = true;

	switch (ev.type)
	{
	case sf::Event::MouseMoved: result = context->ProcessMouseMove(ev.mouseMove.x, ev.mouseMove.y, RmlSFML::GetKeyModifierState()); break;
	case sf::Event::MouseButtonPressed: result = context->ProcessMouseButtonDown(ev.mouseButton.button, RmlSFML::GetKeyModifierState()); break;
	case sf::Event::MouseButtonReleased: result = context->ProcessMouseButtonUp(ev.mouseButton.button, RmlSFML::GetKeyModifierState()); break;
	case sf::Event::MouseWheelMoved: result = context->ProcessMouseWheel(float(-ev.mouseWheel.delta), RmlSFML::GetKeyModifierState()); break;
	case sf::Event::MouseLeft: result = context->ProcessMouseLeave(); break;
	case sf::Event::TextEntered:
	{
		Rml::Character character = Rml::Character(ev.text.unicode);
		if (character == Rml::Character('\r'))
			character = Rml::Character('\n');

		if (ev.text.unicode >= 32 || character == Rml::Character('\n'))
			result = context->ProcessTextInput(character);
	}
	break;
	case sf::Event::KeyPressed: result = context->ProcessKeyDown(RmlSFML::ConvertKey(ev.key.code), RmlSFML::GetKeyModifierState()); break;
	case sf::Event::KeyReleased: result = context->ProcessKeyUp(RmlSFML::ConvertKey(ev.key.code), RmlSFML::GetKeyModifierState()); break;
	default: break;
	}

	return result;
}

Rml::Input::KeyIdentifier RmlSFML::ConvertKey(sf::Keyboard::Key sfml_key)
{
	// clang-format off
	switch (sfml_key)
	{
	case sf::Keyboard::A:         return Rml::Input::KI_A;
	case sf::Keyboard::B:         return Rml::Input::KI_B;
	case sf::Keyboard::C:         return Rml::Input::KI_C;
	case sf::Keyboard::D:         return Rml::Input::KI_D;
	case sf::Keyboard::E:         return Rml::Input::KI_E;
	case sf::Keyboard::F:         return Rml::Input::KI_F;
	case sf::Keyboard::G:         return Rml::Input::KI_G;
	case sf::Keyboard::H:         return Rml::Input::KI_H;
	case sf::Keyboard::I:         return Rml::Input::KI_I;
	case sf::Keyboard::J:         return Rml::Input::KI_J;
	case sf::Keyboard::K:         return Rml::Input::KI_K;
	case sf::Keyboard::L:         return Rml::Input::KI_L;
	case sf::Keyboard::M:         return Rml::Input::KI_M;
	case sf::Keyboard::N:         return Rml::Input::KI_N;
	case sf::Keyboard::O:         return Rml::Input::KI_O;
	case sf::Keyboard::P:         return Rml::Input::KI_P;
	case sf::Keyboard::Q:         return Rml::Input::KI_Q;
	case sf::Keyboard::R:         return Rml::Input::KI_R;
	case sf::Keyboard::S:         return Rml::Input::KI_S;
	case sf::Keyboard::T:         return Rml::Input::KI_T;
	case sf::Keyboard::U:         return Rml::Input::KI_U;
	case sf::Keyboard::V:         return Rml::Input::KI_V;
	case sf::Keyboard::W:         return Rml::Input::KI_W;
	case sf::Keyboard::X:         return Rml::Input::KI_X;
	case sf::Keyboard::Y:         return Rml::Input::KI_Y;
	case sf::Keyboard::Z:         return Rml::Input::KI_Z;
	case sf::Keyboard::Num0:      return Rml::Input::KI_0;
	case sf::Keyboard::Num1:      return Rml::Input::KI_1;
	case sf::Keyboard::Num2:      return Rml::Input::KI_2;
	case sf::Keyboard::Num3:      return Rml::Input::KI_3;
	case sf::Keyboard::Num4:      return Rml::Input::KI_4;
	case sf::Keyboard::Num5:      return Rml::Input::KI_5;
	case sf::Keyboard::Num6:      return Rml::Input::KI_6;
	case sf::Keyboard::Num7:      return Rml::Input::KI_7;
	case sf::Keyboard::Num8:      return Rml::Input::KI_8;
	case sf::Keyboard::Num9:      return Rml::Input::KI_9;
	case sf::Keyboard::Numpad0:   return Rml::Input::KI_NUMPAD0;
	case sf::Keyboard::Numpad1:   return Rml::Input::KI_NUMPAD1;
	case sf::Keyboard::Numpad2:   return Rml::Input::KI_NUMPAD2;
	case sf::Keyboard::Numpad3:   return Rml::Input::KI_NUMPAD3;
	case sf::Keyboard::Numpad4:   return Rml::Input::KI_NUMPAD4;
	case sf::Keyboard::Numpad5:   return Rml::Input::KI_NUMPAD5;
	case sf::Keyboard::Numpad6:   return Rml::Input::KI_NUMPAD6;
	case sf::Keyboard::Numpad7:   return Rml::Input::KI_NUMPAD7;
	case sf::Keyboard::Numpad8:   return Rml::Input::KI_NUMPAD8;
	case sf::Keyboard::Numpad9:   return Rml::Input::KI_NUMPAD9;
	case sf::Keyboard::Left:      return Rml::Input::KI_LEFT;
	case sf::Keyboard::Right:     return Rml::Input::KI_RIGHT;
	case sf::Keyboard::Up:        return Rml::Input::KI_UP;
	case sf::Keyboard::Down:      return Rml::Input::KI_DOWN;
	case sf::Keyboard::Add:       return Rml::Input::KI_ADD;
	case sf::Keyboard::BackSpace: return Rml::Input::KI_BACK;
	case sf::Keyboard::Delete:    return Rml::Input::KI_DELETE;
	case sf::Keyboard::Divide:    return Rml::Input::KI_DIVIDE;
	case sf::Keyboard::End:       return Rml::Input::KI_END;
	case sf::Keyboard::Escape:    return Rml::Input::KI_ESCAPE;
	case sf::Keyboard::F1:        return Rml::Input::KI_F1;
	case sf::Keyboard::F2:        return Rml::Input::KI_F2;
	case sf::Keyboard::F3:        return Rml::Input::KI_F3;
	case sf::Keyboard::F4:        return Rml::Input::KI_F4;
	case sf::Keyboard::F5:        return Rml::Input::KI_F5;
	case sf::Keyboard::F6:        return Rml::Input::KI_F6;
	case sf::Keyboard::F7:        return Rml::Input::KI_F7;
	case sf::Keyboard::F8:        return Rml::Input::KI_F8;
	case sf::Keyboard::F9:        return Rml::Input::KI_F9;
	case sf::Keyboard::F10:       return Rml::Input::KI_F10;
	case sf::Keyboard::F11:       return Rml::Input::KI_F11;
	case sf::Keyboard::F12:       return Rml::Input::KI_F12;
	case sf::Keyboard::F13:       return Rml::Input::KI_F13;
	case sf::Keyboard::F14:       return Rml::Input::KI_F14;
	case sf::Keyboard::F15:       return Rml::Input::KI_F15;
	case sf::Keyboard::Home:      return Rml::Input::KI_HOME;
	case sf::Keyboard::Insert:    return Rml::Input::KI_INSERT;
	case sf::Keyboard::LControl:  return Rml::Input::KI_LCONTROL;
	case sf::Keyboard::LShift:    return Rml::Input::KI_LSHIFT;
	case sf::Keyboard::Multiply:  return Rml::Input::KI_MULTIPLY;
	case sf::Keyboard::Pause:     return Rml::Input::KI_PAUSE;
	case sf::Keyboard::RControl:  return Rml::Input::KI_RCONTROL;
	case sf::Keyboard::Return:    return Rml::Input::KI_RETURN;
	case sf::Keyboard::RShift:    return Rml::Input::KI_RSHIFT;
	case sf::Keyboard::Space:     return Rml::Input::KI_SPACE;
	case sf::Keyboard::Subtract:  return Rml::Input::KI_SUBTRACT;
	case sf::Keyboard::Tab:       return Rml::Input::KI_TAB;
	default: break;
	}
	// clang-format on

	return Rml::Input::KI_UNKNOWN;
}

int RmlSFML::GetKeyModifierState()
{
	int modifiers = 0;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift))
		modifiers |= Rml::Input::KM_SHIFT;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::RControl))
		modifiers |= Rml::Input::KM_CTRL;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) || sf::Keyboard::isKeyPressed(sf::Keyboard::RAlt))
		modifiers |= Rml::Input::KM_ALT;

	return modifiers;
}
