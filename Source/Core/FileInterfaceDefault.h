#pragma once

#include "../../Include/RmlUi/Core/FileInterface.h"

#ifndef RMLUI_NO_FILE_INTERFACE_DEFAULT

namespace Rml {

/**
    Implementation of the RmlUi file interface using the Standard C file functions.
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
