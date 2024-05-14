/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2024 The RmlUi Team, and contributors
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

#include "SystemFontWin32.h"
#include <RmlUi/Core/Log.h>
#include <RmlUi_Include_Windows.h>
#include <ShlObj.h>

static Rml::String GetSystemFontDirectory()
{
	Rml::String font_path;
	PWSTR fonts_path_wide;
	if (SHGetKnownFolderPath(FOLDERID_Fonts, 0, NULL, &fonts_path_wide) == S_OK)
	{
		int buffer_size = WideCharToMultiByte(CP_ACP, 0, fonts_path_wide, -1, NULL, 0, NULL, NULL);
		font_path.resize(std::max(buffer_size - 1, 0));
		WideCharToMultiByte(CP_ACP, 0, fonts_path_wide, -1, &font_path[0], buffer_size, NULL, NULL);

		CoTaskMemFree(fonts_path_wide);
	}

	return font_path;
}

Rml::Vector<Rml::String> GetSelectedSystemFonts()
{
	Rml::Vector<Rml::String> result;

	const Rml::String system_font_directory = GetSystemFontDirectory();
	if (!system_font_directory.empty())
	{
		// Partly based on: https://stackoverflow.com/a/57362436/2555318
		const char* system_font_files[] = {
			"segoeui.ttf ",  // Segoe UI (Latin; Greek; Cyrillic; Armenian; Georgian; Georgian Khutsuri; Arabic; Hebrew; Fraser)
			"tahoma.ttf ",   // Tahoma (Latin; Greek; Cyrillic; Armenian; Hebrew; Arabic; Thai)
			"meiryo.ttc ",   // Meiryo UI (Japanese)
			"msgothic.ttc",  // MS Gothic (Japanese)
			"msjh.ttc",      // Microsoft JhengHei (Chinese Traditional; Han; Han with Bopomofo)
			"msyh.ttc",      // Microsoft YaHei (Chinese Simplified; Han)
			"malgun.ttf ",   // Malgun Gothic (Korean)
			"simsun.ttc ",   // SimSun (Han Simplified)
			"seguiemj.ttf ", // Segoe UI (Latin; Greek; Cyrillic; Armenian; Georgian; Georgian Khutsuri; Arabic; Hebrew; Fraser)
		};

		for (const char* font_file : system_font_files)
		{
			Rml::String path = system_font_directory + '\\' + font_file;
			DWORD attributes = GetFileAttributesA(path.c_str());

			if (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY))
				result.push_back(path);
			else
				Rml::Log::Message(Rml::Log::LT_INFO, "Could not find system font file '%s', skipping.", path.c_str());
		}
	}

	return result;
}
