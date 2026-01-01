#include "../include/PlatformExtensions.h"
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Platform.h>

#if defined RMLUI_PLATFORM_WIN32

	#include <RmlUi_Include_Windows.h>
	#include <io.h>
	#include <shlwapi.h>

#elif defined RMLUI_PLATFORM_MACOSX

	#include <CoreFoundation/CoreFoundation.h>
	#include <dirent.h>
	#include <string.h>

#elif defined RMLUI_PLATFORM_UNIX

	#include <X11/Xlib.h>
	#include <dirent.h>
	#include <stdlib.h>
	#include <string.h>
	#include <sys/stat.h>
	#include <unistd.h>

#endif

Rml::String PlatformExtensions::FindSamplesRoot()
{
#ifdef RMLUI_PLATFORM_WIN32
	// Test various relative paths to the "Samples" directory, based on common build and install locations.
	const char* candidate_paths[] = {
		".\\",
		"Samples\\",
		"..\\Samples\\",
		"..\\share\\Samples\\",
		"..\\..\\Samples\\",
		"..\\..\\..\\Samples\\",
		"..\\..\\..\\..\\Samples\\",
	};

	char path_buffer[MAX_PATH];

	// Fetch the path of the executable.
	if (GetModuleFileNameA(NULL, path_buffer, MAX_PATH) >= MAX_PATH && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		return {};
	Rml::String executable_directory_path = Rml::String(path_buffer);
	executable_directory_path = executable_directory_path.substr(0, executable_directory_path.rfind('\\') + 1);

	// We assume we have found the correct path if we can find the lookup file from it.
	const char* lookup_file = "assets\\rml.rcss";

	// Test the candidate paths relative to the executable folder, and the current working directory, respectively.
	for (const Rml::String relative_target_path : candidate_paths)
	{
		const Rml::String absolute_target_path = executable_directory_path + relative_target_path;
		const Rml::String absolute_lookup_path = absolute_target_path + lookup_file;
		if (PathFileExistsA(absolute_lookup_path.c_str()))
		{
			if (!PathCanonicalizeA(path_buffer, absolute_target_path.c_str()))
			{
				Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to canonicalize the path to the samples root: %s", absolute_target_path.c_str());
				return {};
			}

			return Rml::String(path_buffer);
		}

		const Rml::String relative_lookup_path = relative_target_path + lookup_file;
		if (PathFileExistsA(relative_lookup_path.c_str()))
		{
			const DWORD working_directory_length = GetFullPathNameA(relative_target_path.c_str(), MAX_PATH, path_buffer, nullptr);
			if (working_directory_length <= 0 || working_directory_length >= MAX_PATH)
			{
				Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to get the full path to the samples root: %s", relative_target_path.c_str());
				return {};
			}

			return Rml::String(path_buffer);
		}
	}

	Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to find the path to the samples root");

	return Rml::String();

#elif defined RMLUI_PLATFORM_MACOSX

	Rml::String path = "../Samples/";

	// Find the location of the executable.
	CFBundleRef bundle = CFBundleGetMainBundle();
	CFURLRef executable_url = CFBundleCopyExecutableURL(bundle);
	CFStringRef executable_posix_file_name = CFURLCopyFileSystemPath(executable_url, kCFURLPOSIXPathStyle);
	CFIndex max_length = CFStringGetMaximumSizeOfFileSystemRepresentation(executable_posix_file_name);
	char* executable_file_name = new char[max_length];
	if (!CFStringGetFileSystemRepresentation(executable_posix_file_name, executable_file_name, max_length))
		executable_file_name[0] = 0;

	Rml::String executable_path = Rml::String(executable_file_name);
	executable_path = executable_path.substr(0, executable_path.rfind("/") + 1);

	delete[] executable_file_name;
	CFRelease(executable_posix_file_name);
	CFRelease(executable_url);

	return executable_path + "../../../" + path;

#elif defined RMLUI_PLATFORM_EMSCRIPTEN

	return Rml::String("Samples/");

#elif defined RMLUI_PLATFORM_UNIX

	char path_buffer[PATH_MAX + 1];
	ssize_t len = readlink("/proc/self/exe", path_buffer, PATH_MAX);
	if (len == -1)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to determine the executable path");
		path_buffer[0] = 0;
	}
	else
	{
		// readlink() does not append a null byte to buf.
		path_buffer[len] = 0;
	}
	Rml::String executable_directory_path = Rml::String(path_buffer);
	executable_directory_path = executable_directory_path.substr(0, executable_directory_path.rfind("/") + 1);

	// We assume we have found the correct path if we can find the lookup file from it.
	const char* lookup_file = "assets/rml.rcss";

	// Test various relative paths to the "Samples" directory, based on common build and install locations.
	const char* candidate_paths[] = {
		"./",
		"Samples/",
		"../",
		"../Samples/",
		"../share/Samples/",
		"../../Samples/",
		"../../../Samples/",
		"../../../../Samples/",
	};

	auto isRegularFile = [](const Rml::String& path) -> bool {
		struct stat sb;
		return stat(path.c_str(), &sb) == 0 && S_ISREG(sb.st_mode);
	};
	auto GetAbsoluteFilePath = [&](const Rml::String& path) -> Rml::String {
		const char* absolute_path = realpath(path.c_str(), path_buffer);
		if (!absolute_path)
		{
			Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to canonicalize the path to the samples root: %s", path.c_str());
			return {};
		}
		return Rml::String(absolute_path) + '/';
	};

	for (const Rml::String relative_target_path : candidate_paths)
	{
		const Rml::String absolute_target_path = executable_directory_path + relative_target_path;
		const Rml::String absolute_lookup_path = absolute_target_path + lookup_file;
		if (isRegularFile(absolute_lookup_path))
			return GetAbsoluteFilePath(absolute_target_path);

		const Rml::String relative_lookup_path = relative_target_path + lookup_file;
		if (isRegularFile(relative_lookup_path))
			return GetAbsoluteFilePath(relative_target_path);
	}

	Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to find the path to the samples root");

	return Rml::String();

#else

	return Rml::String();

#endif
}

enum class ListType { Files, Directories };

static Rml::StringList ListFilesOrDirectories(ListType type, const Rml::String& directory, const Rml::String& extension)
{
	if (directory.empty())
		return Rml::StringList();

	Rml::StringList result;

#ifdef RMLUI_PLATFORM_WIN32

	const Rml::String find_path = directory + "/*." + (extension.empty() ? Rml::String("*") : extension);

	_finddata_t find_data;
	intptr_t find_handle = _findfirst(find_path.c_str(), &find_data);
	if (find_handle != -1)
	{
		do
		{
			if (strcmp(find_data.name, ".") == 0 || strcmp(find_data.name, "..") == 0)
				continue;

			bool is_directory = ((find_data.attrib & _A_SUBDIR) == _A_SUBDIR);
			bool is_file = (!is_directory && ((find_data.attrib & _A_NORMAL) == _A_NORMAL));

			if (((type == ListType::Files) && is_file) || ((type == ListType::Directories) && is_directory))
			{
				result.push_back(find_data.name);
			}

		} while (_findnext(find_handle, &find_data) == 0);

		_findclose(find_handle);
	}

#else

	struct dirent** file_list = nullptr;
	const int file_count = scandir(directory.c_str(), &file_list, 0, alphasort);
	if (file_count == -1)
		return Rml::StringList();

	for (int i = 0; i < file_count; i++)
	{
		if (strcmp(file_list[i]->d_name, ".") == 0 || strcmp(file_list[i]->d_name, "..") == 0)
			continue;

		bool is_directory = ((file_list[i]->d_type & DT_DIR) == DT_DIR);
		bool is_file = ((file_list[i]->d_type & DT_REG) == DT_REG);

		if (!extension.empty())
		{
			const char* last_dot = strrchr(file_list[i]->d_name, '.');
			if (!last_dot || strcmp(last_dot + 1, extension.c_str()) != 0)
				continue;
		}

		if ((type == ListType::Files && is_file) || (type == ListType::Directories && is_directory))
		{
			result.push_back(file_list[i]->d_name);
		}
	}
	for (int i = 0; i < file_count; i++)
		free(file_list[i]);

	free(file_list);

#endif

	return result;
}

Rml::StringList PlatformExtensions::ListDirectories(const Rml::String& in_directory)
{
	return ListFilesOrDirectories(ListType::Directories, in_directory, Rml::String());
}

Rml::StringList PlatformExtensions::ListFiles(const Rml::String& in_directory, const Rml::String& extension)
{
	return ListFilesOrDirectories(ListType::Files, in_directory, extension);
}
