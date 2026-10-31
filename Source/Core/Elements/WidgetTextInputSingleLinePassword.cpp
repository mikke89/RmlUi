#include "WidgetTextInputSingleLinePassword.h"
#include "../../../Include/RmlUi/Core/ElementText.h"

namespace Rml {

// The bullet character (U+2022) encoded in UTF-8, displayed in place of each character of the value.
static constexpr char mask_character[] = "\xE2\x80\xA2";
static constexpr int mask_character_size = int(sizeof(mask_character) - 1);

WidgetTextInputSingleLinePassword::WidgetTextInputSingleLinePassword(ElementFormControl* parent) : WidgetTextInputSingleLine(parent) {}

void WidgetTextInputSingleLinePassword::TransformValue(String& value)
{
	const size_t character_length = StringUtilities::LengthUTF8(value);
	value.clear();
	value.reserve(character_length * mask_character_size);
	for (size_t i = 0; i < character_length; i++)
		value += mask_character;
}

int WidgetTextInputSingleLinePassword::DisplayIndexToAttributeIndex(int display_index, const String& attribute_value)
{
	// Each character of the attribute value is displayed as one mask character, thus, here we convert the byte offset
	// into the display value to the character offset, and then count the continuation bytes of the attribute value back
	// in up to that offset.
	const int character_offset = display_index / mask_character_size;
	return StringUtilities::ConvertCharacterOffsetToByteOffset(attribute_value, character_offset);
}

int WidgetTextInputSingleLinePassword::AttributeIndexToDisplayIndex(int attribute_index, const String& attribute_value)
{
	const int character_offset = (int)StringUtilities::LengthUTF8(StringView(attribute_value, 0, (size_t)attribute_index));
	return character_offset * mask_character_size;
}

} // namespace Rml
