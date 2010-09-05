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

template <typename T>
ElementInstancerGeneric<T>::~ElementInstancerGeneric()
{
}
	
// Instances an element given the tag name and attributes
template <typename T>
Element* ElementInstancerGeneric<T>::InstanceElement(Element* /*parent*/, const String& tag, const XMLAttributes& /*attributes*/)
{
	return new T(tag);
}



// Releases the given element
template <typename T>
void ElementInstancerGeneric<T>::ReleaseElement(Element* element)
{
	delete element;
}



// Release the instancer
template <typename T>
void ElementInstancerGeneric<T>::Release()
{
	delete this;
}
