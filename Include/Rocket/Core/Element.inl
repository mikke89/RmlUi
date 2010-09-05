/*
 * Copyright (c) 2006 - 2008
 * Wandering Monster Studios Limited
 *
 * Any use of this program is governed by the terms of Wandering Monster
 * Studios Limited's Licence Agreement included with this program, a copy
 * of which can be obtained by contacting Wandering Monster Studios
 * Limited at info@wanderingmonster.co.nz.
 *
 */

// Returns the values of one of this element's properties.
// We can assume the property will exist based on the RCSS inheritance.
template < typename T >
T Element::GetProperty(const String& name)
{
	const Property* property = GetProperty(name);
	EMP_ASSERTMSG(property, "Invalid property name.");
	return property->Get< T >();
}

// Sets an attribute on the element.
template< typename T >
void Element::SetAttribute(const String& name, const T& value)
{
	attributes.Set(name, value);
	AttributeNameList changed_attributes;
	changed_attributes.insert(name);

	OnAttributeChange(changed_attributes);
}

// Gets the specified attribute, with default value.
template< typename T >
T Element::GetAttribute(const String& name, const T& default_value) const
{			
	return attributes.Get(name, default_value);
}

// Iterates over the attributes.
template< typename T >
bool Element::IterateAttributes(int& index, String& name, T& value) const
{
	return attributes.Iterate(index, name, value);
}
