#pragma once

#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/EventListener.h"
#include "ElementDebugDocument.h"

namespace Rml {
namespace Debugger {

typedef Pair<String, const Property*> NamedProperty;
typedef Vector<NamedProperty> NamedPropertyList;

class ElementInfo : public ElementDebugDocument, public EventListener {
public:
	RMLUI_RTTI_DefineWithParent(ElementInfo, ElementDebugDocument)

	ElementInfo(const String& tag);
	~ElementInfo();

	/// Initialises the info element.
	/// @return True if the element initialised successfully, false otherwise.
	bool Initialise();
	/// Clears the element references.
	void Reset();

	/// Called when an element is destroyed.
	void OnElementDestroy(Element* element);

	void RenderHoverElement();
	void RenderSourceElement();

	Element* GetSourceElement() const { return source_element; }

protected:
	void ProcessEvent(Event& event) override;
	/// Updates the element info if changed
	void OnUpdate() override;

private:
	void SetSourceElement(Element* new_source_element);
	void UpdateSourceElement();

	void BuildElementPropertiesRML(String& property_rml, Element* element, Element* primary_element);
	void BuildPropertyRML(String& property_rml, const String& name, const Property* property);

	void UpdateTitle();

	bool IsDebuggerElement(Element* element);

	double previous_update_time;

	String attributes_rml, properties_rml, events_rml, position_rml, ancestors_rml, children_rml;

	// Enables or disables the selection of elements in user context.
	bool enable_element_select;
	// Draws the dimensions of the source element.
	bool show_source_element;
	// Updates the source element information at regular intervals.
	bool update_source_element;
	// Forces an update to the source element during the next update loop.
	bool force_update_once;

	bool title_dirty;

	Element* hover_element;
	Element* source_element;
};

} // namespace Debugger
} // namespace Rml
