#pragma once

#include "../../Include/RmlUi/Core/Stream.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

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
