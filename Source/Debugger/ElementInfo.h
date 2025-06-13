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

#ifndef RMLUI_DEBUGGER_ELEMENTINFO_H
#define RMLUI_DEBUGGER_ELEMENTINFO_H

#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/EventListener.h"
#include "ElementDebugDocument.h"

namespace Rml {
namespace Debugger {

typedef Pair<String, const Property*> NamedProperty;
typedef Vector<NamedProperty> NamedPropertyList;

/**
    @author Robert Curry
 */

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

#endif
