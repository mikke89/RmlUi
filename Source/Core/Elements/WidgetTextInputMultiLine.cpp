#include "WidgetTextInputMultiLine.h"
#include "../../../Include/RmlUi/Core/Dictionary.h"
#include "../../../Include/RmlUi/Core/ElementText.h"
#include <algorithm>

namespace Rml {

WidgetTextInputMultiLine::WidgetTextInputMultiLine(ElementFormControl* parent) : WidgetTextInput(parent) {}

WidgetTextInputMultiLine::~WidgetTextInputMultiLine() {}

void WidgetTextInputMultiLine::SanitizeValue(String& value)
{
	value.erase(std::remove_if(value.begin(), value.end(), [](char c) { return c == '\r' || c == '\t'; }), value.end());
}

void WidgetTextInputMultiLine::LineBreak() {}

} // namespace Rml
