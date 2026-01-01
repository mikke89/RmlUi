#include "WidgetTextInputSingleLinePassword.h"
#include "../../../Include/RmlUi/Core/ElementText.h"

namespace Rml {

WidgetTextInputSingleLinePassword::WidgetTextInputSingleLinePassword(ElementFormControl* parent) : WidgetTextInputSingleLine(parent) {}

void WidgetTextInputSingleLinePassword::TransformValue(String& value)
{
	const size_t character_length = StringUtilities::LengthUTF8(value);
	value.replace(0, value.length(), character_length, '*');
}

int WidgetTextInputSingleLinePassword::DisplayIndexToAttributeIndex(int display_index, const String& attribute_value)
{
	// Transforming from the attribute value to the display value (above) essentially strips away all continuation
	// bytes. Thus, here we effectively count them back in up to the offset.
	return StringUtilities::ConvertCharacterOffsetToByteOffset(attribute_value, display_index);
}

int WidgetTextInputSingleLinePassword::AttributeIndexToDisplayIndex(int attribute_index, const String& attribute_value)
{
	return (int)StringUtilities::LengthUTF8(StringView(attribute_value, 0, (size_t)attribute_index));
}

} // namespace Rml
