#include "../../../Include/RmlUi/Core/Elements/ElementFormControlTextArea.h"
#include "../../../Include/RmlUi/Core/ElementText.h"
#include "../../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../../Include/RmlUi/Core/Math.h"
#include "../../../Include/RmlUi/Core/PropertyIdSet.h"
#include "../../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "WidgetTextInputMultiLine.h"

namespace Rml {

ElementFormControlTextArea::ElementFormControlTextArea(const String& tag) : ElementFormControl(tag)
{
	widget = MakeUnique<WidgetTextInputMultiLine>(this);
	SetWordWrapProperties();
}

ElementFormControlTextArea::~ElementFormControlTextArea() {}

String ElementFormControlTextArea::GetValue() const
{
	return GetAttribute<String>("value", "");
}

void ElementFormControlTextArea::SetValue(const String& value)
{
	SetAttribute("value", value);
}

void ElementFormControlTextArea::SetNumColumns(int num_columns)
{
	SetAttribute<int>("cols", Math::Max(1, num_columns));
}

int ElementFormControlTextArea::GetNumColumns() const
{
	return GetAttribute<int>("cols", 20);
}

void ElementFormControlTextArea::SetNumRows(int num_rows)
{
	SetAttribute<int>("rows", Math::Max(1, num_rows));
}

int ElementFormControlTextArea::GetNumRows() const
{
	return GetAttribute<int>("rows", 2);
}

void ElementFormControlTextArea::SetMaxLength(int max_length)
{
	SetAttribute<int>("maxlength", max_length);
}

int ElementFormControlTextArea::GetMaxLength() const
{
	return GetAttribute<int>("maxlength", -1);
}

void ElementFormControlTextArea::SetWordWrap(bool word_wrap)
{
	if (word_wrap != GetWordWrap())
	{
		if (word_wrap)
			RemoveAttribute("wrap");
		else
			SetAttribute("wrap", "nowrap");
	}
}

bool ElementFormControlTextArea::GetWordWrap()
{
	String attribute = GetAttribute<String>("wrap", "");
	return attribute != "nowrap";
}

void ElementFormControlTextArea::Select()
{
	widget->Select();
}

void ElementFormControlTextArea::SetSelectionRange(int selection_start, int selection_end)
{
	widget->SetSelectionRange(selection_start, selection_end);
}

void ElementFormControlTextArea::GetSelection(int* selection_start, int* selection_end, String* selected_text) const
{
	widget->GetSelection(selection_start, selection_end, selected_text);
}

void ElementFormControlTextArea::SetCompositionRange(int range_start, int range_end)
{
	widget->SetCompositionRange(range_start, range_end);
}

bool ElementFormControlTextArea::GetIntrinsicDimensions(Vector2f& dimensions, float& /*ratio*/)
{
	dimensions.x = (float)(GetNumColumns() * ElementUtilities::GetStringWidth(this, "m"));
	dimensions.y = (float)GetNumRows() * Math::Round(GetLineHeight());

	return true;
}

void ElementFormControlTextArea::OnUpdate()
{
	widget->OnUpdate();
}

void ElementFormControlTextArea::OnRender()
{
	widget->OnRender();
}

void ElementFormControlTextArea::OnResize()
{
	widget->OnResize();
}

void ElementFormControlTextArea::OnLayout()
{
	widget->OnLayout();
}

void ElementFormControlTextArea::OnAttributeChange(const ElementAttributes& changed_attributes)
{
	ElementFormControl::OnAttributeChange(changed_attributes);

	if (changed_attributes.find("wrap") != changed_attributes.end())
		SetWordWrapProperties();

	if (changed_attributes.find("rows") != changed_attributes.end() || changed_attributes.find("cols") != changed_attributes.end())
		DirtyLayout();

	auto it = changed_attributes.find("maxlength");
	if (it != changed_attributes.end())
		widget->SetMaxLength(it->second.Get(-1));

	it = changed_attributes.find("value");
	if (it != changed_attributes.end())
		widget->SetValue(it->second.Get<String>());
}

void ElementFormControlTextArea::OnPropertyChange(const PropertyIdSet& changed_properties)
{
	ElementFormControl::OnPropertyChange(changed_properties);

	// Some inherited properties require text formatting update, mainly font and line-height properties.
	const PropertyIdSet changed_inherited_layout_properties = changed_properties &
		(StyleSheetSpecification::GetRegisteredInheritedProperties() & StyleSheetSpecification::GetRegisteredPropertiesForcingLayout());

	if (!changed_inherited_layout_properties.Empty())
		widget->ForceFormattingOnNextLayout();

	if (changed_properties.Contains(PropertyId::Color) || changed_properties.Contains(PropertyId::BackgroundColor))
		widget->UpdateSelectionColours();

	if (changed_properties.Contains(PropertyId::CaretColor))
		widget->GenerateCursor();
}

void ElementFormControlTextArea::GetInnerRML(String& content) const
{
	content = GetValue();
}

void ElementFormControlTextArea::SetWordWrapProperties()
{
	const bool word_wrap = GetWordWrap();
	SetProperty(PropertyId::OverflowX, Property(word_wrap ? Style::Overflow::Hidden : Style::Overflow::Auto));
	SetProperty(PropertyId::OverflowY, Property(Style::Overflow::Auto));
	SetProperty(PropertyId::WhiteSpace, Property(word_wrap ? Style::WhiteSpace::Prewrap : Style::WhiteSpace::Pre));
}

} // namespace Rml
