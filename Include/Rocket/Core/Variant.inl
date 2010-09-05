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

// Constructs a variant with internal data.
template< typename T >
Variant::Variant(const T& t)
{
	data_block = NULL;
	Set( t );
}

// Templatised data accessor.
template< typename T >
bool Variant::GetInto(T& value) const
{	
	switch (GetType())
	{
		case BYTE:
			return TypeConverter< byte, T >::Convert(*(byte*)data_block->data_ptr, value);
		break;

		case CHAR:
			return TypeConverter< char, T >::Convert(*(char*)data_block->data_ptr, value);
		break;

		case FLOAT:
			return TypeConverter< float, T >::Convert(*(float*)data_block->data_ptr, value);
		break;

		case INT:
			return TypeConverter< int, T >::Convert(*(int*)data_block->data_ptr, value);
		break;

		case STRING:
			return TypeConverter< String, T >::Convert(*(String*)data_block->data_ptr, value);
		break;

		case WORD:
			return TypeConverter< word, T >::Convert(*(word*)data_block->data_ptr, value);
		break;

		case VECTOR2:
			return TypeConverter< Vector2f, T >::Convert(*(Vector2f*)data_block->data_ptr, value);
		break;

		case COLOURF:
			return TypeConverter< Colourf, T >::Convert(*(Colourf*)data_block->data_ptr, value);
		break;

		case COLOURB:
			return TypeConverter< Colourb, T >::Convert(*(Colourb*)data_block->data_ptr, value);
		break;

		/*case SCRIPTINTERFACE:
		{
			ScriptInterface* interface = (ScriptInterface*) data_block->data_ptr;
			return TypeConverter< ScriptInterface*, T >::Convert(interface, value);			
		}
		break;

		case VOIDPTR:
			return TypeConverter< void*, T >::Convert(data_block->data_ptr, value);
		break;*/
	}

	return false;
}

// Templatised data accessor.
template< typename T >
T Variant::Get() const
{
	T value;
	GetInto(value);
	return value;
}
