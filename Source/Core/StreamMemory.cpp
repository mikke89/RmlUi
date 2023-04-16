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

#include "../../Include/RmlUi/Core/StreamMemory.h"
#include <stdio.h>
#include <string.h>

namespace Rml {

const int DEFAULT_BUFFER_SIZE = 256;
const int BUFFER_INCREMENTS = 256;

StreamMemory::StreamMemory()
{
	buffer = nullptr;
	buffer_ptr = nullptr;
	buffer_size = 0;
	buffer_used = 0;
	owns_buffer = true;
	Reallocate(DEFAULT_BUFFER_SIZE);
}

StreamMemory::StreamMemory(size_t initial_size)
{
	buffer = nullptr;
	buffer_ptr = nullptr;
	buffer_size = 0;
	buffer_used = 0;
	owns_buffer = true;
	Reallocate(initial_size);
}

StreamMemory::StreamMemory(const byte* _buffer, size_t _buffer_size)
{
	buffer = (byte*)_buffer;
	buffer_size = _buffer_size;
	buffer_used = _buffer_size;
	owns_buffer = false;
	buffer_ptr = buffer;
}

StreamMemory::~StreamMemory()
{
	if (owns_buffer)
		free(buffer);
}

void StreamMemory::Close()
{
	Stream::Close();
}

bool StreamMemory::IsEOS() const
{
	return buffer_ptr >= buffer + buffer_used;
}

size_t StreamMemory::Tell() const
{
	return buffer_ptr - buffer;
}

size_t StreamMemory::Length() const
{
	return buffer_used;
}

size_t StreamMemory::Read(void* _buffer, size_t bytes) const
{
	bytes = Math::Min(bytes, (size_t)(buffer + buffer_used - buffer_ptr));

	memcpy(_buffer, buffer_ptr, bytes);

	buffer_ptr += bytes;

	return bytes;
}

size_t StreamMemory::Peek(void* _buffer, size_t bytes) const
{
	bytes = Math::Min(bytes, (size_t)(buffer + buffer_used - buffer_ptr));

	memcpy(_buffer, buffer_ptr, bytes);

	return bytes;
}

size_t StreamMemory::Write(const void* _buffer, size_t bytes)
{
	if (buffer_ptr + bytes > buffer + buffer_size)
		if (!Reallocate(bytes + BUFFER_INCREMENTS))
			return 0;

	memcpy(buffer_ptr, _buffer, bytes);

	buffer_ptr += bytes;
	buffer_used = Math::Max((size_t)(buffer_ptr - buffer), buffer_used);

	return bytes;
}

size_t StreamMemory::Truncate(size_t bytes)
{
	if (bytes > buffer_used)
		return 0;

	size_t old_size = buffer_used;
	buffer_used = bytes;
	buffer_ptr = buffer + bytes;
	return old_size - buffer_used;
}

bool StreamMemory::Seek(long offset, int origin) const
{
	byte* new_ptr = nullptr;

	switch (origin)
	{
	case SEEK_SET: new_ptr = buffer + offset; break;
	case SEEK_END: new_ptr = buffer + (buffer_used - offset); break;
	case SEEK_CUR: new_ptr = buffer_ptr + offset;
	}

	// Check of overruns
	if (new_ptr < buffer || new_ptr > buffer + buffer_used)
		return false;

	buffer_ptr = new_ptr;

	return true;
}

size_t StreamMemory::PushFront(const void* _buffer, size_t bytes)
{
	if (buffer_used + bytes > buffer_size)
		if (!Reallocate(bytes + BUFFER_INCREMENTS))
			return 0;

	memmove(&buffer[bytes], &buffer[0], buffer_used);
	memcpy(buffer, _buffer, bytes);
	buffer_used += bytes;
	buffer_ptr += bytes;
	return bytes;
}

size_t StreamMemory::PopFront(size_t bytes)
{
	Erase(0, bytes);
	buffer_ptr -= bytes;
	buffer_ptr = Math::Max(buffer_ptr, buffer);
	return bytes;
}

const byte* StreamMemory::RawStream() const
{
	return buffer;
}

void StreamMemory::Erase(size_t offset, size_t bytes)
{
	bytes = Math::Min(bytes, buffer_used - offset);
	memmove(&buffer[offset], &buffer[offset + bytes], buffer_used - offset - bytes);
	buffer_used -= bytes;
}

bool StreamMemory::IsReadReady()
{
	return !IsEOS();
}

bool StreamMemory::IsWriteReady()
{
	return true;
}

void StreamMemory::SetSourceURL(const URL& url)
{
	SetStreamDetails(url, Stream::MODE_READ | (owns_buffer ? Stream::MODE_WRITE : 0));
}

bool StreamMemory::Reallocate(size_t size)
{
	RMLUI_ASSERT(owns_buffer);
	if (!owns_buffer)
		return false;

	byte* new_buffer = (byte*)realloc(buffer, buffer_size + size);
	if (new_buffer == nullptr)
		return false;

	buffer_ptr = new_buffer + (buffer_ptr - buffer);

	buffer = new_buffer;
	buffer_size += size;

	return true;
}

} // namespace Rml
