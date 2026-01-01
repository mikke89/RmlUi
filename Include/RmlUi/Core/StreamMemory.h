#pragma once

#include "Header.h"
#include "Stream.h"

namespace Rml {

/**
    Memory Byte Stream Class
 */

class RMLUICORE_API StreamMemory final : public Stream {
public:
	/// Empty memory stream with default size buffer
	StreamMemory();
	/// Empty memory stream with specified buffer size
	StreamMemory(size_t initial_size);
	/// Read only memory stream based on the existing buffer
	StreamMemory(const byte* buffer, size_t buffer_size);
	virtual ~StreamMemory();

	/// Close the stream
	void Close() override;

	/// Are we at the end of the stream
	bool IsEOS() const override;

	/// Size of this stream ( in bytes )
	size_t Length() const override;

	/// Get Stream position ( in bytes )
	size_t Tell() const override;

	/// Set Stream position ( in bytes )
	bool Seek(long offset, int origin) const override;

	/// Read from the stream
	using Stream::Read;
	size_t Read(void* buffer, size_t bytes) const override;

	/// Peek into the stream
	size_t Peek(void* buffer, size_t bytes) const override;

	/// Write to the stream
	using Stream::Write;
	size_t Write(const void* buffer, size_t bytes) override;

	/// Truncate the stream to the specified length
	size_t Truncate(size_t bytes) override;

	/// Push onto the front of the stream
	size_t PushFront(const void* buffer, size_t bytes) override;

	/// Pop from the front of the stream
	size_t PopFront(size_t bytes) override;

	/// Raw access to the stream
	const byte* RawStream() const;

	/// Erase a section of the stream
	void Erase(size_t offset, size_t bytes);

	/// Does the stream have data available for reading
	bool IsReadReady() override;

	/// Is the stream able to accept data now
	bool IsWriteReady() override;

	/// Sets this streams source URL, useful data that is stored
	/// in memory streams that originated from files
	void SetSourceURL(const URL& url);

private:
	byte* buffer;
	mutable byte* buffer_ptr;
	size_t buffer_size;
	size_t buffer_used;
	bool owns_buffer;

	bool Reallocate(size_t size);
};

} // namespace Rml
