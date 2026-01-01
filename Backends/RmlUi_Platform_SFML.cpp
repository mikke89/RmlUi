#include "RmlUi_Platform_SFML.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/StringUtilities.h>
#include <RmlUi/Core/SystemInterface.h>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>

#if SFML_VERSION_MAJOR >= 3
SystemInterface_SFML::Cursors::Cursors() :
	cursor_default(sf::Cursor::Type::Arrow), cursor_move(sf::Cursor::Type::SizeAll), cursor_pointer(sf::Cursor::Type::Hand),
	cursor_resize(sf::Cursor::createFromSystem(sf::Cursor::Type::SizeTopLeftBottomRight).value_or(sf::Cursor(sf::Cursor::Type::SizeAll))),
	cursor_cross(sf::Cursor::Type::Cross), cursor_text(sf::Cursor::Type::Text), cursor_unavailable(sf::Cursor::Type::NotAllowed)
{}
SystemInterface_SFML::SystemInterface_SFML()
{
	try
	{
		cursors = std::make_unique<Cursors>();
	} catch (const sf::Exception& /*exception*/)
	{
		cursors.reset();
	}
}

#else

SystemInterface_SFML::Cursors::Cursors() {}
SystemInterface_SFML::SystemInterface_SFML()
{
	cursors = std::make_unique<Cursors>();
	bool cursors_valid = true;
	cursors_valid &= cursors->cursor_default.loadFromSystem(sf::Cursor::Arrow);
	cursors_valid &= cursors->cursor_move.loadFromSystem(sf::Cursor::SizeAll);
	cursors_valid &= cursors->cursor_pointer.loadFromSystem(sf::Cursor::Hand);
	cursors_valid &=
		cursors->cursor_resize.loadFromSystem(sf::Cursor::SizeTopLeftBottomRight) || cursors->cursor_resize.loadFromSystem(sf::Cursor::SizeAll);
	cursors_valid &= cursors->cursor_cross.loadFromSystem(sf::Cursor::Cross);
	cursors_valid &= cursors->cursor_text.loadFromSystem(sf::Cursor::Text);
	cursors_valid &= cursors->cursor_unavailable.loadFromSystem(sf::Cursor::NotAllowed);
	if (!cursors_valid)
		cursors.reset();
}
#endif

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
	if (cursors && window)
	{
		sf::Cursor* cursor = nullptr;
		if (cursor_name.empty() || cursor_name == "arrow")
			cursor = &cursors->cursor_default;
		else if (cursor_name == "move")
			cursor = &cursors->cursor_move;
		else if (cursor_name == "pointer")
			cursor = &cursors->cursor_pointer;
		else if (cursor_name == "resize")
			cursor = &cursors->cursor_resize;
		else if (cursor_name == "cross")
			cursor = &cursors->cursor_cross;
		else if (cursor_name == "text")
			cursor = &cursors->cursor_text;
		else if (cursor_name == "unavailable")
			cursor = &cursors->cursor_unavailable;
		else if (Rml::StringUtilities::StartsWith(cursor_name, "rmlui-scroll"))
			cursor = &cursors->cursor_move;

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

bool RmlSFML::InputHandler(Rml::Context* context, const sf::Event& ev)
{
	bool result = true;

#if SFML_VERSION_MAJOR >= 3

	if (auto mouse_moved = ev.getIf<sf::Event::MouseMoved>())
	{
		result = context->ProcessMouseMove(mouse_moved->position.x, mouse_moved->position.y, RmlSFML::GetKeyModifierState());
	}
	else if (auto mouse_pressed = ev.getIf<sf::Event::MouseButtonPressed>())
	{
		result = context->ProcessMouseButtonDown(int(mouse_pressed->button), RmlSFML::GetKeyModifierState());
	}
	else if (auto mouse_released = ev.getIf<sf::Event::MouseButtonReleased>())
	{
		result = context->ProcessMouseButtonUp(int(mouse_released->button), RmlSFML::GetKeyModifierState());
	}
	else if (auto wheel_scrolled = ev.getIf<sf::Event::MouseWheelScrolled>())
	{
		const Rml::Vector2f delta = {
			wheel_scrolled->wheel == sf::Mouse::Wheel::Horizontal ? -wheel_scrolled->delta : 0.f,
			wheel_scrolled->wheel == sf::Mouse::Wheel::Vertical ? -wheel_scrolled->delta : 0.f,
		};
		result = context->ProcessMouseWheel(delta, RmlSFML::GetKeyModifierState());
	}
	else if (ev.is<sf::Event::MouseLeft>())
	{
		result = context->ProcessMouseLeave();
	}
	else if (auto text_entered = ev.getIf<sf::Event::TextEntered>())
	{
		Rml::Character character = Rml::Character(text_entered->unicode);
		if (character == Rml::Character('\r'))
			character = Rml::Character('\n');

		if (text_entered->unicode >= 32 || character == Rml::Character('\n'))
			result = context->ProcessTextInput(character);
	}
	else if (auto key_pressed = ev.getIf<sf::Event::KeyPressed>())
	{
		result = context->ProcessKeyDown(RmlSFML::ConvertKey(key_pressed->code), RmlSFML::GetKeyModifierState());
	}
	else if (auto key_released = ev.getIf<sf::Event::KeyReleased>())
	{
		result = context->ProcessKeyUp(RmlSFML::ConvertKey(key_released->code), RmlSFML::GetKeyModifierState());
	}

#else

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
#endif

	return result;
}

Rml::Input::KeyIdentifier RmlSFML::ConvertKey(sf::Keyboard::Key sfml_key)
{
#if SFML_VERSION_MAJOR >= 3
	constexpr auto sfBackspace = sf::Keyboard::Key::Backspace;
	constexpr auto sfEnter = sf::Keyboard::Key::Enter;
#else
	constexpr auto sfBackspace = sf::Keyboard::Key::BackSpace;
	constexpr auto sfEnter = sf::Keyboard::Key::Return;
#endif

	// clang-format off
	switch (sfml_key)
	{
	case sf::Keyboard::Key::A:         return Rml::Input::KI_A;
	case sf::Keyboard::Key::B:         return Rml::Input::KI_B;
	case sf::Keyboard::Key::C:         return Rml::Input::KI_C;
	case sf::Keyboard::Key::D:         return Rml::Input::KI_D;
	case sf::Keyboard::Key::E:         return Rml::Input::KI_E;
	case sf::Keyboard::Key::F:         return Rml::Input::KI_F;
	case sf::Keyboard::Key::G:         return Rml::Input::KI_G;
	case sf::Keyboard::Key::H:         return Rml::Input::KI_H;
	case sf::Keyboard::Key::I:         return Rml::Input::KI_I;
	case sf::Keyboard::Key::J:         return Rml::Input::KI_J;
	case sf::Keyboard::Key::K:         return Rml::Input::KI_K;
	case sf::Keyboard::Key::L:         return Rml::Input::KI_L;
	case sf::Keyboard::Key::M:         return Rml::Input::KI_M;
	case sf::Keyboard::Key::N:         return Rml::Input::KI_N;
	case sf::Keyboard::Key::O:         return Rml::Input::KI_O;
	case sf::Keyboard::Key::P:         return Rml::Input::KI_P;
	case sf::Keyboard::Key::Q:         return Rml::Input::KI_Q;
	case sf::Keyboard::Key::R:         return Rml::Input::KI_R;
	case sf::Keyboard::Key::S:         return Rml::Input::KI_S;
	case sf::Keyboard::Key::T:         return Rml::Input::KI_T;
	case sf::Keyboard::Key::U:         return Rml::Input::KI_U;
	case sf::Keyboard::Key::V:         return Rml::Input::KI_V;
	case sf::Keyboard::Key::W:         return Rml::Input::KI_W;
	case sf::Keyboard::Key::X:         return Rml::Input::KI_X;
	case sf::Keyboard::Key::Y:         return Rml::Input::KI_Y;
	case sf::Keyboard::Key::Z:         return Rml::Input::KI_Z;
	case sf::Keyboard::Key::Num0:      return Rml::Input::KI_0;
	case sf::Keyboard::Key::Num1:      return Rml::Input::KI_1;
	case sf::Keyboard::Key::Num2:      return Rml::Input::KI_2;
	case sf::Keyboard::Key::Num3:      return Rml::Input::KI_3;
	case sf::Keyboard::Key::Num4:      return Rml::Input::KI_4;
	case sf::Keyboard::Key::Num5:      return Rml::Input::KI_5;
	case sf::Keyboard::Key::Num6:      return Rml::Input::KI_6;
	case sf::Keyboard::Key::Num7:      return Rml::Input::KI_7;
	case sf::Keyboard::Key::Num8:      return Rml::Input::KI_8;
	case sf::Keyboard::Key::Num9:      return Rml::Input::KI_9;
	case sf::Keyboard::Key::Numpad0:   return Rml::Input::KI_NUMPAD0;
	case sf::Keyboard::Key::Numpad1:   return Rml::Input::KI_NUMPAD1;
	case sf::Keyboard::Key::Numpad2:   return Rml::Input::KI_NUMPAD2;
	case sf::Keyboard::Key::Numpad3:   return Rml::Input::KI_NUMPAD3;
	case sf::Keyboard::Key::Numpad4:   return Rml::Input::KI_NUMPAD4;
	case sf::Keyboard::Key::Numpad5:   return Rml::Input::KI_NUMPAD5;
	case sf::Keyboard::Key::Numpad6:   return Rml::Input::KI_NUMPAD6;
	case sf::Keyboard::Key::Numpad7:   return Rml::Input::KI_NUMPAD7;
	case sf::Keyboard::Key::Numpad8:   return Rml::Input::KI_NUMPAD8;
	case sf::Keyboard::Key::Numpad9:   return Rml::Input::KI_NUMPAD9;
	case sf::Keyboard::Key::Left:      return Rml::Input::KI_LEFT;
	case sf::Keyboard::Key::Right:     return Rml::Input::KI_RIGHT;
	case sf::Keyboard::Key::Up:        return Rml::Input::KI_UP;
	case sf::Keyboard::Key::Down:      return Rml::Input::KI_DOWN;
	case sf::Keyboard::Key::Add:       return Rml::Input::KI_ADD;
	case sfBackspace:                  return Rml::Input::KI_BACK;
	case sf::Keyboard::Key::Delete:    return Rml::Input::KI_DELETE;
	case sf::Keyboard::Key::Divide:    return Rml::Input::KI_DIVIDE;
	case sf::Keyboard::Key::End:       return Rml::Input::KI_END;
	case sf::Keyboard::Key::Escape:    return Rml::Input::KI_ESCAPE;
	case sf::Keyboard::Key::F1:        return Rml::Input::KI_F1;
	case sf::Keyboard::Key::F2:        return Rml::Input::KI_F2;
	case sf::Keyboard::Key::F3:        return Rml::Input::KI_F3;
	case sf::Keyboard::Key::F4:        return Rml::Input::KI_F4;
	case sf::Keyboard::Key::F5:        return Rml::Input::KI_F5;
	case sf::Keyboard::Key::F6:        return Rml::Input::KI_F6;
	case sf::Keyboard::Key::F7:        return Rml::Input::KI_F7;
	case sf::Keyboard::Key::F8:        return Rml::Input::KI_F8;
	case sf::Keyboard::Key::F9:        return Rml::Input::KI_F9;
	case sf::Keyboard::Key::F10:       return Rml::Input::KI_F10;
	case sf::Keyboard::Key::F11:       return Rml::Input::KI_F11;
	case sf::Keyboard::Key::F12:       return Rml::Input::KI_F12;
	case sf::Keyboard::Key::F13:       return Rml::Input::KI_F13;
	case sf::Keyboard::Key::F14:       return Rml::Input::KI_F14;
	case sf::Keyboard::Key::F15:       return Rml::Input::KI_F15;
	case sf::Keyboard::Key::Home:      return Rml::Input::KI_HOME;
	case sf::Keyboard::Key::Insert:    return Rml::Input::KI_INSERT;
	case sf::Keyboard::Key::LControl:  return Rml::Input::KI_LCONTROL;
	case sf::Keyboard::Key::LShift:    return Rml::Input::KI_LSHIFT;
	case sf::Keyboard::Key::Multiply:  return Rml::Input::KI_MULTIPLY;
	case sf::Keyboard::Key::Pause:     return Rml::Input::KI_PAUSE;
	case sf::Keyboard::Key::RControl:  return Rml::Input::KI_RCONTROL;
	case sfEnter:                      return Rml::Input::KI_RETURN;
	case sf::Keyboard::Key::RShift:    return Rml::Input::KI_RSHIFT;
	case sf::Keyboard::Key::Space:     return Rml::Input::KI_SPACE;
	case sf::Keyboard::Key::Subtract:  return Rml::Input::KI_SUBTRACT;
	case sf::Keyboard::Key::Tab:       return Rml::Input::KI_TAB;
	default: break;
	}
	// clang-format on

	return Rml::Input::KI_UNKNOWN;
}

int RmlSFML::GetKeyModifierState()
{
	int modifiers = 0;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift))
		modifiers |= Rml::Input::KM_SHIFT;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl))
		modifiers |= Rml::Input::KM_CTRL;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RAlt))
		modifiers |= Rml::Input::KM_ALT;

	return modifiers;
}
