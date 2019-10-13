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

namespace Rml {
namespace Core {

// Returns the values of one of this element's properties.
template < typename T >
T Element::GetProperty(const String& name)
{
	const Property* property = GetProperty(name);
	if (!property)
	{
		Log::Message(Log::LT_WARNING, "Invalid property name %s.", name.c_str());
		return T{};
	}
	return property->Get< T >();
}

// Sets an attribute on the element.
template< typename T >
void Element::SetAttribute(const String& name, const T& value)
{
	Variant variant(value);
	attributes[name] = variant;
	ElementAttributes changed_attributes = { {name, variant} };
	OnAttributeChange(changed_attributes);
}

// Gets the specified attribute, with default value.
template< typename T >
T Element::GetAttribute(const String& name, const T& default_value) const
{
	return Get(attributes, name, default_value);
}

}
}
