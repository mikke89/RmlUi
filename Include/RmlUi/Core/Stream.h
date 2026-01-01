#pragma once

#include "Header.h"
#include "Traits.h"
#include "Types.h"
#include "URL.h"

namespace Rml {

class StreamListener;

/**
    Abstract class for a media-independent byte stream.
 */

class RMLUICORE_API Stream : public NonCopyMoveable {
public:
	// Stream modes.
	enum StreamMode {
		MODE_WRITE = 1 << 0,
		MODE_APPEND = 1 << 1,
		MODE_READ = 1 << 2,
		MODE_ASYNC = 1 << 3,

		MODE_MASK = MODE_WRITE | MODE_APPEND | MODE_READ
	};

	Stream();
	virtual ~Stream();

	/// Closes the stream.
	virtual void Close();

	/// Returns the mode the stream was opened in.
	int GetStreamMode() const;

	/// Obtain the source url of this stream (if available)
	const URL& GetSourceURL() const;

	/// Are we at the end of the stream
	virtual bool IsEOS() const;

	/// Returns the size of this stream (in bytes).
	virtual size_t Length() const = 0;

	/// Returns the position of the stream pointer (in bytes).
	virtual size_t Tell() const = 0;
	/// Sets the stream position (in bytes).
	virtual bool Seek(long offset, int origin) const = 0;

	/// Read from the stream.
	virtual size_t Read(void* buffer, size_t bytes) const = 0;
	/// Read from the stream into another stream.
	virtual size_t Read(Stream* stream, size_t bytes) const;
	/// Read from the stream and append to the string buffer
	virtual size_t Read(String& buffer, size_t bytes) const;
	/// Read from the stream, without increasing the stream offset.
	virtual size_t Peek(void* buffer, size_t bytes) const;

	/// Write to the stream at the current position.
	virtual size_t Write(const void* buffer, size_t bytes) = 0;
	/// Write to this stream from another stream.
	virtual size_t Write(const Stream* stream, size_t bytes);
	/// Write a character array to the stream.
	virtual size_t Write(const char* string);
	/// Write a string to the stream
	virtual size_t Write(const String& string);

	/// Truncate the stream to the specified length.
	virtual size_t Truncate(size_t bytes) = 0;

	/// Push onto the front of the stream.
	virtual size_t PushFront(const void* buffer, size_t bytes);
	/// Push onto the back of the stream.
	virtual size_t PushBack(const void* buffer, size_t bytes);

	/// Pop from the front of the stream.
	virtual size_t PopFront(size_t bytes);
	/// Pop from the back of the stream.
	virtual size_t PopBack(size_t bytes);

	/// Returns true if the stream is ready for reading, false otherwise.
	/// This is usually only implemented on streams supporting asynchronous
	/// operations.
	virtual bool IsReadReady() = 0;
	/// Returns true if the stream is ready for writing, false otherwise.
	/// This is usually only implemented on streams supporting asynchronous
	/// operations.
	virtual bool IsWriteReady() = 0;

protected:
	/// Sets the mode on the stream; should be called by a stream when it is opened.
	void SetStreamDetails(const URL& url, int stream_mode);

private:
	URL url;
	int stream_mode;
};

} // namespace Rml
