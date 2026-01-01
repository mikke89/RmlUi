#pragma once

#include "../../../Include/RmlUi/Core/Element.h"

namespace Rml {

class WidgetTextInput;

/**
    A stub element used by the WidgetTextInput to query the RCSS-specified text colour and
    background colour for selected text.
 */

class ElementTextSelection : public Element {
public:
	RMLUI_RTTI_DefineWithParent(ElementTextSelection, Element)

	ElementTextSelection(const String& tag);
	virtual ~ElementTextSelection();

	/// Set the widget that this selection element was created for. This is the widget that will be
	/// notified when this element's properties are altered.
	void SetWidget(WidgetTextInput* widget);

protected:
	/// Processes 'color' and 'background-color' property changes.
	void OnPropertyChange(const PropertyIdSet& changed_properties) override;

private:
	WidgetTextInput* widget;
};

} // namespace Rml
