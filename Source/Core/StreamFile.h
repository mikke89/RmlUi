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

#ifndef RMLUI_CORE_STREAMFILE_H
#define RMLUI_CORE_STREAMFILE_H

#include "../../Include/RmlUi/Core/Stream.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

/**
    @author Peter Curry
 */

class StreamFile final : public Stream {
public:
	StreamFile();
	virtual ~StreamFile();

	/// Attempts to open the stream pointing at a given location.
	bool Open(const String& path);
	/// Closes the stream.
	void Close() override;

	/// Returns the size of this stream (in bytes).
	size_t Length() const override;

	/// Returns the position of the stream pointer (in bytes).
	size_t Tell() const override;
	/// Sets the stream position (in bytes).
	bool Seek(long offset, int origin) const override;

	/// Read from the stream.
	size_t Read(void* buffer, size_t bytes) const override;
	using Stream::Read;

	/// Write to the stream at the current position.
	size_t Write(const void* buffer, size_t bytes) override;
	using Stream::Write;

	/// Truncate the stream to the specified length.
	size_t Truncate(size_t bytes) override;

	/// Returns true if the stream is ready for reading, false otherwise.
	bool IsReadReady() override;
	/// Returns false.
	bool IsWriteReady() override;

private:
	// Determines the length of the stream.
	void GetLength();

	FileHandle file_handle;
	size_t length;
};

} // namespace Rml
#endif
