#pragma once

#include "WidgetTextInputSingleLine.h"

namespace Rml {

class WidgetTextInputSingleLinePassword : public WidgetTextInputSingleLine {
public:
	WidgetTextInputSingleLinePassword(ElementFormControl* parent);

protected:
	void TransformValue(String& value) override;

	int DisplayIndexToAttributeIndex(int display_index, const String& attribute_value) override;

	int AttributeIndexToDisplayIndex(int attribute_index, const String& attribute_value) override;
};

} // namespace Rml
