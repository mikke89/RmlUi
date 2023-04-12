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

#ifndef RMLUI_CORE_FILEINTERFACEDEFAULT_H
#define RMLUI_CORE_FILEINTERFACEDEFAULT_H

#include "../../Include/RmlUi/Core/FileInterface.h"

#ifndef RMLUI_NO_FILE_INTERFACE_DEFAULT

namespace Rml {

/**
    Implementation of the RmlUi file interface using the Standard C file functions.

    @author Peter Curry
 */

class FileInterfaceDefault : public FileInterface {
public:
	virtual ~FileInterfaceDefault();

	/// Opens a file.
	/// @param path The path of the file to open.
	/// @return A valid file handle, or nullptr on failure
	FileHandle Open(const String& path) override;
	/// Closes a previously opened file.
	/// @param file The file handle previously opened through Open().
	void Close(FileHandle file) override;

	/// Reads data from a previously opened file.
	/// @param buffer The buffer to be read into.
	/// @param size The number of bytes to read into the buffer.
	/// @param file The handle of the file.
	/// @return The total number of bytes read into the buffer.
	size_t Read(void* buffer, size_t size, FileHandle file) override;
	/// Seeks to a point in a previously opened file.
	/// @param file The handle of the file to seek.
	/// @param offset The number of bytes to seek.
	/// @param origin One of either SEEK_SET (seek from the beginning of the file), SEEK_END (seek from the end of the file) or SEEK_CUR (seek from
	/// the current file position).
	/// @return True if the operation completed successfully, false otherwise.
	bool Seek(FileHandle file, long offset, int origin) override;
	/// Returns the current position of the file pointer.
	/// @param file The handle of the file to be queried.
	/// @return The number of bytes from the origin of the file.
	size_t Tell(FileHandle file) override;
};

} // namespace Rml
#endif /*RMLUI_NO_FILE_INTERFACE_DEFAULT*/

#endif
