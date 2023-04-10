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

#include "StreamFile.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/FileInterface.h"
#include "../../Include/RmlUi/Core/StringUtilities.h"

namespace Rml {

StreamFile::StreamFile()
{
	file_handle = 0;
	length = 0;
}

StreamFile::~StreamFile()
{
	if (file_handle)
		StreamFile::Close();
}

bool StreamFile::Open(const String& path)
{
	String url_safe_path = StringUtilities::Replace(path, ':', '|');
	SetStreamDetails(URL(url_safe_path), Stream::MODE_READ);

	if (file_handle)
		Close();

	// Fix the path if a leading colon has been replaced with a pipe.
	String fixed_path = StringUtilities::Replace(path, '|', ':');
	file_handle = GetFileInterface()->Open(fixed_path);
	if (!file_handle)
	{
		Log::Message(Log::LT_WARNING, "Unable to open file %s.", fixed_path.c_str());
		return false;
	}

	GetLength();

	return true;
}

void StreamFile::Close()
{
	if (file_handle)
	{
		GetFileInterface()->Close(file_handle);
		file_handle = 0;
	}

	length = 0;
	Stream::Close();
}

size_t StreamFile::Length() const
{
	return length;
}

size_t StreamFile::Tell() const
{
	return GetFileInterface()->Tell(file_handle);
}

bool StreamFile::Seek(long offset, int origin) const
{
	return GetFileInterface()->Seek(file_handle, offset, origin);
}

size_t StreamFile::Read(void* buffer, size_t bytes) const
{
	return GetFileInterface()->Read(buffer, bytes, file_handle);
}

size_t StreamFile::Write(const void* /*buffer*/, size_t /*bytes*/)
{
	RMLUI_ERROR;
	return 0;
}

size_t StreamFile::Truncate(size_t /*bytes*/)
{
	RMLUI_ERROR;
	return 0;
}

bool StreamFile::IsReadReady()
{
	return Tell() < Length();
}

bool StreamFile::IsWriteReady()
{
	return false;
}
void StreamFile::GetLength()
{
	length = GetFileInterface()->Length(file_handle);
}

} // namespace Rml
