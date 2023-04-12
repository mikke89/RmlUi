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

#include "../../Include/RmlUi/Core/Stream.h"
#include <stdio.h>
#include <string.h>

namespace Rml {

const size_t READ_BLOCK_SIZE = 1024;

Stream::Stream()
{
	stream_mode = 0;
}

Stream::~Stream()
{
	Stream::Close();
}

void Stream::Close()
{
	stream_mode = 0;
}

int Stream::GetStreamMode() const
{
	return stream_mode;
}

const URL& Stream::GetSourceURL() const
{
	return url;
}

bool Stream::IsEOS() const
{
	return Tell() >= Length();
}

size_t Stream::Peek(void* buffer, size_t bytes) const
{
	size_t pos = Tell();
	size_t read = Read(buffer, bytes);
	Seek((long)pos, SEEK_SET);
	return read;
}

size_t Stream::Read(Stream* stream, size_t bytes) const
{
	byte buffer[READ_BLOCK_SIZE];
	size_t total_bytes_read = 0;
	while (total_bytes_read < bytes)
	{
		size_t bytes_read = this->Read(buffer, Math::Min(READ_BLOCK_SIZE, bytes - total_bytes_read));
		if (bytes_read < 1)
			return total_bytes_read;
		stream->Write(buffer, bytes_read);
		total_bytes_read += bytes_read;
	}
	return total_bytes_read;
}

size_t Stream::Read(String& string, size_t bytes) const
{
	size_t string_size = string.size();
	string.resize(string_size + bytes + 1);
	size_t read = Read(&string[string_size], bytes);
	string[string_size + read] = '\0';
	string.resize(string_size + read);
	return read;
}

size_t Stream::Write(const Stream* stream, size_t bytes)
{
	return stream->Read(this, bytes);
}

size_t Stream::Write(const char* string)
{
	return Write(string, strlen(string));
}

size_t Stream::Write(const String& string)
{
	return Write(string.c_str(), string.size());
}

size_t Stream::PushFront(const void* /*buffer*/, size_t /*bytes*/)
{
	RMLUI_ERRORMSG("No generic way to PushFront to a stream.");
	return false;
}

size_t Stream::PushBack(const void* buffer, size_t bytes)
{
	size_t pos = Tell();
	Seek(0, SEEK_END);
	size_t wrote = Write(buffer, bytes);
	Seek((long)pos, SEEK_SET);
	return wrote;
}

size_t Stream::PopFront(size_t /*bytes*/)
{
	RMLUI_ERRORMSG("No generic way to PopFront from a stream.");
	return 0;
}

size_t Stream::PopBack(size_t bytes)
{
	return Truncate(Length() - bytes);
}

void Stream::SetStreamDetails(const URL& _url, int _stream_mode)
{
	url = _url;
	stream_mode = _stream_mode;
}

} // namespace Rml
