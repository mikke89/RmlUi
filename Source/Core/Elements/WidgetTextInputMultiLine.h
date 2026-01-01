#ifndef RMLUI_CORE_ELEMENTS_WIDGETTEXTINPUTMULTILINE_H
#define RMLUI_CORE_ELEMENTS_WIDGETTEXTINPUTMULTILINE_H

#include "WidgetTextInput.h"

namespace Rml {

/**
    A specialisation of the text input widget for multi-line text fields.

    @author Peter Curry
 */

class WidgetTextInputMultiLine : public WidgetTextInput {
public:
	WidgetTextInputMultiLine(ElementFormControl* parent);
	virtual ~WidgetTextInputMultiLine();

protected:
	/// Removes any invalid characters from the string.
	void SanitizeValue(String& value) override;
	/// Called when the user pressed enter.
	void LineBreak() override;
};

} // namespace Rml
#endif
