/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#include "precompiled.h"
#include <Rocket/Core/Variant.h>

namespace Rocket {
namespace Core {

Variant::Variant()
{
	data_block = NULL;
	
	// Make sure our object size assumptions fit inside the static buffer
	ROCKET_STATIC_ASSERT(sizeof(String) <= DataBlock::BUFFER_SIZE, Invalid_Size_String);
	ROCKET_STATIC_ASSERT(sizeof(Colourb) <= DataBlock::BUFFER_SIZE, Invalid_Size_Colourb);
	ROCKET_STATIC_ASSERT(sizeof(Colourf) <= DataBlock::BUFFER_SIZE, Invalid_Size_Colourf);
}

Variant::Variant( const Variant& copy )
{
	data_block = NULL;
	Set(copy);
}

Variant::~Variant() 
{
	ReleaseDataBlock();
}

void Variant::Clear()
{
	ReleaseDataBlock();
}

Variant::Type Variant::GetType() const
{
	if (!data_block)
		return NONE;
	
	return data_block->type;
}

//////////////////////////////////////////////////
// Set methods
//////////////////////////////////////////////////

#define SET_VARIANT(type) *((type*)data_block->data_ptr) = value;

void Variant::Set(const Variant& copy)
{
	ReleaseDataBlock();
	if (copy.data_block)
		copy.data_block->reference_count++;
	data_block = copy.data_block;
}

void Variant::Set(const byte value)
{
	NewDataBlock(BYTE);
	SET_VARIANT(byte);
}

void Variant::Set(const char value)
{
	NewDataBlock(CHAR);
	SET_VARIANT(char);
}

void Variant::Set(const float value)
{
	NewDataBlock(FLOAT);
	SET_VARIANT(float);
}

void Variant::Set(const int value)
{
	NewDataBlock( INT );
	SET_VARIANT(int);
}

void Variant::Set(const String& value) {
	NewDataBlock(STRING);
	((String*) data_block->data_ptr)->Assign( value );
}

void Variant::Set(const word value)
{
	NewDataBlock(WORD);
	SET_VARIANT(word);  
}

void Variant::Set(const char* value) 
{
	Set(String(value));
}

void Variant::Set(void* voidptr) 
{
	NewDataBlock(VOIDPTR);
	data_block->data_ptr = voidptr;
}

/*void Variant::Set(const Vector3f& value)
{
	NewDataBlock(VECTOR3);
	SET_VARIANT(Vector3f);  
}*/

void Variant::Set(const Vector2f& value)
{
	NewDataBlock(VECTOR2);
	SET_VARIANT(Vector2f);
}

void Variant::Set(const Colourf& value)
{
	NewDataBlock(COLOURF);
	SET_VARIANT(Colourf);
}

void Variant::Set(const Colourb& value)
{
	NewDataBlock(COLOURB);
	SET_VARIANT(Colourb);
}

void Variant::Set(ScriptInterface* value) 
{
	NewDataBlock(SCRIPTINTERFACE);
	data_block->data_ptr = value;
}

Variant& Variant::operator=(const Variant& copy)
{
	Set(copy);
	return *this;
}

void Variant::NewDataBlock(Type type) 
{  
	// Only actually have to make a new data block if 
	// somebody else is referencing our existing one
	if ( !data_block || data_block->reference_count > 1 )
	{
		ReleaseDataBlock();
		data_block = new DataBlock();
	}
	if (data_block->type != STRING || type == STRING)
		new(data_block->data_ptr) String();

	data_block->type = type;
}

void Variant::ReleaseDataBlock() 
{
	if (!data_block)
		return;

	data_block->reference_count--;
	ROCKET_ASSERT(data_block->reference_count >= 0);
	if (data_block->reference_count == 0) 
	{
		delete data_block;
	}  
	data_block = NULL;
}

Variant::DataBlock::DataBlock() 
{
	type = NONE;
	reference_count = 1;
	data_ptr = data;
}

Variant::DataBlock::~DataBlock() 
{
	Clear();
}

// Clear allocated
void Variant::DataBlock::Clear() 
{
	// Should only clear when we have no references
	ROCKET_ASSERT(reference_count == 0);

	// Free any allocated types.
	switch (type) 
	{      
		case STRING:
		{
			// Clean up the string.
			String* string = (String*)data_ptr;
			string->~String();
		}
		break;

		default:
		break;
	}
	type = NONE;
	data_ptr = data;
}

}
}
