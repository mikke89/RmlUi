#ifndef RMLUI_CORE_ELEMENTS_ELEMENTLABEL_H
#define RMLUI_CORE_ELEMENTS_ELEMENTLABEL_H

#include "../../../Include/RmlUi/Core/Element.h"
#include "../../../Include/RmlUi/Core/EventListener.h"
#include "../../../Include/RmlUi/Core/Header.h"

namespace Rml {

/**
    A specialisation of the generic Core::Element representing a label element.

 */

class ElementLabel : public Element, public EventListener {
public:
	RMLUI_RTTI_DefineWithParent(ElementLabel, Element)

	ElementLabel(const String& tag);
	virtual ~ElementLabel();

protected:
	void OnPseudoClassChange(const String& pseudo_class, bool activate) override;

	void ProcessEvent(Event& event) override;

private:
	Element* GetTarget();

	bool disable_click = false;
};

} // namespace Rml

#endif
