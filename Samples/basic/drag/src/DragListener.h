#pragma once

#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Types.h>

class DragListener : public Rml::EventListener {
public:
	/// Registers an elemenet as being a container of draggable elements.
	static void RegisterDraggableContainer(Rml::Element* element);

protected:
	void ProcessEvent(Rml::Event& event) override;
};
