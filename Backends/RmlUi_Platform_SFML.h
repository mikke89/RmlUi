#pragma once

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

	struct Cursors {
		Cursors();
		sf::Cursor cursor_default;
		sf::Cursor cursor_move;
		sf::Cursor cursor_pointer;
		sf::Cursor cursor_resize;
		sf::Cursor cursor_cross;
		sf::Cursor cursor_text;
		sf::Cursor cursor_unavailable;
	};
	Rml::UniquePtr<Cursors> cursors;
};

/**
    Optional helper functions for the SFML platform.
 */
namespace RmlSFML {

// Applies input on the context based on the given SFML event.
// @return True if the event is still propagating, false if it was handled by the context.
bool InputHandler(Rml::Context* context, const sf::Event& ev);

// Converts the SFML key to RmlUi key.
Rml::Input::KeyIdentifier ConvertKey(sf::Keyboard::Key sfml_key);

// Returns the active RmlUi key modifier state.
int GetKeyModifierState();

} // namespace RmlSFML
