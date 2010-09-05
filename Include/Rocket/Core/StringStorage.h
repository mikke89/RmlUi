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

#ifndef ROCKETCORESTRINGSTORAGE_H
#define ROCKETCORESTRINGSTORAGE_H

#include <stddef.h>

namespace Rocket {
namespace Core {

/**
	Storage interface for StringBase
	@author Lloyd Weehuizen
 */

class ROCKETCORE_API StringStorage
{
public:
	typedef void* StringID;
	static char* empty_string;

	/// Clears all shared strings from the string pools.
	static void ClearPools();

	/// Alloc/Realloc a string
	static char* ReallocString(char* string, size_t old_length, size_t new_length, size_t character_size);
	/// Release a previously allocated string
	static void ReleaseString(char* string, size_t length);

	/// Adds or increases the reference count on the given string
	/// @param string[in,out] String to add to the storage, returns new address of string
	/// @param string_length The length of the new string
	/// @param character_size Number of bytes of an individual parameter	
	static StringID AddString(const char* &string, size_t string_length, size_t character_size);
	/// Adds a reference to the given string id
	/// @param id Id of the string to add a reference to
	static void AddReference(StringID id);
	/// Removes a reference from the given string id
	/// @param id Id of the string to remove a reference from
	static void RemoveReference(StringID id);

private:
	/// Shutdown the storage pool
	static void OnLibraryShutdown();

	friend class LibraryMain;
};

}
}

#endif
