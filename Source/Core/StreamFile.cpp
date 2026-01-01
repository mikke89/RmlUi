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
