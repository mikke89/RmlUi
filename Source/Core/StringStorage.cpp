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
#include <Rocket/Core/StringStorage.h>

namespace Rocket {
namespace Core {

const int MIN_STRING = 16;		// Smallest string size.
const int NUM_POOLS = 4;		// Number of pools we have, in powers of 2 starting from the smallest string (16, 32, 64 and 128)
const int ROCKET_MAX_POOL_SIZE = 128;	// Maximum number of free strings to keep in a pool
const int ROCKET_MAX_HASH_LENGTH = 32;	// Maximum number of character to use when calculating the string hash (saves hashing HUGE strings)

#define HASH(hval, string, length)										\
{																		\
	hval = 0;															\
	unsigned char *bp = (unsigned char *)string;						\
	unsigned char *be = bp + length;									\
																		\
	while (bp < be)														\
	{																	\
		hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);	\
		hval ^= *bp++;													\
	}																	\
}


struct StringEntry
{
	char* buffer;
	size_t reference_count;
	StringEntry* next;
	StringEntry* prev;
	Hash hash;
};

struct Storage
{
	typedef std::map<Hash, StringEntry*> Lookup;
	Lookup lookup;

	typedef std::list< char* > StringPool;
	StringPool pools[NUM_POOLS];
};

static char empty_string_buffer[] = {0, 0, 0, 0};
char* StringStorage::empty_string = empty_string_buffer;
static Storage* storage = NULL;

#define BLOCK_SIZE(string_size) (string_size >= 1024 ? 1024 : (string_size >= 128 ? 128 : (string_size > MIN_STRING ? Math::ToPowerOfTwo(string_size) : MIN_STRING)))
#define ALLOC_SIZE(string_size) (((string_size / BLOCK_SIZE(string_size)) + 1) * BLOCK_SIZE(string_size))

// Clears all shared strings from the string pools.
void StringStorage::ClearPools()
{
	for (int i = 0; i < NUM_POOLS; ++i)
	{
		for (Storage::StringPool::iterator iterator = storage->pools[i].begin(); iterator != storage->pools[i].end(); ++iterator)
			free(*iterator);

		storage->pools[i].clear();
	}
}

char* StringStorage::ReallocString(char* string, size_t old_length, size_t new_length, size_t character_size)
{
	ROCKET_ASSERT(old_length < (1 << 24));
	ROCKET_ASSERT(new_length < (1 << 24));
	ROCKET_ASSERT(new_length >= old_length);
	size_t new_size = (new_length + 1) * character_size;

	if (string == empty_string)
	{
		string = NULL;
	}
	else if (string != NULL)
	{
		size_t old_size = (old_length + 1) * character_size;		
		if (new_size < ALLOC_SIZE(old_size))
			return string;
	}

	size_t alloc_size = ALLOC_SIZE(new_size);
	ROCKET_ASSERT(alloc_size > new_size);

	// Check if we can use an old allocation from our pools
	if (alloc_size < (MIN_STRING << NUM_POOLS))
	{
		// Ensure we have storage for strings
		if (!storage)
			storage = new Storage();

		size_t pool_index = 0;
		size_t size = alloc_size;
		while (size > MIN_STRING)
		{
			pool_index++;
			size = size >> 1;
		}
		ROCKET_ASSERT(pool_index < NUM_POOLS);

		if (!storage->pools[pool_index].empty())
		{
			char* new_string = storage->pools[pool_index].front();
			storage->pools[pool_index].pop_front();
			if (string)
			{
				// Copy the correct number of values across and null terminate
				size_t copy_size = (old_length < new_length ? old_length : new_length) * character_size;
				memcpy(new_string, string, copy_size);
				memset(&new_string[copy_size], 0, character_size);

				ReleaseString(string, old_length);
			}
			return new_string;
		}
	}	

	// Standard realloc
	return (char*) realloc(string, alloc_size);
}

void StringStorage::ReleaseString(char* string, size_t size)
{
	if (string == empty_string)
		return;

	if (storage == NULL)
	{
		free(string);
		return;
	}

	size_t alloc_size = ALLOC_SIZE(size);
	if (alloc_size < (MIN_STRING << NUM_POOLS))
	{
		size_t pool_index = 0;
		size_t size = alloc_size;
		while (size > MIN_STRING)
		{
			pool_index++;
			size = size >> 1;
		}
		ROCKET_ASSERT(pool_index < NUM_POOLS);

		if (storage->pools[pool_index].size() < ROCKET_MAX_POOL_SIZE)
		{
			storage->pools[pool_index].push_back(string);
			return;
		}
	}

	free(string);
}

StringStorage::StringID StringStorage::AddString(const char* &string, size_t string_length, size_t character_size)
{
	size_t length = string_length * character_size;

	// Ensure we have storage for strings
	if (!storage)
		storage = new Storage();

	// Hash the incoming string
	Hash hash;
	size_t hash_length = (length < ROCKET_MAX_HASH_LENGTH ? length : ROCKET_MAX_HASH_LENGTH);
	HASH(hash, string, hash_length);		

	// See if we can find the entry group for this hash ( strings with the same hash )
	Storage::Lookup::iterator itr = storage->lookup.find(hash);
	
	StringEntry* entry_group = NULL;
	if (itr != storage->lookup.end())	
	{		
		// If we found it, iterate all the strings in the group
		// looking for this specific string, if its found,
		// increase reference count and return it
		entry_group = (*itr).second;
		StringEntry* entry = entry_group;
		while (entry)
		{
			// If the memory check passes and the null terminator exists in the correct place
			if (memcmp(entry->buffer, string, length) == 0 && memcmp(&entry->buffer[length], empty_string, character_size) == 0)
			{				
				free((char*)string);

				entry->reference_count++;
				string = entry->buffer;
				return (StringID)entry;
			}

			entry = entry->next;
		}		
	}

	// Create a new entry
	StringEntry* entry = new StringEntry();
	entry->reference_count = 1;
	entry->next = NULL;
	entry->prev = NULL;
	entry->hash = hash;	
	entry->buffer = (char*)string;

	// If we found an entry group for this string earlier,
	// insert the string size_to this entry group
	if (entry_group)
	{				
		if (entry_group->next)
		{
			entry_group->next->prev = entry;
			entry->next = entry_group->next;		
		}
		entry->prev = entry_group;
		entry_group->next = entry;		
	}
	else
	{
		// Otherwise add as a new entry group
		storage->lookup[hash] = entry;
	}

	return (StringID)entry;
}


void StringStorage::AddReference(StringID string_id)
{
	if (string_id == 0)
		return;

	// Simply increase the reference count on the given string id
	StringEntry* entry = (StringEntry*)string_id;
	entry->reference_count++;
}

void StringStorage::RemoveReference(StringID string_id)
{
	if (string_id == 0)
		return;

	StringEntry* entry = (StringEntry*)string_id;

	ROCKET_ASSERT(entry->reference_count > 0);
	entry->reference_count--;
	if (entry->reference_count > 0)
		return;

	if (storage)
	{
		// If this is the only string in the hash group (hopefully most common case), 
		// then we just remove it from the lookup table
		if (entry->prev == NULL && entry->next == NULL)
		{
			storage->lookup.erase(entry->hash);
		}

		// If we have a next and a previous, just remove us from the middle
		else if (entry->prev && entry->next)
		{
			entry->prev->next = entry->next;
			entry->next->prev = entry->prev;
		}

		// If we have a next only, we need to update the map index
		else if (entry->next)
		{
			storage->lookup[entry->hash] = entry->next;
			entry->next->prev = NULL;
		}
		
		// Otherwise we only have a previous, just remove us from the chain
		else
		{
			entry->prev->next = NULL;
		}
		
		ROCKET_ASSERT(storage->lookup.find(entry->hash) == storage->lookup.end() || (*storage->lookup.find(entry->hash)).second != entry);
	}

	free(entry->buffer);
	delete entry;
}

void StringStorage::OnLibraryShutdown()
{
	ClearPools();
	delete storage;
	storage = NULL;
}

}
}
