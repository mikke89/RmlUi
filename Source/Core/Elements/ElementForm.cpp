#include "../../../Include/RmlUi/Core/Elements/ElementForm.h"
#include "../../../Include/RmlUi/Core/Dictionary.h"
#include "../../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../../Include/RmlUi/Core/Elements/ElementFormControl.h"

namespace Rml {

ElementForm::ElementForm(const String& tag) : Element(tag) {}

ElementForm::~ElementForm() {}

void ElementForm::Submit(const String& name, const String& submit_value)
{
	Dictionary values;
	if (name.empty())
		values["submit"] = submit_value;
	else
		values[name] = submit_value;

	ElementList form_controls;
	ElementUtilities::GetElementsByTagName(form_controls, this, "input");
	ElementUtilities::GetElementsByTagName(form_controls, this, "textarea");
	ElementUtilities::GetElementsByTagName(form_controls, this, "select");

	for (size_t i = 0; i < form_controls.size(); i++)
	{
		ElementFormControl* control = rmlui_dynamic_cast<ElementFormControl*>(form_controls[i]);
		if (!control)
			continue;

		// Skip disabled controls.
		if (control->IsDisabled())
			continue;

		// Only process controls that should be submitted.
		if (!control->IsSubmitted())
			continue;

		String control_name = control->GetName();
		String control_value = control->GetValue();

		// Skip over unnamed form controls.
		if (control_name.empty())
			continue;

		// If the item already exists, append to it.
		Variant* value = GetIf(values, control_name);
		if (value != nullptr)
			*value = value->Get<String>() + ", " + control_value;
		else
			values[control_name] = control_value;
	}

	DispatchEvent(EventId::Submit, values);
}

} // namespace Rml
