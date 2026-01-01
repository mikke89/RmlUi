#pragma once

#include "WidgetTextInput.h"

namespace Rml {

/**
    A specialisation of the text input widget for multi-line text fields.
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
