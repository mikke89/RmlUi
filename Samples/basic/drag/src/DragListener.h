#ifndef DRAGLISTENER_H
#define DRAGLISTENER_H

#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Types.h>

/**
    @author Pete
 */

class DragListener : public Rml::EventListener {
public:
	/// Registers an elemenet as being a container of draggable elements.
	static void RegisterDraggableContainer(Rml::Element* element);

protected:
	void ProcessEvent(Rml::Event& event) override;
};

#endif
