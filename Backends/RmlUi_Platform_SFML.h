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

#ifndef RMLUI_BACKENDS_PLATFORM_SFML_H
#define RMLUI_BACKENDS_PLATFORM_SFML_H

#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/SystemInterface.h>
#include <RmlUi/Core/Types.h>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>

class SystemInterface_SFML : public Rml::SystemInterface {
public:
	SystemInterface_SFML();

	// Optionally, provide or change the window to be used for setting the mouse cursors.
	// @lifetime Any window provided here must be destroyed before the system interface.
	// @lifetime The currently active window must stay alive until after the call to Rml::Shutdown.
	void SetWindow(sf::RenderWindow* window);

	// -- Inherited from Rml::SystemInterface  --

	double GetElapsedTime() override;

	void SetMouseCursor(const Rml::String& cursor_name) override;

	void SetClipboardText(const Rml::String& text) override;
	void GetClipboardText(Rml::String& text) override;

private:
	sf::Clock timer;
	sf::RenderWindow* window = nullptr;

	bool cursors_valid = false;
	sf::Cursor cursor_default;
	sf::Cursor cursor_move;
	sf::Cursor cursor_pointer;
	sf::Cursor cursor_resize;
	sf::Cursor cursor_cross;
	sf::Cursor cursor_text;
	sf::Cursor cursor_unavailable;
};

/**
    Optional helper functions for the SFML plaform.
 */
namespace RmlSFML {

// Applies input on the context based on the given SFML event.
// @return True if the event is still propagating, false if it was handled by the context.
bool InputHandler(Rml::Context* context, sf::Event& ev);

// Converts the SFML key to RmlUi key.
Rml::Input::KeyIdentifier ConvertKey(sf::Keyboard::Key sfml_key);

// Returns the active RmlUi key modifier state.
int GetKeyModifierState();

} // namespace RmlSFML

#endif
