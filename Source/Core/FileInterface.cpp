#include "../../Include/RmlUi/Core/FileInterface.h"
#include "../../Include/RmlUi/Core/Log.h"

namespace Rml {

FileInterface::FileInterface() {}

FileInterface::~FileInterface() {}

size_t FileInterface::Length(FileHandle file)
{
	size_t current_position = Tell(file);
	Seek(file, 0, SEEK_END);
	size_t length = Tell(file);
	Seek(file, (long)current_position, SEEK_SET);
	return length;
}

bool FileInterface::LoadFile(const String& path, String& out_data)
{
	FileHandle handle = Open(path);
	if (!handle)
		return false;

	const size_t length = Length(handle);

	out_data = String(length, 0);

	const size_t read_length = Read(&out_data[0], length, handle);

	if (length != read_length)
	{
		Log::Message(Log::LT_WARNING, "Could only read %zu of %zu bytes from file %s", read_length, length, path.c_str());
	}

	Close(handle);

	return true;
}

} // namespace Rml
