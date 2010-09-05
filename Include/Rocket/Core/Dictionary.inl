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

template< typename T >
inline void Dictionary::Set(const String& key, const T& value)
{
	Set(key, Variant(value));
}

template< typename T >
inline T Dictionary::Get(const String& key, const T& default_value) const
{
	T value;
	if (!GetInto(key, value))
		return default_value;

	return value;
}


template <typename T>
inline bool Dictionary::GetInto(const String& key, T& value) const
{
	Variant* variant = Get(key);
	if (!variant)
		return false;
		
	return variant->GetInto<T>(value);	
}

template <typename T>
inline bool Dictionary::Iterate(int &pos, String& key, T& value) const
{
	Variant* variant;
	bool iterate = Iterate(pos, key, variant);
	if (iterate)
		variant->GetInto(value);
	return iterate;
}
