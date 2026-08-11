#pragma once

#include <RmlUi/Core/Types.h>
#include <ShellFileInterface.h>
#include <cstdio>

/**
    File interface which first resolves paths against a user-provided working directory. Falls back to the shell
    behavior to make the sandbox's own assets available to the loaded document.
 */
class SandboxFileInterface : public ShellFileInterface {
public:
	SandboxFileInterface(const Rml::String& root) : ShellFileInterface(root) {}

	void SetWorkingDirectory(Rml::String directory)
	{
		if (!directory.empty() && directory.back() != '/' && directory.back() != '\\')
			directory += '/';
		working_directory = std::move(directory);
	}

	Rml::FileHandle Open(const Rml::String& path) override
	{
		if (!working_directory.empty())
		{
			if (FILE* fp = std::fopen((working_directory + path).c_str(), "rb"))
				return (Rml::FileHandle)fp;
		}
		return ShellFileInterface::Open(path);
	}

private:
	Rml::String working_directory;
};
