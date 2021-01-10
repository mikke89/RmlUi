/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "Shell.h"
#include <RmlUi/Core/Core.h>
#include <string.h>

#ifdef RMLUI_PLATFORM_WIN32
#include <io.h>
#else
#include <dirent.h>
#endif

/// Loads the default fonts from the given path.
void Shell::LoadFonts(const char* directory)
{
	struct FontFace {
		Rml::String filename;
		bool fallback_face;
	};
	FontFace font_faces[] = {
		{ "LatoLatin-Regular.ttf",    false },
		{ "LatoLatin-Italic.ttf",     false },
		{ "LatoLatin-Bold.ttf",       false },
		{ "LatoLatin-BoldItalic.ttf", false },
		{ "NotoEmoji-Regular.ttf",    true  },
	};

	for (const FontFace& face : font_faces)
	{
		Rml::LoadFontFace(Rml::String(directory) + face.filename, face.fallback_face);
	}
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
			if (strcmp(find_data.name, ".") == 0 ||
				strcmp(find_data.name, "..") == 0)
				continue;

			bool is_directory = ((find_data.attrib & _A_SUBDIR) == _A_SUBDIR);
			bool is_file = (!is_directory && ((find_data.attrib & _A_NORMAL) == _A_NORMAL));

			if (((type == ListType::Files) && is_file) ||
				((type == ListType::Directories) && is_directory))
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
		if (strcmp(file_list[i]->d_name, ".") == 0 ||
			strcmp(file_list[i]->d_name, "..") == 0)
			continue;

		bool is_directory = ((file_list[i]->d_type & DT_DIR) == DT_DIR);
		bool is_file = ((file_list[i]->d_type & DT_REG) == DT_REG);

		if (!extension.empty())
		{
			const char* last_dot = strrchr(file_list[i]->d_name, '.');
			if (!last_dot || strcmp(last_dot + 1, extension.c_str()) != 0)
				continue;
		}

		if ((type == ListType::Files && is_file) ||
			(type == ListType::Directories && is_directory))
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

Rml::StringList Shell::ListDirectories(const Rml::String& in_directory)
{
	return ListFilesOrDirectories(ListType::Directories, in_directory, Rml::String());
}

Rml::StringList Shell::ListFiles(const Rml::String& in_directory, const Rml::String& extension)
{
	return ListFilesOrDirectories(ListType::Files, in_directory, extension);
}

