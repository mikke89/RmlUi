#ifndef RMLUI_LUAINVADERS_ELEMENTGAME_H
#define RMLUI_LUAINVADERS_ELEMENTGAME_H

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/EventListener.h>

class Game;

/**
    @author Peter Curry
 */

class ElementGame : public Rml::Element, public Rml::EventListener {
public:
	ElementGame(const Rml::String& tag);
	virtual ~ElementGame();

	/// Intercepts and handles key events.
	void ProcessEvent(Rml::Event& event) override;

	/// This will get called when we're added to the tree, which allows us to bind to events.
	void OnChildAdd(Rml::Element* element) override;

	/// This will get called when we're removed from the tree, which allows us to clean up the event listeners previously added.
	void OnChildRemove(Element* element) override;

protected:
	/// Updates the game.
	void OnUpdate() override;
	/// Renders the game.
	void OnRender() override;

private:
	Game* game;
};

#endif
