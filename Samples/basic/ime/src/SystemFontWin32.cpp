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
