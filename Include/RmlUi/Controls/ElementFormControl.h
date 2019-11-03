/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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

#ifndef RMLUICONTROLSELEMENTFORMCONTROL_H
#define RMLUICONTROLSELEMENTFORMCONTROL_H

#include "../Core/Element.h"
#include "Header.h"

namespace Rml {
namespace Controls {

/**
	A generic specialisation of the generic Core::Element for all input controls.

	@author Peter Curry
 */

class RMLUICONTROLS_API ElementFormControl : public Core::Element
{
public:
	RMLUI_RTTI_DefineWithParent(ElementFormControl, Core::Element)

	/// Constructs a new ElementFormControl. This should not be called directly; use the Factory
	/// instead.
	/// @param[in] tag The tag the element was declared as in RML.
	ElementFormControl(const Rml::Core::String& tag);
	virtual ~ElementFormControl();

	/// Returns the name of the form control. This is not guaranteed to be unique, and in the case of some form
	/// controls (such as radio buttons) most likely will not be.
	/// @return The name of the form control.
	Rml::Core::String GetName() const;
	/// Sets the name of the form control.
	/// @param[in] name The new name of the form control.
	void SetName(const Rml::Core::String& name);

	/// Returns a string representation of the current value of the form control.
	/// @return The value of the form control.
	virtual Rml::Core::String GetValue() const = 0;
	/// Sets the current value of the form control.
	/// @param[in] value The new value of the form control.
	virtual void SetValue(const Rml::Core::String& value) = 0;
	/// Returns if this value should be submitted with the form.
	/// @return True if the value should be be submitted with the form, false otherwise.
	virtual bool IsSubmitted();

	/// Returns the disabled status of the form control.
	/// @return True if the element is disabled, false otherwise.
	bool IsDisabled() const;
	/// Sets the disabled status of the form control.
	/// @param[in] disable True to disable the element, false to enable.
	void SetDisabled(bool disable);

protected:
	/// Checks for changes to the 'disabled' attribute.
	/// @param[in] changed_attributes List of changed attributes on the element.
	void OnAttributeChange(const Core::ElementAttributes& changed_attributes) override;
};

}
}

#endif
