#ifndef DRAGLISTENER_H
#define DRAGLISTENER_H

#include <Rocket/Core/EventListener.h>
#include <Rocket/Core/Types.h>

/**
	@author Pete
 */

class DragListener : public Rocket::Core::EventListener
{
public:
	/// Registers an elemenet as being a container of draggable elements.
	static void RegisterDraggableContainer(Rocket::Core::Element* element);

protected:
	virtual void ProcessEvent(Rocket::Core::Event& event);
};

#endif
