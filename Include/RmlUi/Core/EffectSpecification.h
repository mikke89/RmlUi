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

#ifndef RMLUI_CORE_EFFECTSPECIFICATION_H
#define RMLUI_CORE_EFFECTSPECIFICATION_H

#include "Header.h"
#include "PropertySpecification.h"
#include "Types.h"

namespace Rml {

class PropertyDefinition;

/**
    Specifies properties and shorthands for effects (decorators and filters).
 */
class RMLUICORE_API EffectSpecification {
public:
	EffectSpecification();

	/// Returns the property specification associated with the instancer.
	const PropertySpecification& GetPropertySpecification() const;

protected:
	~EffectSpecification();

	/// Registers a property for the effect.
	/// @param[in] property_name The name of the new property (how it is specified through RCSS).
	/// @param[in] default_value The default value to be used.
	/// @return The new property definition, ready to have parsers attached.
	PropertyDefinition& RegisterProperty(const String& property_name, const String& default_value);

	/// Registers a shorthand property definition. Specify a shorthand name of 'decorator' or 'filter' to parse
	/// anonymous decorators or filters, respectively.
	/// @param[in] shorthand_name The name to register the new shorthand property under.
	/// @param[in] property_names A comma-separated list of the properties this definition is a shorthand for. The order
	/// in which they are specified here is the order in which the values will be processed.
	/// @param[in] type The type of shorthand to declare.
	/// @return An ID for the new shorthand, or 'Invalid' if the shorthand declaration is invalid.
	ShorthandId RegisterShorthand(const String& shorthand_name, const String& property_names, ShorthandType type);

private:
	PropertySpecification properties;
};

} // namespace Rml
#endif
