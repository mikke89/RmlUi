#pragma once

#include <RmlUi/Core/Log.h>
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
		ClearLoadedFiles();
	}

	Rml::FileHandle Open(const Rml::String& path) override
	{
		loaded_files.push_back(path);
		if (!working_directory.empty())
		{
			const Rml::String full_path = working_directory + path;
			if (FILE* fp = std::fopen(full_path.c_str(), "rb"))
				return (Rml::FileHandle)fp;
		}
		return ShellFileInterface::Open(path);
	}

	const Rml::StringList& GetLoadedFiles() const { return loaded_files; }

	void ClearLoadedFiles() { loaded_files.clear(); }

private:
	Rml::String working_directory;
	Rml::StringList loaded_files;
};
