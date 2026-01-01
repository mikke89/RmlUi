#include "../include/ShellFileInterface.h"
#include <stdio.h>

ShellFileInterface::ShellFileInterface(const Rml::String& root) : root(root) {}

ShellFileInterface::~ShellFileInterface() {}

Rml::FileHandle ShellFileInterface::Open(const Rml::String& path)
{
	// Attempt to open the file relative to the application's root.
	FILE* fp = fopen((root + path).c_str(), "rb");
	if (fp != nullptr)
		return (Rml::FileHandle)fp;

	// Attempt to open the file relative to the current working directory.
	fp = fopen(path.c_str(), "rb");
	return (Rml::FileHandle)fp;
}

void ShellFileInterface::Close(Rml::FileHandle file)
{
	fclose((FILE*)file);
}

size_t ShellFileInterface::Read(void* buffer, size_t size, Rml::FileHandle file)
{
	return fread(buffer, 1, size, (FILE*)file);
}

bool ShellFileInterface::Seek(Rml::FileHandle file, long offset, int origin)
{
	return fseek((FILE*)file, offset, origin) == 0;
}

size_t ShellFileInterface::Tell(Rml::FileHandle file)
{
	return ftell((FILE*)file);
}
